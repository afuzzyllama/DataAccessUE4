// Copyright 2014 afuzzyllama. All Rights Reserved.

#pragma once

/**
 * Interface that facilities saving data.  Will utilizie Unreal's reflection to save data
 */
class IDataHandler
{
public:
    virtual ~IDataHandler(){}
    
    /**
     * Create a record from passed data
     *
     * @param   Obj     data to create new record from
     * @param   OutId   new id from the created record, returns -1 if there is an error
     * @return          true if successful, false otherwise
     */
    virtual bool Create(UObject* const Obj) = 0;
    
    /**
     * Read a record from passed in id
     *
     * @param Id    id of data to read
     * @param Obj   object to place data into
     * @return      true if successful, false otherwise
     */
    virtual bool Read(int32 Id, UObject* const Obj) = 0;
    
    /**
     * Update a record from passed data
     *
     * @param   Id      id of record to update
     * @param   Obj     data to update record to
     * @return          true if successful, false otherwise
     */
    virtual bool Update(UObject* const Obj) = 0;
    
    /**
     * Delete a record from passed in id
     *
     * @param   Obj Object we want to delete
     * @return      true if successful, false otherwise
     */
    virtual bool Delete(UObject* const Obj) = 0;
     
};