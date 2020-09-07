/**
 *   @brief The Game class is the abstract base class for DirecX 11 demos.
 */
#pragma once

#include <Events.h>

class Window;

class Game
{
public:
    /**
     * Create the DirectX demo in the specified window.
     */
    Game( Window& window );
    virtual ~Game();

    /**
     *  Initialize the DirectX Runtime.
     */
    virtual bool Initialize();

    /**
     *  Load content required for the demo.
     */
    virtual bool LoadContent() = 0;

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    virtual void UnloadContent() = 0;

    /**
     * Cleanup the DirectX runtime.
    */
    virtual void Cleanup();

protected:
    friend class Window;

    // The window associated with this demo.
    Window& m_Window;

    // Direct3D device and swap chain.
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dDeviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_d3dSwapChain;

    // Render target view for the back buffer of the swap chain.
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_d3dRenderTargetView;
    // Depth/stencil view for use as a depth buffer.
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_d3dDepthStencilView;
    // A texture to associate to the depth stencil view.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_d3dDepthStencilBuffer;

    // Define the functionality of the depth/stencil stages.
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_d3dDepthStencilState;
    // Define the functionality of the rasterizer stage.
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_d3dRasterizerState;
    // Present parameters used by the IDXGISwapChain1::Present1 method
    DXGI_PRESENT_PARAMETERS m_PresentParameters;

    /**
     * Clear the contents of the back buffer, depth buffer, and stencil buffer.
     * This function is usually called before anything is rendered to the screen.
     */
    void Clear( const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil );
    /**
     * Swap the contents of the back buffer to the front.
     * This function is usually called after everything has been rendered.
     */
    void Present();

    /**
     *  Update the game logic.
     */
    virtual void OnUpdate( UpdateEventArgs& e );

    /**
     *  Render stuff.
     */
    virtual void OnRender( RenderEventArgs& e );

    /**
     * Invoked by the registered window when a key is pressed
     * while the window has focus.
     */
    virtual void OnKeyPressed( KeyEventArgs& e );

    /**
     * Invoked when a key on the keyboard is released.
     */
    virtual void OnKeyReleased( KeyEventArgs& e );

    /**
     * Invoked when the mouse is moved over the registered window.
     */
    virtual void OnMouseMoved( MouseMotionEventArgs& e );

    /**
     * Invoked when a mouse button is pressed over the registered window.
     */
    virtual void OnMouseButtonPressed( MouseButtonEventArgs& e );

    /**
     * Invoked when a mouse button is released over the registered window.
     */
    virtual void OnMouseButtonReleased( MouseButtonEventArgs& e );

    /**
     * Invoked when the mouse wheel is scrolled while the registered window has focus.
     */
    virtual void OnMouseWheel( MouseWheelEventArgs& e );

    /**
     * Invoked when the attached window is resized.
     */
    virtual void OnResize( ResizeEventArgs& e );

    /**
     * Invoked when the registered window instance is destroyed.
     */
    virtual void OnWindowDestroy();

private:

    // Resize the front and back buffers associated with the swap chain.
    bool ResizeSwapChain( int width, int height );

    bool m_bIsInitialized;

};