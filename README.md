DataAccess
==========

Data Access Module for the Unreal Engine which saves UObjects 

Overview
========

While working with Unreal, I wanted to be able to save UObjects to a local database, sqlite, but I didn't want to have to have to write custom code every time I created a new object.  Looking at a modern web framework like the Entity Framework, you can apply attributes to a class that will allow the framework to access meta data about your object and save it.  The team at Epic have built in a sweet reflection interface for developers and I decided to see if I could use it in a similar fashion.

I have no idea if this should be used in production code, so consider this more of a proof of concept :)

The plugin in its current state can save a UObject to an sqlite database if it meets these requirements:

 1.  It uses basic data types or a TArray.
 2.  It contains a member `int32 Id`
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
};
```

That class would have the following table created in sqlite:
```
CREATE TABLE TestObject ( 
  Id INTEGER PRIMARY KEY AUTOINCREMENT, 
  TestInt INTEGER, 
  TestFloat REAL, 
  TestBool NUMERIC, 
  TestString TEXT, 
  TestArray BLOB 
);
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
DataHandler->Create(TestObj);

// Read a record
DataHandler->Read(/**record id*/ 1, TestObj);

// Update a record
TestObj->SomeProperty = "some value";
DataHandler->Update(TestObj);

// Delete a record
DataHandler->Delete(TestObj);

// This shouldn't be necessary since this should be run when the TSharedPtr runs out of references
DataResource->Release();


```


Installation
============

The only implementation that exists at the moment is for sqlite.  Sqlite can be obtained at the [official website](http://www.sqlite.org/).  After obtaining the source files, place the files (shell.c, sqlite3.h, sqlite3.c, sqlite3ext.h) in the following directory:
```
Plugins/DataAccess/Source/DataAccess/ThirdParty/Sqlite/3.8.5/
```

Notes
=====

Here are some interesting points of the project:

- I used the testing framework that is in the Unreal Engine.  See [SqliteTest.cpp](https://github.com/afuzzyllama/DataAccess/blob/master/Source/DataAccess/Private/Tests/SqliteTest.cpp) if you are interested in looking at an example of that.  To run the rest in the editor, add a sqlite database at `$(PROJECT DIR)/Data/Test.db` with the `TestObject` table inside of it.
- TArrays are stored as byte arrays in the database.  In theory this should work with anything you can throw at it, but I haven't tried pushing the limits too hard.
- This has only been tested with sqlite 3.8.5
