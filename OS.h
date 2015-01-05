#ifndef OS_H
#define OS_H

#include <cassert>
#include <functional>
#include <map>
#include <stdexcept>
#include <vector>
#include <Windows.h>

#define EXPORT extern "C" __declspec(dllexport)


typedef WCHAR wchar;
typedef DWORD dword;
typedef WORD word;


namespace OS
{
	class RuntimeException;
	
	class Window;

	class WindowClass;

	typedef std::function<LRESULT(Window*,WPARAM,LPARAM)> MessageHandler;
	typedef std::function<void(Window*,WPARAM,LPARAM)> ExtendingMessageHandler;

	typedef void(WindowOnClickCallbackSignature)(OS::Window&);
	typedef std::function<WindowOnClickCallbackSignature> WindowOnClickCallback;
	typedef void(WindowOnCloseCallbackSignature)(OS::Window&);
	typedef std::function<WindowOnCloseCallbackSignature> WindowOnCloseCallback;
	typedef void(WindowOnCreateCallbackSignature)(OS::Window&);
	typedef std::function<WindowOnCreateCallbackSignature> WindowOnCreateCallback;
	typedef void(WindowOnDestroyCallbackSignature)(OS::Window&);
	typedef std::function<WindowOnDestroyCallbackSignature> WindowOnDestroyCallback;

	/* Function Prototypes */
	void DisplayErrorMessage();

	void DisplayErrorMessage(DWORD error);

	int StartMessageLoop();
	
	void StopMessageLoop(int exit_code = 0);

	/* Class Prototypes */
	class RuntimeException : public std::runtime_error
	{
		public:
			static void Throw(DWORD error,void* from_method = nullptr,bool is_zero_error = false);

		private:
			DWORD m_cause;

		public:
			RuntimeException();

			RuntimeException(const char* information);

			RuntimeException(const std::string& information);

			RuntimeException(const char* information,DWORD cause);

			RuntimeException(const std::string& information,DWORD cause);

			DWORD cause() const;
	};

	class Module
	{
		public:
			static Module GetCurrent();

		private:
			HINSTANCE module_handle;

		public:
			Module(HINSTANCE module);

			template<typename ProcedureSignature>
			std::function<ProcedureSignature> getProcedure(const char* procedure_name)
			{
				assert(procedure_name != nullptr);

				
				std::function<ProcedureSignature> procedure = reinterpret_cast<ProcedureSignature*>(GetProcAddress(*this,procedure_name));


				if(procedure)
				{
					return procedure;
				}
				else
				{
					if(IsDebuggerPresent())
					{
						OS::DisplayErrorMessage();
					}
					throw OS::RuntimeException(std::string("No procedure with the name \"").append(procedure_name).append("\" exists within the module."));
				}
			}

			void* getResource(WORD resource_id,const wchar* resource_type,WORD language = MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL));

