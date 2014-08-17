// Copyright 2014 afuzzyllama. All Rights Reserved.

#pragma once

#include "TestObject.generated.h"
// Sqlite : CREATE TABLE TestObject ( Id INTEGER PRIMARY KEY AUTOINCREMENT, TestInt INTEGER, TestFloat REAL, TestBool NUMERIC, TestString TEXT, TestArray BLOB );
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
    
    friend class FSqliteDataAccessTest;
};