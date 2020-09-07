#include <DirectXTemplateLibPCH.h>
#include <Application.h>
#include "..\resource.h"

#include <Window.h>

#define WINDOW_CLASS_NAME "DX11RenderWindowClass"

typedef std::map< HWND, Window* > WindowMap;
typedef std::map< std::string, Window* > WindowNameMap;

// An invalid window to return if an error occurred while creating a window.
Window Application::ms_InvalidWindow;

static Application* gs_pSingelton = nullptr;
static WindowMap gs_Windows;
static WindowNameMap gs_WindowByName;

static LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

Application::Application( HINSTANCE hInst )
    : m_hInstance( hInst )
{
    WNDCLASSEX wndClass = {0};

    wndClass.cbSize = sizeof( WNDCLASSEX );
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = &WndProc;
    wndClass.hInstance = m_hInstance;
    wndClass.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wndClass.hIcon = LoadIcon( m_hInstance, MAKEINTRESOURCE(APP_ICON) );
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = WINDOW_CLASS_NAME;

    if ( !RegisterClassEx( &wndClass) )
    {
        MessageBoxA( NULL, "Unable to register the window class.", "Error", MB_OK|MB_ICONERROR );
    }
}

void Application::Create( HINSTANCE hInst )
{
    if ( !gs_pSingelton )
    {
        gs_pSingelton = new Application(hInst);
    }
}

Application& Application::Get()
{
    assert( gs_pSingelton );
    return *gs_pSingelton;
}

void Application::Destroy()
{
    if ( gs_pSingelton )
    {
        delete gs_pSingelton;
        gs_pSingelton = nullptr;
    }
}

Application::~Application()
{
    WindowMap windows = gs_Windows;
    for ( WindowMap::value_type window : windows )
    {
        window.second->Destroy();
    }

    gs_Windows.clear();
    gs_WindowByName.clear();
}

Window& Application::CreateRenderWindow( const std::string& windowName, int clientWidth, int clientHeight, bool vSync, bool windowed )
{
    // First check if a window with the given name already exists.
    WindowNameMap::iterator windowIter = gs_WindowByName.find(windowName);
    if ( windowIter != gs_WindowByName.end() )
    {
        return *(windowIter->second);
    }

    RECT windowRect = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRect( &windowRect, WS_OVERLAPPEDWINDOW, FALSE );

    HWND hWnd = CreateWindowA( WINDOW_CLASS_NAME, windowName.c_str(), 
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
        windowRect.right - windowRect.left, 
        windowRect.bottom - windowRect.top, 
        nullptr, nullptr, m_hInstance, nullptr );

    if ( !hWnd )
    {
        MessageBoxA( NULL, "Could not create the render window.", "Error", MB_OK|MB_ICONERROR );
        return ms_InvalidWindow;
    }

    Window* pWindow = new Window( hWnd, windowName, clientWidth, clientHeight, vSync, windowed );
    gs_Windows.insert( WindowMap::value_type( hWnd, pWindow ) );
    gs_WindowByName.insert( WindowNameMap::value_type( windowName, pWindow) );

    ShowWindow( hWnd, SW_SHOW );
    UpdateWindow( hWnd );

    return *pWindow;
}

void Application::DestroyWindow( Window& window )
{
    window.Destroy();
}

void Application::DestroyWindow( const std::string& windowName )
{
    DestroyWindow( GetWindowByName(windowName) );
}

Window& Application::GetWindowByName( const std::string& windowName )
{
    WindowNameMap::iterator iter = gs_WindowByName.find( windowName );
    if ( iter != gs_WindowByName.end() )
    {
        return *(iter->second);
    }

    // No window found. Return an invalid window handle.
    return ms_InvalidWindow;
}


