DataAccess
==========

Data Access Module for the Unreal Engine which saves UObjects.  Intended to be used with SQLite, but could be modified to support a variety of other databases if the interfaces are implemented correctly. 

Overview
========

While working with Unreal, I wanted to be able to save UObjects to a local database, sqlite, but I didn't want to have to have to write custom code every time I created a new object.  Looking at a modern web framework like the Entity Framework, you can apply attributes to a class that will allow the framework to access meta data about your object and save it.  The team at Epic have built in a sweet reflection interface for developers and I decided to see if I could use it in a similar fashion.

Later I found that having a way to query my data like with Laravel would also be useful, so I threw some code together that mimics that system loosely.

I have no idea if this should be used in production code, so consider this more of a proof of concept :)

The plugin in its current state can save a UObject to an sqlite database if it meets these requirements:

 1.  It uses basic data types or a TArray.
 2.  It contains a members `int32 Id`, `int32 CreateTimestamp`, and `int32 LastUpdateTimestamp`
 3.  The sqlite database used contains a table that matches the class name of the object

Here is an example class:
```
UCLASS()
class UTestObject : public UObject
{
    GENERATED_UCLASS_BODY()
    
public:
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
};
```

That class would have the following table and triggers created in sqlite:
```
CREATE TABLE TestObject ( 
  Id INTEGER PRIMARY KEY AUTOINCREMENT, 
  TestInt INTEGER, 
  TestFloat REAL, 
  TestBool NUMERIC, 
  TestString TEXT, 
  TestArray BLOB,
  CreateTimestamp INTEGER, 
  LastUpdateTimestamp INTEGER
);

CREATE TRIGGER TestObject_Insert 
AFTER INSERT ON TestObject 
BEGIN 
 UPDATE TestObject 
 SET CreateTimestamp = strftime('%s','now'), 
     LastUpdateTimestamp = strftime('%s','now') 
 WHERE Id = new.Id; 
END;

CREATE TRIGGER TestObject_Update 
AFTER UPDATE ON TestObject FOR EACH ROW 
BEGIN 
 UPDATE TestObject 
 SET LastUpdateTimestamp = strftime('%s','now') 
 WHERE Id = new.Id; 
END;
```
Usage
=====

Here is a snippet of how to use the code:
```
TSharedPtr<SqliteDataResource> DataResource = MakeShareable(new SqliteDataResource(FString(FPaths::GameDir() + "/Data/Test.db")));
DataResource->Acquire();
TSharedPtr<IDataHandler> DataHandler = MakeShareable(new SqliteDataHandler(DataResource));

UTestObject* TestObj = NewObject<UTestObject>();

// Create a record
DataHandler->Source(UTestObject::StaticClass()).Create(TestObj);

// Read a record
DatHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, FString::FromInt(TestObj->Id)).First(TestObj);

// Read all records.  Not the most ideal setup, but the array should match the amount of records returned.  
TArray<UObject*> Results;
int32 Count;
DatHandler->Source(UTestObject::StaticClass()).Count(Count);
for(int32 i = 0; i < Count; ++i)
{
	Results.Add(NewObject<UTestObject>());
}
DatHandler->Source(UTestObject::StaticClass()).Get(Results);

// Update a record
TestObj->SomeProperty = "some value";
DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, FString::FromInt(TestObj->Id)).Update(TestObj);

// Delete a record
DataHandler->Source(UTestObject::StaticClass()).Where("Id", EDataHandlerOperator::Equals, FString::FromInt(TestObj->Id)).Delete(TestObj);

// This shouldn't be necessary since this should be run when the TSharedPtr runs out of references
DataResource->Release();


```


Installation
============

Epic has included a way to add SQLiteSupport in the engine. Look in `Engine/Source/ThirdParty/sqlite` for more information on how to include support.  This plugin will not compile without it!

Notes
=====

Here are some interesting points of the project:

- I used the testing framework that is in the Unreal Engine.  See [SqliteTest.cpp](https://github.com/afuzzyllama/DataAccess/blob/master/Source/DataAccess/Private/Tests/SqliteTest.cpp) if you are interested in looking at an example of that.  To run the rest in the editor, add a sqlite database at `$(PROJECT DIR)/Data/Test.db` with the `TestObject` table inside of it.
- TArrays are stored as byte arrays in the database.  In theory this should work with anything you can throw at it, but I haven't tried pushing the limits too hard.
- This has only been slightly tested with sqlite 3.8.6
