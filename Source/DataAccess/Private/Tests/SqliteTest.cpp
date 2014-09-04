// Copyright 2014 afuzzyllama. All Rights Reserved.

#if WITH_SQLITE

#include "DataAccessPrivatePCH.h"
#include "AutomationTest.h"
#include "SqliteDataResource.h"
#include "SqliteDataHandler.h"
#include "TestObject.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSqliteDataAccessTest, "DataAccess.Sqlite", EAutomationTestFlags::ATF_ApplicationMask)

bool FSqliteDataAccessTest::RunTest(const FString& Parameters)
{
    AddLogItem(TEXT("Creating SqliteDataResource"));
    if(!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FString(FPaths::GameDir() + "/Data/Test.db")))
    {
        AddError(TEXT("Test database does not exist at \"" + FPaths::GameDir() + "/Data/Test.db\""));
        return false;
    }
    
    TSharedPtr<SqliteDataResource> DataResource = MakeShareable(new SqliteDataResource(FString(FPaths::GameDir() + "/Data/Test.db")));
    if(!DataResource->Acquire())
    {
        AddError(TEXT("Test database resource could not be acquired"));
        return false;
    }
    AddLogItem(TEXT("Successfully created SqliteDataResource"));
    
    
    AddLogItem(TEXT("Creating SqliteDataHandler"));
    TSharedPtr<IDataHandler> DataHandler = MakeShareable(new SqliteDataHandler(DataResource));
    AddLogItem(TEXT("Successfully created SqliteDataHandler"));
    
    
    AddLogItem(TEXT("Creating test object"));
    UTestObject* TestObj = nullptr;
    TestObj = NewObject<UTestObject>();
    TestObj->TestInt = 42;
    TestObj->TestFloat = 42.f;
    TestObj->TestBool = true;
    TestObj->TestString = "Test String";
    TestObj->TestArray.Add(42);
    
    if(!DataHandler->Source(UTestObject::StaticClass()).Create(TestObj))
    {
        AddError(TEXT("Error creating a new record"));
        return false;
    }
    
    if(TestObj->Id == -1)
    {
        AddError(TEXT("TestObj not correctly set after creating record"));
        return false;
    }
    AddLogItem(TEXT("Successfully created test object"));
    
    
    AddLogItem(TEXT("Reading created test object"));
    UTestObject* TestObj2 = nullptr;
    TestObj2 = NewObject<UTestObject>();
    TestObj2->TestInt = 0;
    TestObj2->TestFloat = 0.f;
    TestObj2->TestBool = false;
    TestObj2->TestString = "";
    TestObj2->TestArray.Empty();
    if(!DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, FString::FromInt(TestObj->Id)).First(TestObj2))
    {
        AddError(TEXT("Error reading created record"));
        return false;
    }
    
    if(!(TestObj->TestInt           ==  TestObj2->TestInt           &&
         TestObj->TestFloat         ==  TestObj2->TestFloat         &&
         TestObj->TestBool          ==  TestObj2->TestBool          &&
         TestObj->TestString        ==  TestObj2->TestString        &&
         TestObj->TestArray.Num()   ==  TestObj2->TestArray.Num()   &&
         TestObj->TestArray[0]      ==  TestObj2->TestArray[0]))
    {
        AddError(TEXT("Created object and read object do not match"));
        return false;
    }
    AddLogItem(TEXT("Read test object matched returned test object"));
    AddLogItem(TEXT("Successfully created and read test object"));
    
    AddLogItem(TEXT("Updating test object"));
    TestObj->TestInt = 43;
    TestObj->TestFloat = 43.f;
    TestObj->TestBool = false;
    TestObj->TestString = "Another Test String";
    TestObj->TestArray.Add(43);
    if(!DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, FString::FromInt(TestObj->Id)).Update(TestObj))
    {
        AddError(TEXT("Error updating record"));
        return false;
    }
    
    TestObj2 = NewObject<UTestObject>();
    TestObj2->TestInt = 0;
    TestObj2->TestFloat = 0.f;
    TestObj2->TestBool = false;
    TestObj2->TestString = "";
    TestObj2->TestArray.Empty();
    if(!DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, FString::FromInt(TestObj->Id)).First(TestObj2))
    {
        AddError(TEXT("Error reading updated record"));
        return false;
    }
    
    if(!(TestObj->TestInt           ==  TestObj2->TestInt           &&
         TestObj->TestFloat         ==  TestObj2->TestFloat         &&
         TestObj->TestBool          ==  TestObj2->TestBool          &&
         TestObj->TestString        ==  TestObj2->TestString        &&
         TestObj->TestArray.Num()   ==  TestObj2->TestArray.Num()   &&
         TestObj->TestArray[0]      ==  TestObj2->TestArray[0]      &&
         TestObj->TestArray[1]      ==  TestObj2->TestArray[1]))
    {
        AddError(TEXT("Updated object and read object do not match"));
        return false;
    }
    AddLogItem(TEXT("Updated test object matched return test object"));
    AddLogItem(TEXT("Successfully updated and read test object"));
    
    // Adding second record
    if(!DataHandler->Source(UTestObject::StaticClass()).Create(TestObj2))
    {
        AddError(TEXT("Error creating a second record"));
        return false;
    }
    
    
    AddLogItem(TEXT("Getting count"));
    
    int32 Count;
    if(!DataHandler->Source(UTestObject::StaticClass()).Count(Count))
    {
        AddError(TEXT("Error getting count"));
        return false;
    }

    if(Count != 2)
    {
        AddError(TEXT("Count incorrect"));
    }
    AddLogItem(TEXT("Successfully got count"));
    
    
    AddLogItem(TEXT("Getting all test object"));
    TArray<UObject*> ReturnedObjects;
    for(int32 i = 0; i < Count; ++i)
    {
        ReturnedObjects.Add(NewObject<UTestObject>());
    }
    if(!DataHandler->Source(UTestObject::StaticClass()).Get(ReturnedObjects))
    {
        AddError(TEXT("Error fetching all records"));
        return false;
    }

    if(ReturnedObjects.Num() != 2)
    {
        AddError(TEXT("Error getting all test object"));
    }
    AddLogItem(TEXT("Successfully got all test object"));

    
    AddLogItem(TEXT("Deleting test object"));
    if(!DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, FString::FromInt(TestObj->Id)).Delete())
    {
        AddError(TEXT("Error deleting record"));
        return false;
    }
    AddLogItem(TEXT("Successfully deleted test object"));
    

    AddLogItem(TEXT("Testing expected fail cases"));
    TestObj = NewObject<UTestObject>();
    if(DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, "-1").First(TestObj))
    {
        AddError(TEXT("Read success, but object doesn't exist"));
        return false;
    }
    
    if(DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, "-1").Update(TestObj))
    {
        AddError(TEXT("Update success, but object doesn't exist"));
        return false;
    }
    
    if(DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, "-1").Delete())
    {
        AddError(TEXT("Delete success, but object doesn't exist"));
        return false;
    }
    
    AddLogItem(TEXT("Deleting all test objects"));
    if(!DataHandler->Source(UTestObject::StaticClass()).Delete())
    {
        AddError(TEXT("Problems cleaning up"));
        return false;
    }
    AddLogItem(TEXT("Successfully tested fail cases"));
    
    TestObj->ConditionalBeginDestroy();
    TestObj = nullptr;
    TestObj2->ConditionalBeginDestroy();
    TestObj2 = nullptr;
    
    return true;
}

#endif