int Application::Run()
{
    MSG msg = {0};

    static DWORD previousTime = timeGetTime();
    static float totalTime = 0.0f;
    static const float targetFramerate = 30.0f;
    static const float maxTimeStep = 1.0f / targetFramerate;

    while ( msg.message != WM_QUIT )
    {
        if ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            DWORD currentTime = timeGetTime();
            float deltaTime = ( currentTime - previousTime ) / 1000.0f;
            previousTime = currentTime;

            // Cap the delta time to the max time step (useful if your 
            // debugging and you don't want the deltaTime value to explode.
            deltaTime = std::min<float>(deltaTime, maxTimeStep);

            totalTime += deltaTime;

            UpdateEventArgs updateEventArgs( deltaTime, totalTime );
            RenderEventArgs renderEventArgs( deltaTime, totalTime );

            for( WindowMap::value_type window : gs_Windows )
            {
                window.second->OnUpdate( updateEventArgs );
                window.second->OnRender( renderEventArgs );
            }
        }
    }

    return static_cast<int>(msg.wParam);
}

void Application::Quit( int exitCode )
{
    PostQuitMessage( exitCode );
}

// Remove a window from our window lists.
static void RemoveWindow( HWND hWnd )
{
    WindowMap::iterator windowIter = gs_Windows.find(hWnd);
    if ( windowIter != gs_Windows.end() )
    {
        Window* pWindow = windowIter->second;
        gs_WindowByName.erase( pWindow->get_WindowName() );
        gs_Windows.erase( windowIter );
    }
}

// Convert the message ID into a MouseButton ID
MouseButtonEventArgs::MouseButton DecodeMouseButton( UINT messageID )
{
    MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
    switch ( messageID )
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
        {
            mouseButton = MouseButtonEventArgs::Left;
        }
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
        {
            mouseButton = MouseButtonEventArgs::Right;
        }
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
        {
            mouseButton = MouseButtonEventArgs::Middel;
        }
        break;
    }

    return mouseButton;
}

static LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT paintStruct;
    HDC hDC;

    Window* pWindow = nullptr;
    {
        WindowMap::iterator iter = gs_Windows.find(hwnd);
        if ( iter != gs_Windows.end() )
        {
            pWindow = iter->second;
        }
    }

    switch ( message )
    {
    case WM_PAINT:
        {
            hDC = BeginPaint( hwnd, &paintStruct );
            EndPaint( hwnd, &paintStruct );
        }
        break;
    case WM_KEYDOWN:
        {
            MSG charMsg;
            // Get the unicode character (UTF-16)
            unsigned int c = 0;
            // For printable characters, the next message will be WM_CHAR.
            // This message contains the character code we need to send the KeyPressed event.
            // Inspired by the SDL 1.2 implementation.
            if ( PeekMessage(&charMsg, hwnd, 0, 0, PM_NOREMOVE ) && charMsg.message == WM_CHAR )
            {
                GetMessage( &charMsg, hwnd, 0, 0 );
                c = charMsg.wParam;
            }
            bool shift = GetAsyncKeyState(VK_SHIFT) > 0;
            bool control = GetAsyncKeyState(VK_CONTROL) > 0;
            bool alt = GetAsyncKeyState(VK_MENU) > 0;
            KeyCode::Key key = (KeyCode::Key)wParam;
            unsigned int scanCode = ( lParam & 0x00FF0000 ) >> 16;
            KeyEventArgs keyEventArgs( key, c, KeyEventArgs::Pressed, shift, control, alt );
            pWindow->OnKeyPressed( keyEventArgs );
        }
        break;
    case WM_KEYUP:
        {
            bool shift = GetAsyncKeyState(VK_SHIFT) > 0;
            bool control = GetAsyncKeyState(VK_CONTROL) > 0;
            bool alt = GetAsyncKeyState(VK_MENU) > 0;
            KeyCode::Key key = (KeyCode::Key)wParam;
            unsigned int c = 0;
            unsigned int scanCode = ( lParam & 0x00FF0000 ) >> 16;

            // Determine which key was released by converting the key code and the scan code
            // to a printable character (if possible).
            // Inspired by the SDL 1.2 implementation.
            unsigned char keyboardState[256];
            GetKeyboardState( keyboardState );
            wchar_t translatedCharacters[4];
            if ( int result = ToUnicodeEx(wParam, scanCode, keyboardState, translatedCharacters, 4, 0, NULL ) > 0 )
            {
                c = translatedCharacters[0];
            }

            KeyEventArgs keyEventArgs( key, c, KeyEventArgs::Released, shift, control, alt );
            pWindow->OnKeyReleased( keyEventArgs );
        }
        break;
    case WM_MOUSEMOVE:
        {
            bool lButton = ( wParam & MK_LBUTTON ) != 0;
            bool rButton = ( wParam & MK_RBUTTON ) != 0;
            bool mButton = ( wParam & MK_MBUTTON ) != 0;
            bool shift   = ( wParam & MK_SHIFT )   != 0;
            bool control = ( wParam & MK_CONTROL ) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseMotionEventArgs mouseMotionEventArgs( lButton, mButton, rButton, control, shift, x, y );
            pWindow->OnMouseMoved( mouseMotionEventArgs );
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        {
            bool lButton = ( wParam & MK_LBUTTON ) != 0;
            bool rButton = ( wParam & MK_RBUTTON ) != 0;
            bool mButton = ( wParam & MK_MBUTTON ) != 0;
            bool shift   = ( wParam & MK_SHIFT )   != 0;
            bool control = ( wParam & MK_CONTROL ) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseButtonEventArgs mouseButtonEventArgs( DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y );
            pWindow->OnMouseButtonPressed( mouseButtonEventArgs );
        }
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        {
            bool lButton = ( wParam & MK_LBUTTON ) != 0;
            bool rButton = ( wParam & MK_RBUTTON ) != 0;
            bool mButton = ( wParam & MK_MBUTTON ) != 0;
            bool shift   = ( wParam & MK_SHIFT )   != 0;
            bool control = ( wParam & MK_CONTROL ) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseButtonEventArgs mouseButtonEventArgs( DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y );
            pWindow->OnMouseButtonReleased( mouseButtonEventArgs );
        }
        break;
    case WM_MOUSEWHEEL:
        {
            // The distance the mouse wheel is rotated.
            // A positive value indicates the wheel was rotated to the right.
            // A negative value indicates the wheel was rotated to the left.
            float zDelta = ((int)(short)HIWORD(wParam))/(float)WHEEL_DELTA;
            short keyStates = (short)LOWORD(wParam);

            bool lButton = ( keyStates & MK_LBUTTON ) != 0;
            bool rButton = ( keyStates & MK_RBUTTON ) != 0;
            bool mButton = ( keyStates & MK_MBUTTON ) != 0;
            bool shift = ( keyStates & MK_SHIFT )     != 0;
            bool control = ( keyStates & MK_CONTROL ) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            // Convert the screen coordinates to client coordinates.
            POINT clientToScreenPoint;
            clientToScreenPoint.x = x;
            clientToScreenPoint.y = y;
            ScreenToClient( hwnd, &clientToScreenPoint );

            MouseWheelEventArgs mouseWheelEventArgs( zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y );
            pWindow->OnMouseWheel( mouseWheelEventArgs );
        }
        break;
    case WM_SIZE:
        {
            int width = ((int)(short)LOWORD(lParam));
            int height = ((int)(short)HIWORD(lParam));

            ResizeEventArgs resizeEventArgs( width, height );
            pWindow->OnResize( resizeEventArgs );
        }
        break;
    case WM_DESTROY:
        {
            // If a window is being destroyed, remove it from the 
            // window maps.
            RemoveWindow( hwnd );

            if ( gs_Windows.empty() )
            {
                // If there are no more windows, quit the application.
                PostQuitMessage(0);
            }
        }
        break;
    default: 
        return DefWindowProc( hwnd, message, wParam, lParam );
    }

    return 0;
}