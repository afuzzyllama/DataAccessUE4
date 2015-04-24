// Copyright 2015 afuzzyllama. All Rights Reserved.
#pragma once

#include "IDataResource.h"

typedef struct sqlite3 sqlite3;

/**
 * Implementation of IDataResource for Sqlite
 */
class DATAACCESS_API SqliteDataResource : public IDataResource<sqlite3>
{
public:
    SqliteDataResource(FString DatabaseFileLocation);
    virtual ~SqliteDataResource();
    
    virtual bool Acquire();
    virtual bool Release();
    virtual sqlite3* Get() const;
    
private:
    FString     DatabaseFileLocation;
    sqlite3*    DatabaseResource;
};
