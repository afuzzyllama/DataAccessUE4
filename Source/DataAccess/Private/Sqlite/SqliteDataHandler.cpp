// Copyright 2014 afuzzyllama. All Rights Reserved.

#if WITH_SQLITE

#include "DataAccessPrivatePCH.h"
#include "SqliteDataResource.h"
#include "SqliteDataHandler.h"

SqliteDataHandler::SqliteDataHandler(TSharedPtr<SqliteDataResource> DataResource)
: DataResource(DataResource)
, QueryStarted(false)
, SourceClass(nullptr)

{
    QueryParts.Empty();
    QueryParameters.Empty();
}

SqliteDataHandler::~SqliteDataHandler()
{
    QueryParts.Empty();
    QueryParameters.Empty();
    DataResource.Reset();
}

IDataHandler& SqliteDataHandler::Source(UClass* Source)
{
    check(Source);
    UIntProperty* IdProperty = FindFieldChecked<UIntProperty>(Source, "Id");
    check(IdProperty);
    
    QueryStarted = true;
    SourceClass = Source;
    QueryParts.Empty();
    QueryParameters.Empty();
    
    return *this;
}

IDataHandler& SqliteDataHandler::Where(FString FieldName, EDataHandlerOperator Operator, FString Condition)
{
    check(QueryStarted == true);
    bool bFound = false;
    
    UClass* FieldType = nullptr;
    // Terrible search, could be implemented better
    for(TFieldIterator<UProperty> Itr(SourceClass); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        
        if(Property->GetName() == FieldName)
        {
            bFound = true;
            FieldType = Property->GetClass();
            break;
        }
    }
    
    if(!bFound)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Where: FieldName \"%s\" does not exist in UClass \"%s\".  Clause not added"), *(FieldName), *(SourceClass->GetName()));
        return *this;
    }

    QueryParts.Add(FieldName);
    
    switch(Operator)
    {
    case GreaterThan:
        QueryParts.Add(">");
        break;
    case LessThan:
        QueryParts.Add("<");
        break;
    case Equals:
        QueryParts.Add("=");
        break;
    case LessThanOrEqualTo:
        QueryParts.Add("<=");
        break;
    case GreaterThanOrEqualTo:
        QueryParts.Add(">=");
        break;
    case NotEqualTo:
        QueryParts.Add("<>");
        break;
    }
    
    QueryParts.Add("?");
   
    TPair<UClass*, FString> NewPair;
    NewPair.Key = FieldType;
    NewPair.Value = Condition;
    
    QueryParameters.Add(NewPair);
    
    return *this;
}

IDataHandler& SqliteDataHandler::Or()
{
    check(QueryStarted == true);
    QueryParts.Add("OR");
    return *this;
}

IDataHandler& SqliteDataHandler::And()
{
    check(QueryStarted == true);
    QueryParts.Add("AND");
    return *this;
}

IDataHandler& SqliteDataHandler::BeginNested()
{
    check(QueryStarted == true);
    QueryParts.Add("(");
    return *this;
}

IDataHandler& SqliteDataHandler::EndNested()
{
    check(QueryStarted == true);
    QueryParts.Add(")");
    return *this;
}

