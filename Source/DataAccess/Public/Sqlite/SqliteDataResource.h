// Copyright 2014 afuzzyllama. All Rights Reserved.

#if WITH_SQLITE

#pragma once

#include "IDataResource.h"

typedef struct sqlite3 sqlite3;

/**
 * Implementation of IDataResource for Sqlite
 */
class SqliteDataResource : public IDataResource<sqlite3>
{
public:
    SqliteDataResource(FString DatabaseFileLocation);
    virtual ~SqliteDataResource();
    
    virtual bool Acquire() OVERRIDE;
    virtual bool Release() OVERRIDE;
    virtual sqlite3* Get() const;
    
private:
    FString     DatabaseFileLocation;
    sqlite3*    DatabaseResource;
};

#endif