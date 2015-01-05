#include "Application.h"


/* Main */
int WINAPI WinMain(HINSTANCE m,HINSTANCE p,LPSTR command_line,int show_command)
{
	OS::Module application_module(m);
	int exit_code;


	Application::module = new OS::Module(m);

	Application::Load();
	Application::Execute();
	Application::Unload();
	exit_code = Application::GetExitCode();

	delete Application::module;

	return exit_code;
}