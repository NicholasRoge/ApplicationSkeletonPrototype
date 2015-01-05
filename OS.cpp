#include "OS.h"

#include <cwctype>
#include <map>
#include <string>

using OS::RuntimeException;
using OS::Window;
using OS::WindowClass;

namespace OS
{
	std::map<std::wstring,WindowClass*> window_class_by_name;

	struct WindowPropertyCache
	{
		DWORD style;
		DWORD style_extended;
	};

	/* Constants */
	const wchar* WINDOW_INSTANCE_PROPERTY = L"Window.Instance";

	/* Function Definitions */
	void DisplayErrorMessage()
	{
		DisplayErrorMessage(GetLastError());
	}

	void DisplayErrorMessage(DWORD error)
	{
		wchar* buffer;
		std::wstring error_message;


		buffer = nullptr;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,error,MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),(wchar*)&buffer,0,nullptr);
		error_message
			.append(L"Error Code:\n")
			.append(std::to_wstring(error))
			.append(L"\n\nMessage:\n")
			.append(buffer);
		LocalFree(buffer);

		MessageBox(nullptr,error_message.c_str(),L"An Error Has Occured",MB_OK | MB_ICONERROR);
	}

	int StartMessageLoop()
	{
		MSG message;


		while(GetMessage(&message,nullptr,0,0) > 0)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		return message.wParam;
	}

	void StopMessageLoop(int exit_code)
	{
		PostQuitMessage(exit_code);
	}

	/* Type [OS::RuntimeException] Definition */
	RuntimeException::RuntimeException()
	: RuntimeException("A runtime exception has occured.  Use the \"cause\" method to obtain additional information.",GetLastError())
	{
	}

	RuntimeException::RuntimeException(const char* information)
	: RuntimeException(information,GetLastError())
	{
	}
	
	RuntimeException::RuntimeException(const std::string& information)
	: RuntimeException(information.c_str(),GetLastError())
	{
	}
	
	RuntimeException::RuntimeException(const char* information,DWORD cause)
	: std::runtime_error(information)
	{
		this->m_cause = cause;
	}
	
	RuntimeException::RuntimeException(const std::string& information,DWORD cause)
	: RuntimeException(information.c_str(),cause)
	{
	}

	DWORD RuntimeException::cause() const
	{
		return this->m_cause;
	}

	/* Type [OS::Module] Definition */
	Module::Module(HINSTANCE module)
	{
		//assert();


		this->module_handle = module;
	}

	Module Module::GetCurrent()
	{
		return Module(GetModuleHandle(nullptr));
	}

	void* Module::getResource(WORD resource_id,const wchar* resource_type,WORD language)
	{
		return LockResource(LoadResource(*this,this->getResourceLocation(resource_id,resource_type,language)));
	}

	HRSRC Module::getResourceLocation(WORD resource_id,const wchar* resource_type,WORD language)
	{
		HRSRC resource_location = FindResourceEx(*this,resource_type,MAKEINTRESOURCE(resource_id),language);


		if(resource_location == nullptr)
		{
			if(IsDebuggerPresent())
			{
				OutputDebugString(std::wstring(L"Failed to locate resource with id=").append(std::to_wstring(resource_id)).append(L".\n").c_str());
				DisplayErrorMessage();
			}

			throw OS::RuntimeException();
		}

		return resource_location;
	}

	DWORD Module::getResourceSize(WORD resource_id,const wchar* resource_type,WORD language)
	{
		DWORD resource_size = SizeofResource(*this,this->getResourceLocation(resource_id,resource_type,language));


		if(resource_size == 0)
		{
			if(IsDebuggerPresent())
			{
				DisplayErrorMessage();
			}

			throw OS::RuntimeException();
		}

		return resource_size;
	}

	std::wstring Module::getStringResource(WORD resource_id)
	{
		wchar* buffer;
		std::wstring resource;


		if(LoadString(*this,resource_id,(wchar*)&buffer,0) == 0)
		{
			if(IsDebuggerPresent())
			{
				DisplayErrorMessage();
			}

			throw OS::RuntimeException(std::string("The requested resource does not exist.").c_str());
		}

		//TODO:  The following is an ugly quickfix for the fact that strings are having random control characters appended to them where there should be a null character
		resource = buffer;
		if(resource.find_first_of(L"\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\xD\xE\xF\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F") != std::wstring::npos)
		{
			return resource.substr(0,resource.find_first_of(L"\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\xD\xE\xF\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"));
		}
		else
		{
			return buffer;
		}
	}

	DWORD Module::getStringResourceSize(WORD resource_id)
	{
		wchar* buffer;


		return LoadString(*this,resource_id,(wchar*)&buffer,0);
	}

	Module::operator HINSTANCE&()
	{
		return this->module_handle;
	}

	/* Type [OS::Window] Definition */
	Window::Window(HWND window_handle,WindowClass* window_class)
	: module((HINSTANCE)GetWindowLongPtr(window_handle,GWLP_HINSTANCE))
	{
		assert(IsWindow(window_handle));


		this->window_handle = window_handle;
		this->window_class = window_class;
	}

	void Window::addExtendedStyle(DWORD style)
	{
		this->setExtendedStyle(this->getExtendedStyle() | style);
	}

	void Window::addStyle(DWORD style)
	{
		this->setStyle(this->getStyle() | style);
	}

	HDC Window::beginPaint(PAINTSTRUCT& paintstruct)
	{
		return BeginPaint(this->getNativeHandle(),&paintstruct);
	}

	void Window::destroy()
	{
		if(this->isAlive())
		{
			if(!DestroyWindow(this->window_handle))
			{
				DisplayErrorMessage();
			}
		}
	}

	void Window::endPaint(PAINTSTRUCT& paintstruct)
	{
		EndPaint(this->getNativeHandle(),&paintstruct);
	}

	void Window::extendMessageHandler(UINT message,ExtendingMessageHandler handler)
	{
		assert(handler);


		MessageHandler previous_handler = this->getMessageHandler(message);


		this->setMessageHandler(message,[handler,previous_handler](Window* window,WPARAM w_param,LPARAM l_param){
			LRESULT result;


			result = previous_handler(window,w_param,l_param);
			handler(window,w_param,l_param);

			return result;
		});
	}

	Window* Window::FromHandle(HWND window_handle)
	{
		Window* window;


		window = (Window*)GetProp(window_handle,WINDOW_INSTANCE_PROPERTY);
		if(window == nullptr)
		{
			window = WindowClass::GetByWindowHandle(window_handle)->manage(window_handle);
			if(window->getName() == L"button#prototype")
			{
				DebugBreak();
			}
		}

		return window;
	}

	HBRUSH Window::getBackground()
	{
		if(this->properties.background == nullptr)
		{
			return this->window_class->getBackground();
		}
		else
		{
			return this->properties.background;
		}
	}

	Window* Window::getChildByLocation(LONG x,LONG y,UINT flags)
	{
		POINT point = {x,y};
		HWND child_handle = ChildWindowFromPointEx(this->window_handle,point,flags);


		if(child_handle == nullptr)
		{ 
			return nullptr;
		}
		else
		{
			return WindowClass::GetByWindowHandle(child_handle)->manage(child_handle);
		}
	}

	DWORD Window::getExtendedStyle()
	{
		return GetWindowLongPtr(this->getNativeHandle(),GWL_EXSTYLE);
	}

	int Window::getHeight()
	{
		RECT rectangle = this->getRectangle();


		return rectangle.bottom - rectangle.top;
	}

	int Window::getIdentifier()
	{
		return GetWindowLongPtr(this->getNativeHandle(),GWLP_ID);
	}

	MessageHandler Window::getMessageHandler(UINT message)
	{
		if(this->message_handlers.count(message) > 0)
		{
			return this->message_handlers[message];
		}
		else
		{
			return this->getWindowClass()->getDefaultMessageHandler(message);
		}
	}

	Module& Window::getModule()
	{
		return this->module;
	}

	std::wstring Window::getName()
	{
		std::wstring window_text;
		int window_text_length;


		window_text_length = GetWindowTextLength(this->getNativeHandle());
		if(window_text_length > 0)
		{
			wchar* buffer;


			buffer = new wchar[window_text_length + 1];
			buffer[window_text_length] = 0;
			GetWindowText(this->getNativeHandle(),buffer,window_text_length + 1);
			window_text = buffer;
			delete[] buffer;
		}

		return window_text;
	}

	HWND Window::getNativeHandle() const
	{
		return this->window_handle;
	}

	Window* Window::getOwner()
	{
		HWND window_handle = GetWindow(this->getNativeHandle(),GW_OWNER);


		return window_handle == nullptr ? nullptr : Window::FromHandle(window_handle);
	}

	Window* Window::getParent()
	{
		HWND parent = GetAncestor(this->getNativeHandle(),GA_PARENT);


		if(parent == this->getWindowClass()->prototype->getNativeHandle())
		{
			return nullptr;
		}
		else
		{
			return parent == nullptr ? nullptr : Window::FromHandle(parent);
		}
	}

	HANDLE Window::getProperty(const wchar* property_name)
	{
		return GetProp(this->getNativeHandle(),property_name);
	}

	HANDLE Window::getProperty(const std::wstring& property_name)
	{
		return this->getProperty(property_name.c_str());
	}

	RECT Window::getRectangle(bool client_area)
	{
		RECT rectangle;


		if(client_area)
		{
			GetClientRect(this->getNativeHandle(),&rectangle);
		}
		else
		{
			GetWindowRect(this->getNativeHandle(),&rectangle);
		}

		return rectangle;
	}

	DWORD Window::getStyle()
	{
		return GetWindowLongPtr(this->getNativeHandle(),GWL_STYLE);
	}

	int Window::getWidth()
	{
		RECT rectangle = this->getRectangle();


		return rectangle.right - rectangle.left;
	}

	WindowClass* Window::getWindowClass() const
	{
		return this->window_class;
	}

	int Window::getXCoordinate(bool relative)
	{
		RECT rectangle = this->getRectangle();


		if(relative)
		{
			POINT point;


			point.x = rectangle.left;
			point.y = rectangle.top;

			ScreenToClient(this->getNativeHandle(),&point);

			return point.x;
		}
		else
		{
			return rectangle.left;
		}
	}

	int Window::getYCoordinate(bool relative)
	{
		RECT rectangle = this->getRectangle();


		if(relative)
		{
			POINT point;


			point.x = rectangle.left;
			point.y = rectangle.top;

			ScreenToClient(this->getNativeHandle(),&point);

			return point.y;
		}
		else
		{
			return rectangle.top;
		}
	}

	LRESULT Window::HandleMessage(HWND window_handle,UINT message,WPARAM w_param,LPARAM l_param)
	{
		LRESULT result;
		Window* window = Window::FromHandle(window_handle);


		switch(message)
		{
			case WM_NCCREATE:
				//this->init();

				break;
		}

		result = window->getMessageHandler(message)(window,w_param,l_param);

		switch(message)
		{
			case WM_NCDESTROY:
				window->removeProperty(WINDOW_INSTANCE_PROPERTY);
				window->window_handle = nullptr;

				break;
		}

		return result;
	}

	bool Window::hasParent()
	{
		return this->getParent() != nullptr;
	}

	bool Window::isAlive()
	{
		return this->getNativeHandle() != nullptr && IsWindow(this->getNativeHandle());
	}

	bool Window::isTopLevel()
	{
		return !this->hasParent();
	}

	bool Window::isVisible()
	{
		return IsWindowVisible(this->getNativeHandle()) == TRUE;
	}

	void Window::maximize(bool animate)
	{
		//TODO:  Perform animation

		ShowWindow(this->getNativeHandle(),SW_MAXIMIZE);
	}

	void Window::minimize(bool animate)
	{
		//TODO:  Perform animation

		ShowWindow(this->getNativeHandle(),SW_MINIMIZE);
	}

	void Window::removeExtendedStyle(DWORD style)
	{
		this->setExtendedStyle(this->getExtendedStyle() & ~style);
	}
	
	void Window::removeProperty(const wchar* property_name)
	{
		RemoveProp(this->getNativeHandle(),property_name);
	}

	void Window::removeProperty(const std::wstring& property_name)
	{
		this->removeProperty(property_name.c_str());
	}

	void Window::removeStyle(DWORD style)
	{
		this->setStyle(this->getStyle() & ~style);
	}

	void Window::restore(bool animate)
	{
		//TODO:  Perform animation

		ShowWindow(this->getNativeHandle(),SW_RESTORE);
	}

	void Window::setBackground(HBRUSH background)
	{
		this->properties.background = background;
		
		RedrawWindow(this->getNativeHandle(),nullptr,nullptr,RDW_ERASE | RDW_INVALIDATE);
	}

	void Window::setDimensions(int width,int height)
	{
		SetWindowPos(this->getNativeHandle(),nullptr,0,0,width,height,SWP_NOMOVE | SWP_NOZORDER);
	}

	void Window::setExtendedStyle(DWORD style)
	{
		SetWindowLongPtr(this->getNativeHandle(),GWL_EXSTYLE,style);
	}

	void Window::setMessageHandler(UINT message,MessageHandler handler)
	{
		assert(handler);


		this->message_handlers[message] = handler;
	}

	void Window::setName(const wchar* window_name)
	{
		assert(window_name != nullptr);
		

		SetWindowText(this->getNativeHandle(),window_name);
	}

	void Window::setName(const std::wstring& window_name)
	{
		this->setName(window_name.c_str());
	}

	void Window::setOwner(Window* parent)
	{
		assert(parent != nullptr);


		//SetWindowLongPtr(this->getNativeHandle(),GWLP_HWNDPARENT,(LONG)parent->getNativeHandle());
		//TODO:  This method does not change the owner.  
	}

	void Window::setParent(Window* parent,bool alter_visibility)
	{
		if(this->isTopLevel() && parent == nullptr) //If the window is already a desktop window, calling this method with a null parent has no effect.
		{
			return;
		}

		if(parent == nullptr)
		{
			SetParent(this->getNativeHandle(),nullptr);

			this->removeStyle(WS_CHILD);
			this->addStyle(WS_POPUP);
			if(alter_visibility)
			{
				this->setVisible(false);
			}
		}
		else
		{
			if(this->isTopLevel()) //Only modify the window's styles if the window was not already a child window.
			{
				this->removeStyle(WS_POPUP);
				this->addStyle(WS_CHILD);
				if(alter_visibility)
				{
					this->setVisible(true);
				}
			}

			SetParent(this->getNativeHandle(),parent->getNativeHandle());
		}
		//TODO:  Update window UI states?
	}

	void Window::setPosition(int x,int y,bool relative)
	{
		if(relative)
		{
			MoveWindow(this->getNativeHandle(),x,y,this->getWidth(),this->getHeight(),true);
		}
		else
		{
			SetWindowPos(this->getNativeHandle(),nullptr,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER);
		}
	}

	void Window::setProperty(const wchar* property_name,HANDLE value)
	{
		SetProp(this->getNativeHandle(),property_name,value);
	}

	void Window::setProperty(const std::wstring& property_name,HANDLE value)
	{
		this->setProperty(property_name.c_str(),value);
	}

	void Window::setStyle(DWORD style)
	{
		SetWindowLongPtr(this->getNativeHandle(),GWL_STYLE,style);
	}

	void Window::setVisible(bool visible)
	{
		if(visible)
		{
			this->addStyle(WS_VISIBLE);
		}
		else
		{
			this->removeStyle(WS_VISIBLE);
		}
	}

	void Window::show(int show_command)
	{
		ShowWindow(this->getNativeHandle(),show_command);
	}

	void Window::unsetMessageHandler(UINT message)
	{
		this->message_handlers.erase(message);
	}

	/* Type [OS::WindowClass] Definition */
	WindowClass::WindowClass(const wchar* class_name,HINSTANCE context)
	{
		assert(class_name != nullptr);
		assert(WindowClass::IsValidClassName(class_name));


		lstrcpy(this->class_name,class_name);
		this->context = context;

		if(!WindowClass::Exists(class_name,context))
		{
			WNDCLASSEX data;


			data.cbSize = sizeof(data);
			data.style = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;
			data.lpfnWndProc = DefWindowProc;
			data.cbClsExtra = 0;
			data.cbWndExtra = 0;
			data.hInstance = context;
			data.hIcon = LoadIcon(nullptr,IDI_APPLICATION);
			data.hCursor = LoadCursor(nullptr,IDC_ARROW);
			data.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			data.lpszMenuName = nullptr;
			data.lpszClassName = this->class_name;
			data.hIconSm = LoadIcon(nullptr,IDI_APPLICATION);

			if(!RegisterClassEx(&data))
			{
				throw OS::RuntimeException("Failed to register window class.");
			}
		}

		this->createPrototype();
		this->default_window_procedure = (WNDPROC)GetClassLongPtr(this->prototype->getNativeHandle(),GCLP_WNDPROC);
		SetClassLongPtr(this->prototype->getNativeHandle(),GCLP_WNDPROC,(LONG)(WNDPROC)[](HWND window_handle,UINT message,WPARAM w_param,LPARAM l_param){
			SetWindowLongPtr(window_handle,GWLP_WNDPROC,(LONG)Window::HandleMessage);
			return Window::HandleMessage(window_handle,message,w_param,l_param);
		});
		this->setDefaultMessageHandlers();
	}

	WindowClass::WindowClass(const std::wstring& window_class,HINSTANCE context)
	: WindowClass(window_class.c_str(),context)
	{
	}

	void WindowClass::createPrototype()
	{
		HWND window_handle;


		window_handle = CreateWindowEx(
			0,  // Extended Style
			this->getClassName(),  // Window class name
			std::wstring(this->getClassName()).append(L"#prototype").c_str(),  // Window name
			0, // Window style
			0, // Window x-coordinate
			0, // Window y-coordinate
			-1, // Window width
			-1, // Window height
			HWND_MESSAGE, // Parent window handle
			nullptr, // Menu handle
			this->getContext(),
			nullptr
		);

		if(window_handle == nullptr)
		{
			DisplayErrorMessage();
			throw OS::RuntimeException("Failed to create prototype window.");
		}
		else
		{
			this->prototype = new Window(window_handle,this);
		}
	}

	bool WindowClass::Exists(const wchar* name,HINSTANCE context)
	{
		assert(name != nullptr);


		WNDCLASSEX window_class;


		return GetClassInfoEx(context,name,&window_class) != FALSE;  //NOTE:  In this case, this is not the same thing as saying "GetClassInfoEx(...) == TRUE"
	}

	bool WindowClass::Exists(const std::wstring& name,HINSTANCE context)
	{
		return WindowClass::Exists(name.c_str(),context);
	}

	void WindowClass::extendDefaultMessageHandler(UINT message,ExtendingMessageHandler handler)
	{
		assert(handler);


		MessageHandler previous_handler = this->getDefaultMessageHandler(message);


		this->setDefaultMessageHandler(message,[handler,previous_handler](Window* window,WPARAM w_param,LPARAM l_param){
			LRESULT result;


			result = previous_handler(window,w_param,l_param);
			handler(window,w_param,l_param);

			return result;
		});
	}

	void WindowClass::forget(Window* window)
	{
		assert(window->getWindowClass() == this);


		this->instantiated_windows.erase(window->getNativeHandle());
	}

	ATOM WindowClass::getAtom() const
	{
		return (ATOM)GetClassLongPtr(this->prototype->getNativeHandle(),GCW_ATOM);
	}

	HBRUSH WindowClass::getBackground() const
	{
		return (HBRUSH)GetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HBRBACKGROUND);
	}

	WindowClass* WindowClass::GetByName(const wchar* class_name,HINSTANCE context,bool create)
	{
		std::wstring class_name_lowercase(class_name);


		for(unsigned offset = 0;offset < class_name_lowercase.length();++offset)
		{
			class_name_lowercase[offset] = std::towlower(class_name_lowercase[offset]);
		}

		if(WindowClass::Exists(class_name_lowercase,context))
		{
			if(window_class_by_name.count(class_name_lowercase) == 0)
			{
				/* In this case, the window class already existed, but was not registered in this module. */
				window_class_by_name[class_name_lowercase] = new WindowClass(class_name_lowercase,context);
			}

			return window_class_by_name[class_name_lowercase];
		}
		
		if(create)
		{
			return WindowClass::Register(class_name_lowercase,context);
		}

		return nullptr;
	}

	WindowClass* WindowClass::GetByName(const std::wstring& name,HINSTANCE context,bool create)
	{
		return WindowClass::GetByName(name.c_str(),context,create);
	}

	WindowClass* WindowClass::GetByWindowHandle(HWND window_handle)
	{
		assert(IsWindow(window_handle));


		wchar class_name[256];


		GetClassName(window_handle,class_name,256);

		return WindowClass::GetByName(class_name,(HINSTANCE)GetClassLongPtr(window_handle,GCLP_HMODULE));
	}

	const wchar* WindowClass::getClassName() const
	{
		return this->class_name;
	}

	HINSTANCE WindowClass::getContext() const
	{
		return this->context;
	}

	HCURSOR WindowClass::getCursor() const
	{
		return (HCURSOR)GetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HCURSOR);
	}

	MessageHandler WindowClass::getDefaultMessageHandler(UINT message)
	{
		if(this->message_handlers.count(message) > 0)
		{
			return this->message_handlers[message];
		}
		else
		{
			return [this,message](Window* window,WPARAM w_param,LPARAM l_param)
			{
				return CallWindowProc(this->default_window_procedure,window->getNativeHandle(),message,w_param,l_param);
			};
		}
	}

	HICON WindowClass::getIcon() const
	{
		return (HICON)GetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HICON);
	}

	HICON WindowClass::getIconSmall() const
	{
		return (HICON)GetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HICONSM);
	}

	const wchar* WindowClass::getMenuName() const
	{
		return (const wchar*)GetClassLongPtr(this->prototype->getNativeHandle(),GCLP_MENUNAME);
	}

	DWORD WindowClass::getStyle() const
	{
		return (DWORD)GetClassLongPtr(this->prototype->getNativeHandle(),GCL_STYLE);
	}

	std::vector<Window*> WindowClass::getWindows()
	{
		std::vector<Window*> windows;  //Not that Windows, the other windows.


		for(auto hwnd_window_pair : this->instantiated_windows)
		{
			windows.push_back(hwnd_window_pair.second);
		}

		return windows;
	}

	Window* WindowClass::instantiate()
	{
		return this->instantiate(L"Untitled Window");
	}

	Window* WindowClass::instantiate(const wchar* window_name)
	{
		HWND window_handle;

		
		window_handle = CreateWindowEx(
			this->prototype->getExtendedStyle(),  // Extended Style
			this->getClassName(),  // Window class name
			window_name,  // Window name
			this->prototype->getStyle(), // Window style
			this->prototype->getXCoordinate(), // Window x-coordinate
			this->prototype->getYCoordinate(), // Window y-coordinate
			this->prototype->getWidth(), // Window width
			this->prototype->getHeight(), // Window height
			this->prototype->getNativeHandle(),//parent == nullptr ? nullptr : parent->getNativeHandle(), // Parent window handle
			0, // Menu handle
			GetModuleHandle(nullptr),
			nullptr
		);

		if(window_handle == nullptr)
		{
			DisplayErrorMessage();
			throw OS::RuntimeException();
		}
		else
		{
			return Window::FromHandle(window_handle);
		}
	}

	Window* WindowClass::instantiate(const std::wstring& window_name)
	{
		return this->instantiate(window_name.c_str());
	}
	
	bool WindowClass::IsValidClassName(const wchar* name)
	{
		assert(name != nullptr);


		unsigned length;


		length = 0;
		for(LPCTSTR c = name;*c != 0 && length < 255;++c,++length);

		return length < 256;
	}

	bool WindowClass::IsValidClassName(const std::wstring& name)
	{
		return WindowClass::IsValidClassName(name.c_str());
	}

	Window* WindowClass::manage(HWND window_handle)
	{
		if(this->instantiated_windows.count(window_handle) == 0) //I don't think this should ever not be the case, but it's here just in case.
		{
			Window* window = new Window(window_handle,this);


			this->instantiated_windows[window_handle] = window;
			SetProp(window_handle,WINDOW_INSTANCE_PROPERTY,(HANDLE)window);
		}

		return this->instantiated_windows[window_handle];
	}

	WindowClass* WindowClass::Register(const wchar* class_name,HINSTANCE context)
	{
		std::wstring class_name_lowercase(class_name);


		for(unsigned offset = 0;offset < class_name_lowercase.length();++offset)
		{
			class_name_lowercase[offset] = std::towlower(class_name_lowercase[offset]);
		}

		if(WindowClass::Exists(class_name_lowercase,context))
		{
			throw OS::RuntimeException("A window class with that name already exists.");
		}
		else
		{
			window_class_by_name[class_name_lowercase] = new WindowClass(class_name_lowercase,context);
			
			return window_class_by_name[class_name_lowercase];
		}
	}

	WindowClass* WindowClass::Register(const std::wstring& class_name,HINSTANCE context)
	{
		return WindowClass::Register(class_name.c_str(),context);
	}

	void WindowClass::setBackground(HBRUSH background)
	{
		SetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HBRBACKGROUND,(LONG)background);

		for(auto& window : this->getWindows())
		{
			UpdateWindow(window->getNativeHandle());
		}
	}

	void WindowClass::setCursor(HCURSOR cursor)
	{
		SetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HCURSOR,(LONG)cursor);
	}

	void WindowClass::setDefaultMessageHandler(UINT message,MessageHandler handler)
	{
		assert(handler);


		this->message_handlers[message] = handler;
	}

	void WindowClass::setDefaultMessageHandlers()
	{
		this->setDefaultMessageHandler(WM_CLOSE,[](Window* window,WPARAM w_param,LPARAM l_param){
			return 0;
		});

		this->setDefaultMessageHandler(WM_ERASEBKGND,[](Window* window,WPARAM w_param,LPARAM l_param){
			HBRUSH background = window->getBackground();
			RECT client_rect = window->getRectangle(true);


			FillRect((HDC)w_param,&client_rect,background);

			return 1;
		});
	}

	void WindowClass::setIcon(HICON icon)
	{
		SetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HICON,(LONG)icon);
	}

	void WindowClass::setIconSmall(HICON icon)
	{
		SetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HICONSM,(LONG)icon);
	}

	void WindowClass::setMenuName(const wchar* menu_name)
	{
		SetClassLongPtr(this->prototype->getNativeHandle(),GCLP_HICON,(LONG)menu_name);
	}

	void WindowClass::setMenuName(const std::wstring& menu_name)
	{
		this->setMenuName(menu_name.c_str());
	}

	void WindowClass::setStyle(DWORD style)
	{
		SetClassLongPtr(this->prototype->getNativeHandle(),GCL_STYLE,(LONG)style);
	}

	void WindowClass::setWindowDefaults(DWORD style,DWORD extended_style,int x,int y,int width,int height)
	{
		if((style & WS_CHILD) == 0)
		{
			style &= ~WS_VISIBLE;  //By default, all top level windows should not be visible.
		}
		else
		{
			style |= WS_VISIBLE;  //Inversely, by default, all child windows should be visible.
		}

		this->prototype->setStyle(style);
		this->prototype->setExtendedStyle(extended_style);
		this->prototype->setPosition(x,y);
		this->prototype->setDimensions(width,height);
	}

	void WindowClass::Unregister(WindowClass*& window_class)
	{
		WindowClass::Unregister(window_class->getClassName(),window_class->getContext());

		window_class = nullptr;
	}

	void WindowClass::Unregister(const wchar* name,HINSTANCE context)
	{
		assert(name != nullptr);


		if(!WindowClass::Exists(name,context))
		{
			return;
		}

		if(window_class_by_name.count(name) > 0)  //It could be the case that the window class wasn't being managed by this API
		{
			WindowClass* window_class = window_class_by_name[name];


			for(Window* window : window_class->getWindows())
			{
				if(window->isAlive())
				{
					window->destroy();
				}
				window_class->forget(window);
				delete window;
			}

			window_class->prototype->destroy();
			delete window_class->prototype;
			window_class->prototype = nullptr;

			window_class_by_name.erase(name);
		}

		UnregisterClass(name,context);
	}

	void WindowClass::Unregister(const std::wstring& name,HINSTANCE context)
	{
		WindowClass::Unregister(name.c_str(),context);
	}

	void WindowClass::unsetDefaultMessageHandler(UINT message)
	{
		this->message_handlers.erase(message);
	}
}