bool SqliteDataHandler::Create(UObject* const Obj)
{
    check(Obj);
    check(QueryStarted == true);
    // Check that our object exists and that it had an Id property
    check(Obj->GetClass()->GetName() == SourceClass->GetName());
    
    // Build column names and values for the insert
    FString Columns("(");
    FString Values("(");
    
    for(TFieldIterator<UProperty> Itr(Obj->GetClass()); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        
        if(Property->GetName() == "Id" || Property->GetName() == "CreateTimestamp" || Property->GetName() == "LastUpdateTimestamp" )
        {
            continue;
        }
        
        Columns += FString::Printf(TEXT("%s,"), *(Property->GetName()));
        Values += "?,";
    }
    Columns.RemoveFromEnd(",", ESearchCase::IgnoreCase);
    Values.RemoveFromEnd(",", ESearchCase::IgnoreCase);
    Columns += ")";
    Values += ")";
    
    FString SqlStatement(FString::Printf(TEXT("INSERT INTO %s %s VALUES %s;"), *(Obj->GetClass()->GetName()), *Columns, *Values));
    
    // Create a prepared statement and bind the UObject to it
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    if(!BindObjectToStatement(Obj, SqliteStatement))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: error binding sqlite statement."));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    // Execute
    if(sqlite3_step(SqliteStatement) != SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: error executing insert statement.. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    sqlite3_finalize(SqliteStatement);
    

    // Get last id, create, and update timestamps and update the UObject
    int32 LastId = sqlite3_last_insert_rowid(DataResource->Get());
    UIntProperty* IdProperty = FindFieldChecked<UIntProperty>(Obj->GetClass(), "Id");
    IdProperty->SetPropertyValue_InContainer(Obj, LastId);
    
    SqlStatement = FString::Printf(TEXT("SELECT CreateTimestamp, LastUpdateTimestamp FROM %s WHERE Id = ?;"), *(Obj->GetClass()->GetName()));
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*(SqlStatement)), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot prepare sqlite statement for timestamps. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    if(sqlite3_bind_int(SqliteStatement, 1, LastId))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot bind class name to sqlite statement for seq. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    if(sqlite3_step(SqliteStatement) != SQLITE_ROW)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot step sqlite statement for seq. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
   
    UIntProperty* CreateTimestampProperty = FindFieldChecked<UIntProperty>(Obj->GetClass(), "CreateTimestamp");
    CreateTimestampProperty->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, 0));

    UIntProperty* LastUpdateTimestampProperty = FindFieldChecked<UIntProperty>(Obj->GetClass(), "LastUpdateTimestamp");
    LastUpdateTimestampProperty->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, 1));

    sqlite3_finalize(SqliteStatement);
    ClearQuery();
    return true;
}


bool SqliteDataHandler::Update(UObject* const Obj)
{
    check(Obj);
    check(QueryStarted == true);
    check(Obj->GetClass()->GetName() == SourceClass->GetName());
    
    // Build set commands for the update
    FString Sets;
    int32 PropertyCount = 0;
    for(TFieldIterator<UProperty> Itr(Obj->GetClass()); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        if(Property->GetName() == "Id" || Property->GetName() == "CreateTimestamp" || Property->GetName() == "LastUpdateTimestamp" )
        {
            continue;
        }
        
        ++PropertyCount;
        Sets += FString::Printf(TEXT("%s = ?,"), *(Property->GetName()));
    }
    Sets.RemoveFromEnd(",", ESearchCase::IgnoreCase);
    
    FString SqlStatement(FString::Printf(TEXT("UPDATE %s SET %s %s;"), *(SourceClass->GetName()), *Sets, *(GenerateWhereClause())));
    
    // Prepare a statement and bind the UObject to it.  Also bind the update Id.
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Update: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    if(!BindObjectToStatement(Obj, SqliteStatement))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Update: error binding sqlite statement."));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }

    // Bind Where Paramters
    if(!BindWhereToStatement(SqliteStatement, PropertyCount + 1))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Update: cannot bind where clause. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }

    // Execute
    if(sqlite3_step(SqliteStatement) != SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Update: error executing update statement.. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    if(sqlite3_changes(DataResource->Get()) == 0)
    {
        UE_LOG(LogDataAccess, Log, TEXT("Update: Nothing to update"));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    sqlite3_finalize(SqliteStatement);
    
    // Get create and update timestamps and update the UObject
    SqlStatement = FString::Printf(TEXT("SELECT DISTINCT LastUpdateTimestamp FROM %s %s;"), *(Obj->GetClass()->GetName()), *(GenerateWhereClause()));
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*(SqlStatement)), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot prepare sqlite statement for timestamp. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    if(!BindWhereToStatement(SqliteStatement, 1))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot bind where clause for timestamp. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    if(sqlite3_step(SqliteStatement) != SQLITE_ROW)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot step sqlite statement for seq. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
   
    UIntProperty* LastUpdateTimestampProperty = FindFieldChecked<UIntProperty>(Obj->GetClass(), "LastUpdateTimestamp");
    LastUpdateTimestampProperty->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, 0));
    
    sqlite3_finalize(SqliteStatement);
    ClearQuery();
    return true;
}