			HRSRC getResourceLocation(WORD resource_id,const wchar* resource_type,WORD language = MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL));

			DWORD getResourceSize(WORD resource_id,const wchar* resource_type,WORD language = MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL));

			std::wstring getStringResource(WORD resource_id);

			DWORD getStringResourceSize(WORD resource_id);

			operator HINSTANCE&();
	};

	class Window
	{
		friend class WindowClass;

		private:
			static LRESULT WINAPI HandleMessage(HWND window_handle,UINT message,WPARAM w_param,LPARAM l_param);

		public:
			static Window* FromHandle(HWND window_handle);

		private:
			struct
			{
				HBRUSH background;
			} properties;

			std::map<UINT,MessageHandler> message_handlers;
			Module module;
			WindowClass* window_class;
			HWND window_handle;

		private:
			Window(HWND window_handle,WindowClass* window_class);

		public:
			void addExtendedStyle(DWORD style);

			void addStyle(DWORD style);

			/**
			 * Causes this window to arrange its minimized children.
			 *
			 * @return Returns a value greater than zero corresponding to the height of one row of minimized children.
			 *
			 * @throw 
			 *   OS::WindowRuntimeException
			 *     Thrown if an error occured while this window was trying to comply with the request.
			 *
			 * @message
			 *   WMDIICONARRANGE
			 *     Sent to this window's message queue to cause it to arrange its minimized children.
			 *
			 * @see ArrangeIconicWindows (http://msdn.microsoft.com/en-us/library/windows/desktop/ms632671(v=vs.85).aspx)
			 */
			UINT arrangeMinimizedChildren();

			HDC beginPaint(PAINTSTRUCT& paint_struct);

			/**
			 * Brings this window to the top of teh z-order stack.  Additionally, this call activates this window if it is a top-level window or its parent if it is not.
			 *
			 * @throw
			 *   OS::WindowRuntimeException
			 *     Thrown if an error occured while this window was trying to comply with the request.
			 *
			 * @see BringWindowToTop (http://msdn.microsoft.com/en-us/library/windows/desktop/ms632673(v=vs.85).aspx)
			 */
			void bringToTop();

			/**
			 * Modifies the User Interface Privilege Isolation (UIPI) message filter foor a specified window.
			 *
			 * @param
			 *   message
			 *     Message whose filter is to be modified.
			 *   action
			 *     Action to take when the message specified by the message parameter is received.  May be one of the following values:
			 *       MSGLFT_ALLOW
			 *       MSGFLT_DISALLOW
			 *       MSGFLT_RESET
			 *
			 * @throw 
			 *   OS::WindowRuntimeException
			 *     Thrown if an error occured while this window was trying to comply with the request.
			 *
			 * @message
			 *   WMDIICONARRANGE
			 *     Sent to this window's message queue to cause it to arrange its iconic (minimized) children.
			 *
			 * @see ChangeWindowMessageFilterEx (http://msdn.microsoft.com/en-us/library/windows/desktop/dd388202(v=vs.85).aspx)
			 */
			void changeMessageFilter(UINT message,DWORD action);

			void destroy();

			void endPaint(PAINTSTRUCT& paint_struct);

			void extendMessageHandler(UINT message,ExtendingMessageHandler handler);

			HBRUSH getBackground();

			/**
			 * Gets the child window that lies underneath the location (x,y).  The given coordinates are relative to this window.
			 *
			 * @param
			 *   x
			 *     X coordinate.
			 *   y
			 *     Y coordinate.
			 *   flags
			 *     Filters which children should be eligible for inclusion by this method.  May be one or more of the following values:
			 *       CWP_ALL
			 *       CWP_SKIPDISABLED
			 *       CWP_SKIPINVISIBLE
			 *       CWP_SKIPTRANSPARENT
			 *
			 * @return
			 *   Returns the Window that is under the specified location.  If the location specified is outside of this window or if there are no children at that point, the window returned by this call will return false on a call to its Window::isValid() method.
			 *
			 * @see GetChildFromPoint (http://msdn.microsoft.com/en-us/library/windows/desktop/ms632676(v=vs.85).aspx)
			 */
			Window* getChildByLocation(LONG x,LONG y,UINT flags = CWP_ALL);

			DWORD getExtendedStyle();

			int getHeight();

			int getIdentifier();

			MessageHandler getMessageHandler(UINT message);

			Module& getModule();

			std::wstring getName();

			HWND getNativeHandle() const;

			Window* getOwner();

			Window* getParent();

			HANDLE getProperty(const wchar* property_name);

			HANDLE getProperty(const std::wstring& property_name);

			RECT getRectangle(bool client_area = false);

			DWORD getStyle();

			int getWidth();

			WindowClass* getWindowClass() const;

			int getXCoordinate(bool relative = true);

			int getYCoordinate(bool relative = true);

			bool hasParent();

			bool isAlive();

			bool isTopLevel();

			bool isVisible();

			void maximize(bool animate = true);

			/**
		  	 * Minimizes this window.
			 *
			 * @throw
			 *   OS::WindowRuntimeException
			 *     Thrown if an error occured while this window was trying to comply with the request.
			 *
			 * @see CloseWindow (http://msdn.microsoft.com/en-us/library/windows/desktop/ms632678(v=vs.85).aspx)
			 */
			void minimize(bool animate = true);

			void removeExtendedStyle(DWORD style);

			void removeProperty(const wchar* property_name);

			void removeProperty(const std::wstring& property_name);

			void removeStyle(DWORD style);

			void restore(bool animate = true);

			void setBackground(HBRUSH background);

			void setDimensions(int width,int height);

			void setExtendedStyle(DWORD style);

			void setMessageHandler(UINT message,MessageHandler handler);
			
			void setName(const wchar* window_name);
			
			void setName(const std::wstring& window_name);

			void setOwner(Window* window);

			void setParent(Window* parent,bool alter_visibility = true);

			void setPosition(int x,int y,bool relative = true);

			void setProperty(const wchar* property_name,HANDLE value);

			void setProperty(const std::wstring& property_name,HANDLE value);

			void setStyle(DWORD style);

			void setVisible(bool visible);

			void show(int show_command = SW_SHOW);

			void unsetMessageHandler(UINT message);
	};

	class WindowClass
	{
		friend class Window;

		public:
			static bool Exists(const wchar* name,HINSTANCE context = nullptr);

			static bool Exists(const std::wstring& name,HINSTANCE context = nullptr);

			static WindowClass* GetByName(const wchar* name,HINSTANCE context = nullptr,bool create = false);

			static WindowClass* GetByName(const std::wstring& name,HINSTANCE context = nullptr,bool create = false);

			static WindowClass* GetByWindowHandle(HWND window_handle);
			
			static bool IsValidClassName(const wchar* name);

			static bool IsValidClassName(const std::wstring& name);

			static WindowClass* Register(const wchar* name,HINSTANCE context = nullptr);

			static WindowClass* Register(const std::wstring& name,HINSTANCE context = nullptr);

			static void Unregister(WindowClass*& window_class);

			static void Unregister(const wchar* name,HINSTANCE context);

			static void Unregister(const std::wstring& name,HINSTANCE context);

		private:
			wchar class_name[256];
			HINSTANCE context;
			std::map<HWND,Window*> instantiated_windows;
			std::map<UINT,MessageHandler> message_handlers;
			Window* prototype;

			WNDPROC default_window_procedure;

		private:
			WindowClass(const wchar* name,HINSTANCE context = nullptr);

			WindowClass(const std::wstring& window_class,HINSTANCE context);

			void createPrototype();

			void forget(Window* window);  //Unmanage?

			Window* manage(HWND window_handle);

			void setDefaultMessageHandlers();

		public:
			void extendDefaultMessageHandler(UINT message,ExtendingMessageHandler handler);

			ATOM getAtom() const;

			HBRUSH getBackground() const;

			const wchar* getClassName() const;

			HINSTANCE getContext() const;

			HCURSOR getCursor() const;

			MessageHandler getDefaultMessageHandler(UINT message);

			HICON getIcon() const;

			HICON getIconSmall() const;

			const wchar* getMenuName() const;

			DWORD getStyle() const;

			std::vector<Window*> getWindows();

			Window* instantiate();

			Window* instantiate(const wchar* window_name);

			Window* instantiate(const std::wstring& window_name);
			
			void setBackground(HBRUSH background);

			void setCursor(HCURSOR cursor);

			void setDefaultMessageHandler(UINT message,MessageHandler handler);

			void setWindowDefaults(DWORD style,DWORD extended_style,int x,int y,int width,int height);

			void setIcon(HICON icon);

			void setIconSmall(HICON icon);

			void setMenuName(const wchar* menu_name);

			void setMenuName(const std::wstring& menu_name);

			void setStyle(DWORD style);

			void unsetDefaultMessageHandler(UINT message);
	};
}

#endif