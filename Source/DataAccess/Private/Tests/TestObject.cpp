// Copyright 2014 afuzzyllama. All Rights Reserved.

#include "DataAccessPrivatePCH.h"
#include "TestObject.h"

UTestObject::UTestObject(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
    Id = -1;
}
