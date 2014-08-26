// Copyright 2014 afuzzyllama. All Rights Reserved.

#if WITH_SQLITE

#pragma once

#include "IDataHandler.h"

// forward declaration
class SqliteDataResource;
class sqlite3_stmt;

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
    virtual IDataHandler& Source(UClass* Source) override;

    virtual IDataHandler& Where(FString FieldName, EDataHandlerOperator Operator, FString Condition) override;
    virtual IDataHandler& Or() override;
    virtual IDataHandler& And() override;
    virtual IDataHandler& BeginNested() override;
    virtual IDataHandler& EndNested() override;

    virtual bool Create(UObject* const Obj) override;
    virtual bool Update(UObject* const Obj) override;
    virtual bool Delete() override;
    virtual bool First(UObject* const OutObj) override;
    virtual bool Get(TArray<UObject*>& OutObjs) override;
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