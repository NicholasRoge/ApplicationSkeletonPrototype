#pragma comment(lib,"comctl32.lib")

#include "Application.h"
#include <CommCtrl.h>
#include "./Resources/Resources.h"
#include <utility>

#include <string>


std::string wstos(const std::wstring& wstring)
{
	char* buffer;
	size_t buffer_size = wstring.length() + 1;
	std::string string;


	buffer = new char[buffer_size];
	buffer[buffer_size - 1] = 0;
	wcstombs(buffer,wstring.c_str(),buffer_size);
	string = buffer;
	delete[] buffer;

	return string;
}

namespace Application
{
	int exit_code;
	std::vector<OS::WindowClass*> loaded_classes;
	OS::Module* module;
	OS::Window* window;
	OS::Window* button;

	/* Function Definitions */
	void Execute()
	{
		window->show(SW_SHOWNORMAL);
		
		exit_code = OS::StartMessageLoop();
	}

	int GetExitCode()
	{
		return exit_code;
	}

	OS::Module& GetModule()
	{
		return *Application::module;
	}

	void Load()
	{
		OS::Module& module = Application::GetModule();

		
		InitCommonControls();

		/* Register the window class(es). */
		{
			OS::WindowOnCloseCallback class_on_close = Application::GetModule().getProcedure<OS::WindowOnCloseCallbackSignature>(wstos(module.getStringResource(Application_UIClass_Window_OnClose)).c_str());
			OS::WindowClass* window_class;


			window_class = OS::WindowClass::Register(module.getStringResource(Application_UIClass_Window_Name),module);
			window_class->setWindowDefaults(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,WS_EX_APPWINDOW,0,0,0,0);
			window_class->extendDefaultMessageHandler(WM_CLOSE,[class_on_close](OS::Window* window,WPARAM w_param,LPARAM l_param){
				class_on_close(*window);
			});

			Application::loaded_classes.push_back(window_class);
		}

		{
			OS::WindowClass* window_class;


			window_class = OS::WindowClass::GetByName(module.getStringResource(Application_UIClass_Button_Name));
			window_class->setWindowDefaults(WS_TABSTOP | WS_CHILD | BS_PUSHBUTTON,0,0,0,0,0);

			Application::loaded_classes.push_back(window_class);
		}

		/* Set up the window(s). */
		{
			OS::WindowOnCreateCallback on_create = module.getProcedure<OS::WindowOnCreateCallbackSignature>(wstos(module.getStringResource(Application_MainWindow_OnCreate)).c_str());


			window = OS::WindowClass::GetByName(module.getStringResource(Application_MainWindow_Class),module)->instantiate();
			on_create(*window);
		}

		{
			OS::WindowOnClickCallback on_click = module.getProcedure<OS::WindowOnClickCallbackSignature>(wstos(module.getStringResource(Application_MadButton_OnClick)).c_str());
			OS::WindowOnCreateCallback on_create = module.getProcedure<OS::WindowOnCreateCallbackSignature>(wstos(module.getStringResource(Application_MadButton_OnCreate)).c_str());
			OS::WindowOnDestroyCallback on_destroy = module.getProcedure<OS::WindowOnDestroyCallbackSignature>(wstos(module.getStringResource(Application_MadButton_OnDestroy)).c_str());

			button = OS::WindowClass::GetByName(module.getStringResource(Application_MadButton_Class),module)->instantiate();
			button->extendMessageHandler(WM_LBUTTONUP,[on_click](OS::Window* window,WPARAM w_param,LPARAM l_param){
				on_click(*window);
			});
			button->extendMessageHandler(WM_DESTROY,[on_destroy](OS::Window* window,WPARAM w_param,LPARAM l_param){
				on_destroy(*window);
			});
			button->setParent(window);
			on_create(*button);
		}
	}

	void Unload()
	{
		for(OS::WindowClass* window_class : Application::loaded_classes)
		{
			OS::WindowClass::Unregister(window_class);
		}
	}
}