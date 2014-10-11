// Copyright 2014 afuzzyllama. All Rights Reserved.

#if WITH_SQLITE

#pragma once

#include "IDataHandler.h"

// forward declaration
class SqliteDataResource;
typedef struct sqlite3_stmt sqlite3_stmt;

/**
 * Implementation of the IDataHandler for Sqlite
 */
class SqliteDataHandler : public IDataHandler
{
public:
    /**
     * Construct a new sqlite data handler. Assumes a successfully created data resource is passed in
     *
     * @param   DataResource    sqlite data resource
     */
    SqliteDataHandler(TSharedPtr<SqliteDataResource> DataResource);
    virtual ~SqliteDataHandler();

    // IDataHandler interface
    virtual IDataHandler& Source(UClass* Source) OVERRIDE;

    virtual IDataHandler& Where(FString FieldName, EDataHandlerOperator Operator, FString Condition) OVERRIDE;
    virtual IDataHandler& Or() OVERRIDE;
    virtual IDataHandler& And() OVERRIDE;
    virtual IDataHandler& BeginNested() OVERRIDE;
    virtual IDataHandler& EndNested() OVERRIDE;

    virtual bool Create(UObject* const Obj) OVERRIDE;
    virtual bool Update(UObject* const Obj) OVERRIDE;
    virtual bool Delete() OVERRIDE;
    virtual bool Count(int32& OutCount) OVERRIDE;
    virtual bool First(UObject* const OutObj) OVERRIDE;
    virtual bool Get(TArray<UObject*>& OutObjs) OVERRIDE;
    // End of IDataHandler interface

private:
    TSharedPtr<SqliteDataResource> DataResource;
    
    bool QueryStarted;
    UClass* SourceClass;
    TArray<FString> QueryParts;
    TArray<TPair<UClass*, FString>> QueryParameters;
    
    void ClearQuery();
    FString GenerateWhereClause();

    /**
     *
     */
    bool BindWhereToStatement(sqlite3_stmt* const SqliteStatement, int32 ParameterIndex = 1);
    
    /**
     * Bind parameters to the passed in sqlite statement
     *
     * @param   Obj                 object that contains the data we want to bind
     * @param   SqliteStatement     sqlite statement to bind to
     * @return                      true if successful, false otherwise
     */
    bool BindObjectToStatement(UObject* const Obj, sqlite3_stmt* const SqliteStatement);
    
    /**
     * Bind results to UObject
     *
     * @param SqliteStatement       statement to bind from
     * @param Obj                   object to bind to
     * @return                      true if successful, false otherwise
     */
    bool BindStatementToObject(sqlite3_stmt* const SqliteStatement, UObject* const Obj);
};

#endif