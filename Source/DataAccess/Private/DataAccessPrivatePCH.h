// Copyright 2014 afuzzyllama. All Rights Reserved.
#pragma once

#include "IDataAccess.h"
#include "CoreUObject.h"

#if WITH_SQLITE
#include "sqlite3.h"
#endif // WITH_SQLITE

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.

DECLARE_LOG_CATEGORY_EXTERN(LogDataAccess, Log, All);