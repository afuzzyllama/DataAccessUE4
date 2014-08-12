// Copyright 2014 afuzzyllama. All Rights Reserved.

#if WITH_SQLITE

#include "DataAccessPrivatePCH.h"
#include "SqliteDataResource.h"
#include "sqlite3.h"
#include "SqliteDataHandler.h"

SqliteDataHandler::SqliteDataHandler(TSharedPtr<SqliteDataResource> DataResource)
: DataResource(DataResource)
{}

SqliteDataHandler::~SqliteDataHandler()
{
    DataResource.Reset();
}

bool SqliteDataHandler::Create(UObject* const Obj)
{
    // Check that our object exists and that it had an Id property
    check(Obj);
    UIntProperty* IdProperty = FindFieldChecked<UIntProperty>(Obj->GetClass(), "Id");
    check(IdProperty);
    
    // Build column names and values for the insert
    FString Columns("(");
    FString Values("(");
    
    for(TFieldIterator<UProperty> Itr(Obj->GetClass()); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        
        if(Property->GetName() == "Id")
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
        return false;
    }
    
    if(!BindObjectToStatement(Obj, SqliteStatement))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: error binding sqlite statement."));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    // Execute
    if(sqlite3_step(SqliteStatement) != SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: error executing insert statement."));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    sqlite3_finalize(SqliteStatement);
    

    // Get last auto increment id and update the UObject
    SqlStatement = "SELECT seq FROM sqlite_sequence WHERE name = ?";
    
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*(SqlStatement)), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot prepare sqlite statement for seq. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    if(sqlite3_bind_text(SqliteStatement, 1, TCHAR_TO_UTF8(*(Obj->GetClass()->GetName())), FCString::Strlen(*(Obj->GetClass()->GetName())), SQLITE_TRANSIENT) )
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot bind class name to sqlite statement for seq. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    if(sqlite3_step(SqliteStatement) != SQLITE_ROW)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Create: cannot step sqlite statement for seq. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
   
    IdProperty->SetPropertyValue_InContainer(Obj, sqlite3_column_int(SqliteStatement, 0));

    return true;
}

bool SqliteDataHandler::Read(int32 Id, UObject* const Obj)
{
    // Check that our object exists and that it had an Id property
    check(Obj);
    UIntProperty* IdProperty = FindFieldChecked<UIntProperty>(Obj->GetClass(), "Id");
    check(IdProperty);
    
    // Build columns for select statement
    FString Columns;
    for(TFieldIterator<UProperty> Itr(Obj->GetClass()); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        Columns += FString::Printf(TEXT("%s,"), *(Property->GetName()));
    }
    Columns.RemoveFromEnd(",", ESearchCase::IgnoreCase);
    
    FString SqlStatement(FString::Printf(TEXT("SELECT %s FROM %s WHERE Id = ?;"), *Columns, *(Obj->GetClass()->GetName())));
    
    // Preare statement and bind Id to it
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Read: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    if(sqlite3_bind_int(SqliteStatement, 1, Id) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Read: error binding id to sqlite statement."));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    // Execute
    int32 ResultCode = sqlite3_step(SqliteStatement);
    
    if(ResultCode == SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Log, TEXT("Read: nothing selected."));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    else if(ResultCode != SQLITE_ROW)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Read: error executing select statement."));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    // Bind the results to the passed in object
    BindStatementToObject(SqliteStatement, Obj);
    
    sqlite3_finalize(SqliteStatement);
    return true;
}

bool SqliteDataHandler::Update(UObject* const Obj)
{
    // Check that our object exists and that it had an Id property
    check(Obj);
    UIntProperty* IdProperty = FindFieldChecked<UIntProperty>(Obj->GetClass(), "Id");
    check(IdProperty);
    
    // Build set commands for the update
    FString Sets;
    int32 PropertyCount = 0;
    for(TFieldIterator<UProperty> Itr(Obj->GetClass()); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        if(Property->GetName() == "Id")
        {
            continue;
        }
        
        ++PropertyCount;
        Sets += FString::Printf(TEXT("%s = ?,"), *(Property->GetName()));
    }
    Sets.RemoveFromEnd(",", ESearchCase::IgnoreCase);
    
    FString SqlStatement(FString::Printf(TEXT("UPDATE %s SET %s WHERE Id = ?;"), *(Obj->GetClass()->GetName()), *Sets));
    
    // Prepare a statement and bind the UObject to it.  Also bind the update Id.
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Update: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    if(!BindObjectToStatement(Obj, SqliteStatement))
    {
        UE_LOG(LogDataAccess, Error, TEXT("Update: error binding sqlite statement."));
        sqlite3_finalize(SqliteStatement);
        return false;
    }

    if(sqlite3_bind_int(SqliteStatement, PropertyCount + 1, IdProperty->GetPropertyValue(IdProperty->ContainerPtrToValuePtr<int32>(Obj))) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Update: error binding id to sqlite statement."));
        sqlite3_finalize(SqliteStatement);
        return false;
    }

    // Execute
    if(sqlite3_step(SqliteStatement) != SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Update: error executing update statement."));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    if(sqlite3_changes(DataResource->Get()) == 0)
    {
        UE_LOG(LogDataAccess, Log, TEXT("Update: Nothing to update"));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    sqlite3_finalize(SqliteStatement);
    return true;
}

bool SqliteDataHandler::Delete(UObject* const Obj)
{
    // Check that our object exists and that it had an Id property
    check(Obj);
    UIntProperty* IdProperty = FindFieldChecked<UIntProperty>(Obj->GetClass(), "Id");
    check(IdProperty);
    
    FString SqlStatement(FString::Printf(TEXT("DELETE FROM %s WHERE Id = ?"), *(Obj->GetClass()->GetName())));
    
    // Prepare a statement and bind the Id to it
    sqlite3_stmt* SqliteStatement;
    if(sqlite3_prepare_v2(DataResource->Get(), TCHAR_TO_UTF8(*SqlStatement), FCString::Strlen(*SqlStatement), &SqliteStatement, nullptr) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Delete: cannot prepare sqlite statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    if(sqlite3_bind_int(SqliteStatement, 1, IdProperty->GetPropertyValue(IdProperty->ContainerPtrToValuePtr<int32>(Obj))) != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Delete: cannot bind id. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    // Execute
    if(sqlite3_step(SqliteStatement) != SQLITE_DONE)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Delete: error executing delete statement. Error message \"%s\""), UTF8_TO_TCHAR(sqlite3_errmsg(DataResource->Get())));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    if(sqlite3_changes(DataResource->Get()) == 0)
    {
        UE_LOG(LogDataAccess, Log, TEXT("Delete: Nothing to delete"));
        sqlite3_finalize(SqliteStatement);
        return false;
    }
    
    sqlite3_finalize(SqliteStatement);
    return true;
}

bool SqliteDataHandler::BindObjectToStatement(UObject* const Obj, sqlite3_stmt* const SqliteStatement)
{
    check(Obj);
    check(SqliteStatement);
    int32 ParameterIndex = 1;
    bool bSuccess = true;
    
    // Iterate through all of the UObject properties and bind values to the passed in prepared statement
    for(TFieldIterator<UProperty> Itr(Obj->GetClass()); Itr; ++Itr)
    {
        UProperty* Property = *Itr;
        if(Property->GetName() == "Id")
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
    check(Obj);
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