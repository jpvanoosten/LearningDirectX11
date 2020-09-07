#include <DirectXTemplateLibPCH.h>
#include <Window.h>
#include <Game.h>

Window::Window()
    : m_hWnd(nullptr)
    , m_ClientWidth( 0 )
    , m_ClientHeight( 0 )
    , m_VSync( true )
    , m_bWindowed( true )
    , m_pGame( nullptr )
{}

Window::Window( HWND hWnd, const std::string& windowName, int clientWidth, int clientHeight, bool vSync, bool windowed )
    : m_hWnd( hWnd )
    , m_WindowName( windowName )
    , m_ClientWidth( clientWidth )
    , m_ClientHeight( clientHeight )
    , m_VSync( vSync )
    , m_bWindowed( windowed )
    , m_pGame( nullptr )
{

}

Window::~Window()
{
    Destroy();
}

HWND Window::get_WindowHandle() const
{
    return m_hWnd;
}

const std::string& Window::get_WindowName() const
{
    return m_WindowName;
}

void Window::Destroy()
{
    if ( m_pGame )
    {
        // Notify the registered DirectX template that the window is being destroyed.
        m_pGame->OnWindowDestroy();
        m_pGame = nullptr;
    }
    if ( m_hWnd )
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

bool Window::IsValid() const
{
    return ( m_hWnd != nullptr );
}

int Window::get_ClientWidth() const
{
    return m_ClientWidth;
}

int Window::get_ClientHeight() const
{
    return m_ClientHeight;
}

bool Window::get_VSync() const
{
    return m_VSync;
}

bool Window::get_Windowed() const
{
    return m_bWindowed;
}

bool Window::RegisterDirectXTemplate( Game* pTemplate )
{
    if ( !m_pGame )
    {
        m_pGame = pTemplate;
        
        return true;
    }

    return false;
}

void Window::OnUpdate( UpdateEventArgs& e )
{
    if ( m_pGame )
    {
        m_pGame->OnUpdate( e );
    }
}

void Window::OnRender( RenderEventArgs& e )
{
    if ( m_pGame )
    {
        m_pGame->OnRender( e );
    }
}

void Window::OnKeyPressed( KeyEventArgs& e )
{
    if ( m_pGame )
    {
        m_pGame->OnKeyPressed( e );
    }
}

void Window::OnKeyReleased( KeyEventArgs& e )
{
    if ( m_pGame )
    {
        m_pGame->OnKeyReleased( e );
    }
}

// The mouse was moved
void Window::OnMouseMoved( MouseMotionEventArgs& e )
{
    if ( m_pGame )
    {
        m_pGame->OnMouseMoved( e );
    }
}

// A button on the mouse was pressed
void Window::OnMouseButtonPressed( MouseButtonEventArgs& e )
{
    if ( m_pGame )
    {
        m_pGame->OnMouseButtonPressed( e );
    }
}

// A button on the mouse was released
void Window::OnMouseButtonReleased( MouseButtonEventArgs& e )
{
    if ( m_pGame )
    {
        m_pGame->OnMouseButtonReleased( e );
    }
}

// The mouse wheel was moved.
void Window::OnMouseWheel( MouseWheelEventArgs& e )
{
    if ( m_pGame )
    {
        m_pGame->OnMouseWheel( e );
    }
}

void Window::OnResize( ResizeEventArgs& e )
{
    // Update the client size.
    if ( e.Width <= 0 ) e.Width = 1;
    if ( e.Height <= 0 ) e.Height = 1;

    m_ClientWidth = e.Width;
    m_ClientHeight = e.Height;

    if ( m_pGame )
    {
        m_pGame->OnResize( e );
    }
}
