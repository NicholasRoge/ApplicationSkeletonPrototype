#include "Application.h"
#include "./Resources/Resources.h"


EXPORT void MainWindow_OnCreate(OS::Window& window)
{
	window.setName(window.getModule().getStringResource(Application_Title));
	window.setPosition(0,0);
	window.setDimensions(345,95);
}

EXPORT void Window_Class_OnClose(OS::Window& window)
{
	OS::StopMessageLoop();

	window.destroy();
}