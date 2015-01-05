#ifndef APPLICATION_H
#define APPLICATION_H

#include "OS.h"


namespace Application
{
	extern OS::Module* module;

	void Execute();

	int GetExitCode();

	OS::Module& GetModule();

	void Load();

	void Unload();
}

#endif