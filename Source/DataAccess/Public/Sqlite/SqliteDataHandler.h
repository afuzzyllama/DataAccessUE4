// Copyright 2015 afuzzyllama. All Rights Reserved.
#pragma once

#include "IDataHandler.h"

// forward declaration
class SqliteDataResource;
typedef struct sqlite3_stmt sqlite3_stmt;

/**
 * Implementation of the IDataHandler for Sqlite
 */
class DATAACCESS_API SqliteDataHandler : public IDataHandler
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
    virtual IDataHandler& Source(UClass* Source);

    virtual IDataHandler& Where(FString FieldName, EDataHandlerOperator Operator, FString Condition);
    virtual IDataHandler& Or();
    virtual IDataHandler& And();
    virtual IDataHandler& BeginNested();
    virtual IDataHandler& EndNested();

    virtual bool Create(UObject* const Obj);
    virtual bool Update(UObject* const Obj);
    virtual bool Delete();
    virtual bool Count(int32& OutCount);
    virtual bool First(UObject* const OutObj);
    virtual bool Get(TArray<UObject*>& OutObjs);

	virtual bool ExecuteQuery(FString Query, TArray< TSharedPtr<class FJsonValue> >& JsonArray);
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
     * Bind result to UObject
     *
     * @param SqliteStatement       statement to bind from
     * @param Obj                   object to bind to
     * @return                      true if successful, false otherwise
     */
    bool BindStatementToObject(sqlite3_stmt* const SqliteStatement, UObject* const Obj);

	/**
	* Bind result to JsonValue
	*
	* @param SqliteStatement       statement to bind from
	* @param JsonValue             JsonValue to bind to
	* @return                      true if successful, false otherwise
	*/
	bool BindStatementToArray(sqlite3_stmt* const SqliteStatement, TSharedPtr<class FJsonValue>& JsonValue);
};
