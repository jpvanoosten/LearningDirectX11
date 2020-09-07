/**
 * The application class is used to create windows for our application.
 */
#pragma once

class Window;

class Application
{
public:

    /**
     * Create the application singleton with the application instance handle.
     */
    static void Create( HINSTANCE hInst );

    /**
     * Destroy the application instance and all windows created by this application instance.
     */
    static void Destroy();
    /**
     * Get the application singleton.
     */
    static Application& Get();

    /**
     * Create a new DirectX11 render window instance.
     * @param windowName The name of the window. This name will appear in the title bar of the window. This name should be unique.
     * @param clientWidth The width (in pixels) of the window's client area.
     * @param clientHeight The height (in pixels) of the window's client area.
     * @param vSync Should the rendering be synchronized with the vertical refresh rate of the screen.
     * @param windowed If true, the window will be created in windowed mode. If false, the window will be created full-screen.
     * @returns The created window instance. If an error occurred while creating the window an invalid
     * window instance is returned. If a window with the given name already exists, that window will be 
     * returned.
     */
    Window& CreateRenderWindow( const std::string& windowName, int clientWidth, int clientHeight, bool vSync = false, bool windowed = true );

    /**
     * Destroy a window given the window name.
     */
    void DestroyWindow( const std::string& windowName );
    /**
     * Destroy a window given the window reference.
     */
    void DestroyWindow( Window& window );

    /**
     * Find a window by the window name.
     */
    Window& GetWindowByName( const std::string& windowName );

    /**
     * Run the application loop and message pump.
     * @return The error code if an error occurred.
     */
    int Run();
    
    /**
     * Request to quit the application and close all windows.
     * @param exitCode The error code to return to the invoking process.
     */
    void Quit(int exitCode = 0);

protected:

    // Create an application instance.
    Application( HINSTANCE hInst );
    // Destroy the aplication instance and all windows associated with this application.
    virtual ~Application();

private:
    // The application instance handle that this application was created with.
    HINSTANCE m_hInstance;

    // Return this invalid window when either an error occurs when creating a window
    // or the user asks for window by name but no window with that name exists.
    static Window ms_InvalidWindow;
};