bool SqliteDataHandler::Delete()
{
    check(QueryStarted == true);
    
    FString SqlStatement(FString::Printf(TEXT("DELETE FROM %s %s"), *(SourceClass->GetName()), *(GenerateWhereClause())));
    
    // Prepare a statement and bind the Id to it
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Delete: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    // Bind Where Paramters
    if(!BindWhereToStatement(SqliteStatement))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Delete: cannot bind where clause. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    // Execute
    if(sqlite3_step(SqliteStatement) != SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Delete: error executing delete statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    if(sqlite3_changes(DataResource->Get()) == 0)
    {
        UE_LOG(LogDataAccess, Log, TEXT("Delete: Nothing to delete"));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    sqlite3_finalize(SqliteStatement);
    ClearQuery();
    return true;
}

bool SqliteDataHandler::Count(int32& OutCount)
{
    check(QueryStarted == true);

    OutCount = 0;
    FString SqlStatement(FString::Printf(TEXT("SELECT COUNT(Id) FROM %s %s;"), *(SourceClass->GetName()), *(GenerateWhereClause())));

    // Preare statement and bind Id to it
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Count: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    // Bind Where Paramters
    if(!BindWhereToStatement(SqliteStatement))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Count: cannot bind where clause. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    // Execute
    int32 ResultCode = sqlite3_step(SqliteStatement);
    if(ResultCode == SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Log, TEXT("Count: nothing selected."));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    else if(ResultCode != SQLITE_ROW)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Count: error executing select statement."));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }

    // Bind the results to the passed in object
    OutCount = sqlite3_column_int(SqliteStatement, 0);
    
    sqlite3_finalize(SqliteStatement);
    ClearQuery();
    return true;
}

bool SqliteDataHandler::First(UObject* const OutObj)
{
    check(OutObj);
    check(QueryStarted == true);
    
    // Build columns for select statement
    FString Columns;
    for(TFieldIterator<UProperty> Itr(SourceClass); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        Columns += FString::Printf(TEXT("%s,"), *(Property->GetName()));
    }
    Columns.RemoveFromEnd(",", ESearchCase::IgnoreCase);
    
    FString SqlStatement(FString::Printf(TEXT("SELECT %s FROM %s %s;"), *Columns, *(SourceClass->GetName()), *(GenerateWhereClause())));

    // Preare statement and bind Id to it
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("First: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    // Bind Where Paramters
    if(!BindWhereToStatement(SqliteStatement))
    {
        UE_LOG(LogDataAccess, Error, TEXT("First: cannot bind where clause. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    // Execute
    int32 ResultCode = sqlite3_step(SqliteStatement);
    
    if(ResultCode == SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Log, TEXT("First: nothing selected."));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    else if(ResultCode != SQLITE_ROW)
    {
        UE_LOG(LogDataAccess, Error, TEXT("First: error executing select statement."));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    // Bind the results to the passed in object
    if(!BindStatementToObject(SqliteStatement, OutObj))
    {
        UE_LOG(LogDataAccess, Error, TEXT("First: error binding results."));
        sqlite3_finalize(SqliteStatement);
        ClearQuery();
        return false;
    }
    
    sqlite3_finalize(SqliteStatement);
    ClearQuery();
    return true;
}

bool SqliteDataHandler::Get(TArray<UObject*>& OutObjs)
{
    check(QueryStarted == true);

    if(OutObjs.Num() <= 0)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Get: cannot get with an empty array"));
        OutObjs.Empty();
        ClearQuery();
        return false;
    }
    
    // Build columns for select statement
    FString Columns;
    for(TFieldIterator<UProperty> Itr(SourceClass); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        Columns += FString::Printf(TEXT("%s,"), *(Property->GetName()));
    }
    Columns.RemoveFromEnd(",", ESearchCase::IgnoreCase);
    
    FString SqlStatement(FString::Printf(TEXT("SELECT %s FROM %s %s;"), *Columns, *(SourceClass->GetName()), *(GenerateWhereClause())));
    
    // Preare statement and bind Id to it
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Get: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        OutObjs.Empty();
        ClearQuery();
        return false;
    }
    
    // Bind Where Paramters
    if(!BindWhereToStatement(SqliteStatement))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Get: cannot bind where clause. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        OutObjs.Empty();
        ClearQuery();
        return false;
    }
    
    // Execute
    int32 ResultCode = sqlite3_step(SqliteStatement);
    if(ResultCode == SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Log, TEXT("Get: nothing selected."));
        sqlite3_finalize(SqliteStatement);
        OutObjs.Empty();
        ClearQuery();
        return false;
    }
    else if(ResultCode != SQLITE_ROW)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Get: error executing select statement."));
        sqlite3_finalize(SqliteStatement);
        OutObjs.Empty();
        ClearQuery();
        return false;
    }

    int32 CurrentIndex = 0;
    // Bind the results to the passed in object
    while(ResultCode != SQLITE_DONE)
    {
        if(CurrentIndex == OutObjs.Num())
        {
            UE_LOG(LogDataAccess, Warning, TEXT("Get: Passed array not large enough to handle all objects.  %i objected returned"), CurrentIndex);
            break;
        }
    
        if(!BindStatementToObject(SqliteStatement, OutObjs[CurrentIndex]))
        {
            UE_LOG(LogDataAccess, Error, TEXT("Get: error binding results."));
            sqlite3_finalize(SqliteStatement);
            ClearQuery();
            OutObjs.Empty();
            return false;
        }
        ResultCode = sqlite3_step(SqliteStatement);
        ++CurrentIndex;
    }
    
    sqlite3_finalize(SqliteStatement);
    ClearQuery();
    return true;
}

void SqliteDataHandler::ClearQuery()
{
    QueryStarted = false;
    SourceClass = nullptr;
    QueryParts.Empty();
    QueryParameters.Empty();
}

FString SqliteDataHandler::GenerateWhereClause()
{
    FString WhereClause("");
    if(QueryParts.Num() > 0)
    {
        WhereClause = "WHERE " + FString::Join(QueryParts, TEXT(" "));
    }
    
    return WhereClause;
}

bool SqliteDataHandler::BindWhereToStatement(sqlite3_stmt* const SqliteStatement, int32 ParameterIndex)
{
    bool bSuccess = true;
    // Binding index is 1 based not 0 based
    for(auto Itr = QueryParameters.CreateConstIterator(); Itr; ++Itr)
    {
        const TPair<UClass*, FString> CurrentParameter = *Itr;
    
        if(CurrentParameter.Key->GetName() == UByteProperty::StaticClass()->GetName())
        {
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind byte. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UInt8Property::StaticClass()->GetName())
        {
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind int8. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UInt16Property::StaticClass()->GetName())
        {
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind int16. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UIntProperty::StaticClass()->GetName())
        {
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind int32. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UInt64Property::StaticClass()->GetName())
        {
            if(sqlite3_bind_int64(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind int64. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UUInt16Property::StaticClass()->GetName())
        {
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind uint16. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UUInt32Property::StaticClass()->GetName())
        {
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind uint32. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UUInt64Property::StaticClass()->GetName())
        {
            if(sqlite3_bind_int64(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind uint64. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UFloatProperty::StaticClass()->GetName())
        {
            if(sqlite3_bind_double(SqliteStatement, ParameterIndex, FCString::Atof(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind float. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UDoubleProperty::StaticClass()->GetName())
        {
            if(sqlite3_bind_double(SqliteStatement, ParameterIndex, FCString::Atof(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind double. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UBoolProperty::StaticClass()->GetName())
        {
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, FCString::Atoi(*CurrentParameter.Value)) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind bool. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(CurrentParameter.Key->GetName() == UStrProperty::StaticClass()->GetName())
        {
            if(sqlite3_bind_text(SqliteStatement, ParameterIndex, TCHAR_TO_UTF8(*CurrentParameter.Value), FCString::Strlen(*CurrentParameter.Value), SQLITE_TRANSIENT) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: cannot bind string. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else
        {
            UE_LOG(LogDataAccess, Error, TEXT("BindWhereToStatement: Data type %s is not supported"), *(CurrentParameter.Key->GetName()));
            bSuccess = false;
        }
        ++ParameterIndex;
    }

    return bSuccess;
}

bool SqliteDataHandler::BindObjectToStatement(UObject* const Obj, sqlite3_stmt* const SqliteStatement)
{
    check(SqliteStatement);
    int32 ParameterIndex = 1;
    bool bSuccess = true;
    
    // Iterate through all of the UObject properties and bind values to the passed in prepared statement
    for(TFieldIterator<UProperty> Itr(Obj->GetClass()); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        if(Property->GetName() == "Id" || Property->GetName() == "CreateTimestamp" || Property->GetName() == "LastUpdateTimestamp" )
        {
            continue;
        }
        
        if(Property->IsA(UByteProperty::StaticClass()))
        {
            UByteProperty* ByteProperty = CastChecked<UByteProperty>(Property);
            uint8 CurrentValue = ByteProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<uint8>(Obj));
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind byte. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UInt8Property::StaticClass()))
        {
            UInt8Property* Int8Property = CastChecked<UInt8Property>(Property);
            int8 CurrentValue = Int8Property->GetPropertyValue(Property->ContainerPtrToValuePtr<uint8>(Obj));
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind int8. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UInt16Property::StaticClass()))
        {
            UInt16Property* Int16Property = CastChecked<UInt16Property>(Property);
            int16 CurrentValue = Int16Property->GetPropertyValue(Property->ContainerPtrToValuePtr<int16>(Obj));
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind int16. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UIntProperty::StaticClass()))
        {
            UIntProperty* Int32Property = CastChecked<UIntProperty>(Property);
            int32 CurrentValue = Int32Property->GetPropertyValue(Property->ContainerPtrToValuePtr<int32>(Obj));
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind int32. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UInt64Property::StaticClass()))
        {
            UInt64Property* Int64Property = CastChecked<UInt64Property>(Property);
            int64 CurrentValue = Int64Property->GetPropertyValue(Property->ContainerPtrToValuePtr<int64>(Obj));
            if(sqlite3_bind_int64(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind int64. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UUInt16Property::StaticClass()))
        {
            UUInt16Property* UInt16Property = CastChecked<UUInt16Property>(Property);
            uint16 CurrentValue = UInt16Property->GetPropertyValue(Property->ContainerPtrToValuePtr<uint16>(Obj));
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind uint16. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UUInt32Property::StaticClass()))
        {
            UUInt32Property* UInt32Property = CastChecked<UUInt32Property>(Property);
            uint32 CurrentValue = UInt32Property->GetPropertyValue(Property->ContainerPtrToValuePtr<uint32>(Obj));
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind uint32. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UUInt64Property::StaticClass()))
        {
            UUInt64Property* UInt64Property = CastChecked<UUInt64Property>(Property);
            uint64 CurrentValue = UInt64Property->GetPropertyValue(Property->ContainerPtrToValuePtr<uint64>(Obj));
            if(sqlite3_bind_int64(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind uint64. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UFloatProperty::StaticClass()))
        {
            UFloatProperty* FloatProperty = CastChecked<UFloatProperty>(Property);
            float CurrentValue = FloatProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<float>(Obj));
            if(sqlite3_bind_double(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind float. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UDoubleProperty::StaticClass()))
        {
            UDoubleProperty* DoubleProperty = CastChecked<UDoubleProperty>(Property);
            double CurrentValue = DoubleProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<double>(Obj));
            if(sqlite3_bind_double(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind double. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UBoolProperty::StaticClass()))
        {
            UBoolProperty* BoolProperty = CastChecked<UBoolProperty>(Property);
            bool CurrentValue = BoolProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<bool>(Obj));
            if(sqlite3_bind_int(SqliteStatement, ParameterIndex, CurrentValue) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind bool. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UStrProperty::StaticClass()))
        {
            UStrProperty* StrProperty = CastChecked<UStrProperty>(Property);
            FString CurrentValue = StrProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<FString>(Obj));
            if(sqlite3_bind_text(SqliteStatement, ParameterIndex, TCHAR_TO_UTF8(*CurrentValue), FCString::Strlen(*CurrentValue), SQLITE_TRANSIENT) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind string. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else if(Property->IsA(UArrayProperty::StaticClass()))
        {
            UArrayProperty* ArrayProperty = CastChecked<UArrayProperty>(Property);
            FScriptArrayHelper_InContainer ArrayHelper(ArrayProperty, Obj);
            if(sqlite3_bind_blob(SqliteStatement, ParameterIndex, ArrayHelper.GetRawPtr(), ArrayHelper.Num() * ArrayProperty->Inner->ElementSize, SQLITE_TRANSIENT) != SQLITE_OK)
            {
                UE_LOG(LogDataAccess, Error, TEXT("BindParameters: cannot bind array. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
                bSuccess = false;
            }
        }
        else
        {
            UE_LOG(LogDataAccess, Error, TEXT("BindParameters: Data type on UPROPERTY() %s is not supported"), *(Property->GetName()));
            bSuccess = false;
        }
        ++ParameterIndex;
    }
    
    return bSuccess;
}

bool SqliteDataHandler::BindStatementToObject(sqlite3_stmt* const SqliteStatement, UObject* const Obj)
{
    check(SqliteStatement);

    int32 ColumnIndex = 0;
    bool bSuccess = true;
    
    // Iterator through all of the UObject properties and the passed in prepared statement to it
    for(TFieldIterator<UProperty> Itr(Obj->GetClass()); Itr; ++Itr)
    {
        UProperty* Property = *Itr;

        if(Property->IsA(UByteProperty::StaticClass()))
        {
            UByteProperty* ByteProperty = CastChecked<UByteProperty>(Property);
            ByteProperty->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UInt8Property::StaticClass()))
        {
            UInt8Property* Int8Property = CastChecked<UInt8Property>(Property);
            Int8Property->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UInt16Property::StaticClass()))
        {
            UInt16Property* Int16Property = CastChecked<UInt16Property>(Property);
            Int16Property->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UIntProperty::StaticClass()))
        {
            UIntProperty* Int32Property = CastChecked<UIntProperty>(Property);
            Int32Property->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UInt64Property::StaticClass()))
        {
            UInt64Property* Int64Property = CastChecked<UInt64Property>(Property);
            Int64Property->SetPropertyValue_InContainer(Obj, sqlite3_column_int64(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UUInt16Property::StaticClass()))
        {
            UUInt16Property* UInt16Property = CastChecked<UUInt16Property>(Property);
            UInt16Property->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UUInt32Property::StaticClass()))
        {
            UUInt32Property* UInt32Property = CastChecked<UUInt32Property>(Property);
            UInt32Property->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UUInt64Property::StaticClass()))
        {
            UUInt64Property* UInt64Property = CastChecked<UUInt64Property>(Property);
            UInt64Property->SetPropertyValue_InContainer(Obj, sqlite3_column_int64(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UFloatProperty::StaticClass()))
        {
            UFloatProperty* FloatProperty = CastChecked<UFloatProperty>(Property);
            FloatProperty->SetPropertyValue_InContainer(Obj, sqlite3_column_double(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UDoubleProperty::StaticClass()))
        {
            UDoubleProperty* DoubleProperty = CastChecked<UDoubleProperty>(Property);
            DoubleProperty->SetPropertyValue_InContainer(Obj, sqlite3_column_double(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UBoolProperty::StaticClass()))
        {
            UBoolProperty* BoolProperty = CastChecked<UBoolProperty>(Property);
            BoolProperty->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, ColumnIndex));
        }
        else if(Property->IsA(UStrProperty::StaticClass()))
        {
            UStrProperty* StrProperty = CastChecked<UStrProperty>(Property);
            StrProperty->SetPropertyValue_InContainer(Obj, UTF8_TO_TCHAR(sqlite3_column_text(SqliteStatement, ColumnIndex)));
        }
        else if(Property->IsA(UArrayProperty::StaticClass()))
        {
            UArrayProperty* ArrayProperty = CastChecked<UArrayProperty>(Property);
            const uint8* SrcRaw = static_cast<const uint8*>(sqlite3_column_blob(SqliteStatement, ColumnIndex));
            int32 SrcCount = sqlite3_column_bytes(SqliteStatement, ColumnIndex);

            // Copy values internal is really looking for two void* pointers to TArrays.  Therefore, here we should make a TArray<uint8> of the data from sqlite.
            TArray<uint8> Src;
            Src.AddUninitialized(SrcCount);
            
            for(int32 i = 0; i < SrcCount; ++i)
            {
                Src[i] = SrcRaw[i];
            }
            
            void* Dest = ArrayProperty->ContainerPtrToValuePtr<void>(Obj);
            
            ArrayProperty->CopyValuesInternal(Dest, static_cast<void*>(&Src), 1);
            
            // We need to resize the array to represent the element size and not the byte size
            FScriptArrayHelper_InContainer ArrayHelper(ArrayProperty, Obj);
            ArrayHelper.Resize( SrcCount / ArrayProperty->Inner->ElementSize );
        }
        else
        {
            UE_LOG(LogDataAccess, Error, TEXT("BindParameters: Data type on UPROPERTY() %s is not supported"), *(Property->GetName()));
            bSuccess = false;
        }
        ++ColumnIndex;
    }
    
    return bSuccess;
}

#endif