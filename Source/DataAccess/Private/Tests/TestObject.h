// Copyright 2015 afuzzyllama. All Rights Reserved.

#pragma once

#include "TestObject.generated.h"
/* Sqlite:
CREATE TABLE TestObject ( Id INTEGER PRIMARY KEY AUTOINCREMENT, TestInt INTEGER, TestFloat REAL, TestBool NUMERIC, TestString TEXT, TestArray BLOB, CreateTimestamp INTEGER, LastUpdateTimestamp INTEGER );
CREATE TRIGGER TestObject_Insert AFTER INSERT ON TestObject BEGIN UPDATE TestObject SET CreateTimestamp = strftime('%s','now'), LastUpdateTimestamp = strftime('%s','now') WHERE Id = new.Id; END;
CREATE TRIGGER TestObject_Update AFTER UPDATE ON TestObject FOR EACH ROW BEGIN UPDATE TestObject SET LastUpdateTimestamp = strftime('%s','now') WHERE Id = new.Id; END;
*/

UCLASS()
class UTestObject : public UObject
{
    GENERATED_UCLASS_BODY()
    
private:
    UPROPERTY()
    int32 Id;
    
    UPROPERTY()
    int32 TestInt;
    
    UPROPERTY()
    float TestFloat;
    
    UPROPERTY()
    bool TestBool;
    
    UPROPERTY()
    FString TestString;
    
    UPROPERTY()
    TArray<int32> TestArray;
    
    UPROPERTY()
    int32 CreateTimestamp;
    
    UPROPERTY()
    int32 LastUpdateTimestamp;
    
    friend class FSqliteDataAccessTest;
};