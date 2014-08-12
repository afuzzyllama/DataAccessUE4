// Copyright 2014 afuzzyllama. All Rights Reserved.

#include "DataAccessPrivatePCH.h"
#include "sqlite3.h"
#include "SqliteDataResource.h"

SqliteDataResource::SqliteDataResource(FString DatabaseFileLocation)
: DatabaseFileLocation(DatabaseFileLocation)
{}

SqliteDataResource::~SqliteDataResource()
{
    if(!DatabaseResource)
    {
        return;
    }
    
    Release();
}
    
bool SqliteDataResource::Acquire()
{
    int32 ReturnCode = sqlite3_open(TCHAR_TO_UTF8(*DatabaseFileLocation), &DatabaseResource);
    
    if(ReturnCode != SQLITE_OK)
    {
        UE_LOG(LogDataAccess, Error, TEXT("Acquire: Cannot open database at %s with error %s"), *DatabaseFileLocation, UTF8_TO_TCHAR(sqlite3_errmsg(DatabaseResource)));
        sqlite3_close(DatabaseResource);
        return false;
    }
    
    return true;
}

bool SqliteDataResource::Release()
{
    if(!DatabaseResource)
    {
        return true;
    }
    
    sqlite3_close(DatabaseResource);
    DatabaseResource = nullptr;
    return true;
}

sqlite3* SqliteDataResource::Get() const
{
    return DatabaseResource;
}