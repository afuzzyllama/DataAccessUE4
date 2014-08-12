// Copyright 2014 afuzzyllama. All Rights Reserved.

#pragma once

#include "IDataResource.h"

class sqlite3;

/**
 * Implementation of IDataResource for Sqlite
 */
class SqliteDataResource : public IDataResource<sqlite3>
{
public:
    SqliteDataResource(FString DatabaseFileLocation);
    virtual ~SqliteDataResource();
    
    virtual bool Acquire() override;
    virtual bool Release() override;
    virtual sqlite3* Get() const;
    
private:
    FString     DatabaseFileLocation;
    sqlite3*    DatabaseResource;
};