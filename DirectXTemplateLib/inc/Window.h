/**
 * @brief A window for our application.
 */
#pragma once

#include <Events.h>

// Forward-declare the DirectXTemplate class.
class Game;

class Window
{
public:

    /**
     * Get a handle to this window's instance.
     * @returns The handle to the window instance or nullptr if this is not a valid window.
     */
    HWND get_WindowHandle() const;

    /**
     * Destroy this window.
     */
    void Destroy();

    /**
     * @brief Check to see if this is a valid window handle.
     * @returns false if this window instance does not refer to a valid window.
     */
    bool IsValid() const;

    const std::string& get_WindowName() const;

    int get_ClientWidth() const;
    int get_ClientHeight() const;

    /**
     * Should this window be rendered with vertical refresh synchronization.
     */
    bool get_VSync() const;

    /**
     * Is this a windowed window or full-screen?
     */
    bool get_Windowed() const;

protected:
    // The Window procedure needs to call protected methods of this class.
    friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Only the application can create a window.
    friend class Application;
    // The DirectXTemplate class needs to register itself with a window.
    friend class Game;

    Window();
    Window( HWND hWnd, const std::string& windowName, int clientWidth, int clientHeight, bool vSync, bool windowed );
    virtual ~Window();

    // Register a DirectXTemplate with this window. This allows
    // the window to callback functions in the DirectXTemplate class and notify 
    // the demo that the window has been destroyed.
    bool RegisterDirectXTemplate( Game* pTemplate );

    // Update and Draw can only be called by the application.
    virtual void OnUpdate( UpdateEventArgs& e );
    virtual void OnRender( RenderEventArgs& e );

    // A keyboard key was pressed
    virtual void OnKeyPressed( KeyEventArgs& e );
    // A keyboard key was released
    virtual void OnKeyReleased( KeyEventArgs& e );

    // The mouse was moved
    virtual void OnMouseMoved( MouseMotionEventArgs& e );
    // A button on the mouse was pressed
    virtual void OnMouseButtonPressed( MouseButtonEventArgs& e );
    // A button on the mouse was released
    virtual void OnMouseButtonReleased( MouseButtonEventArgs& e );
    // The mouse wheel was moved.
    virtual void OnMouseWheel( MouseWheelEventArgs& e );

    // The window was resized.
    virtual void OnResize( ResizeEventArgs& e );

private:
    // Windows should not be copied.
    Window( const Window& copy );

    HWND m_hWnd;

    std::string m_WindowName;
    int m_ClientWidth;
    int m_ClientHeight;
    bool m_VSync;
    bool m_bWindowed;

    Game* m_pGame;
};