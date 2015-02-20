// Copyright 2015 afuzzyllama. All Rights Reserved.

#include "DataAccessPrivatePCH.h"

DEFINE_LOG_CATEGORY(LogDataAccess);

class FDataAccess : public IDataAccess
{
	/** IModuleInterface implementation */
	virtual void StartupModule();
	virtual void ShutdownModule();
};

IMPLEMENT_MODULE( FDataAccess, DataAccess )

void FDataAccess::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FDataAccess::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}
