// Copyright 2014 afuzzyllama. All Rights Reserved.

#pragma once

enum EDataHandlerOperator
{
    GreaterThan,
    LessThan,
    Equals,
    LessThanOrEqualTo,
    GreaterThanOrEqualTo,
    NotEqualTo
};

/**
 * Interface that facilities saving data.  Will utilizie Unreal's reflection to save data
 */
class IDataHandler
{
public:
    virtual ~IDataHandler(){}
    
    virtual IDataHandler& Source(UClass* Source) = 0;

    virtual IDataHandler& Where(FString FieldName, EDataHandlerOperator Operator, FString Condition) = 0;
    virtual IDataHandler& Or() = 0;
    virtual IDataHandler& And() = 0;
    virtual IDataHandler& BeginNested() = 0;
    virtual IDataHandler& EndNested() = 0;

    virtual bool Create(UObject* const Obj) = 0;
    virtual bool Update(UObject* const Obj) = 0;
    virtual bool Delete() = 0;
    virtual bool Count(int32& OutCount) = 0;
    virtual bool First(UObject* const OutObj) = 0;
    virtual bool Get(TArray<UObject*>& OutObjs) = 0;
    
};