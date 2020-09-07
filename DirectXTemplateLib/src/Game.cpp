#include <DirectXTemplateLibPCH.h>
#include <Game.h>
#include <Window.h>

Game::Game( Window& window )
    : m_Window( window )
    , m_d3dDevice(nullptr)
    , m_d3dDeviceContext(nullptr)
    , m_d3dSwapChain(nullptr)
    , m_d3dRenderTargetView(nullptr)
    , m_d3dDepthStencilView(nullptr)
    , m_d3dDepthStencilBuffer(nullptr)
    , m_d3dDepthStencilState(nullptr)
    , m_d3dRasterizerState(nullptr)
    , m_bIsInitialized( false )
{
    m_Window.RegisterDirectXTemplate(this);
}

Game::~Game()
{
    Cleanup();
}

// This function was inspired by:
// http://www.rastertek.com/dx11tut03.html
DXGI_RATIONAL QueryRefreshRate( const Window& window )
{
    DXGI_RATIONAL refreshRate = { 0, 1 };
    if ( window.get_VSync() )
    {
        Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        Microsoft::WRL::ComPtr<IDXGIOutput> adapterOutput;

        DXGI_MODE_DESC* displayModeList;

        // Create a DirectX graphics interface factory.
        HRESULT hr = CreateDXGIFactory( __uuidof(IDXGIFactory2), &factory );
        if ( FAILED(hr) )
        {
            MessageBoxA( 0,
                TEXT("Could not create DXGIFactory instance."),
                TEXT("Query Refresh Rate"),
                MB_OK );

            throw new std::exception("Failed to create DXGIFactory.");
        }

        hr = factory->EnumAdapters(0, &adapter );
        if ( FAILED(hr) )
        {
            MessageBoxA( 0,
                TEXT("Failed to enumerate adapters."),
                TEXT("Query Refresh Rate"),
                MB_OK );

            throw new std::exception("Failed to enumerate adapters.");
        }

        hr = adapter->EnumOutputs( 0, &adapterOutput );
        if ( FAILED(hr) )
        {
            MessageBoxA( 0,
                TEXT("Failed to enumerate adapter outputs."),
                TEXT("Query Refresh Rate"),
                MB_OK );

            throw new std::exception("Failed to enumerate adapter outputs.");
        }

        UINT numDisplayModes;
        hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, nullptr );
        if ( FAILED(hr) )
        {
            MessageBoxA( 0,
                TEXT("Failed to query display mode list."),
                TEXT("Query Refresh Rate"),
                MB_OK );

            throw new std::exception("Failed to query display mode list.");
        }

        displayModeList = new DXGI_MODE_DESC[numDisplayModes];
        assert( displayModeList);

        hr = adapterOutput->GetDisplayModeList( DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList );
        if ( FAILED(hr) )
        {
            MessageBoxA( 0,
                TEXT("Failed to query display mode list."),
                TEXT("Query Refresh Rate"),
                MB_OK );

            throw new std::exception("Failed to query display mode list.");
        }

        // Now store the refresh rate of the monitor that matches the width and height of the requested screen.
        for ( UINT i = 0; i < numDisplayModes; ++i )
        {
            if ( displayModeList[i].Width == window.get_ClientWidth() && displayModeList[i].Height == window.get_ClientHeight() )
            {
                refreshRate = displayModeList[i].RefreshRate;
            }
        }

        delete [] displayModeList;
    }

    return refreshRate;
}

bool Game::ResizeSwapChain( int width, int height )
{
    // Don't allow for 0 size swap chain buffers.
    if ( width <= 0 ) width = 1;
    if ( height <= 0 ) height = 1;

    m_d3dDeviceContext->OMSetRenderTargets( 0, nullptr, nullptr );

    // First release the render target and depth/stencil views.
    m_d3dRenderTargetView.Reset();
    m_d3dDepthStencilView.Reset();
    m_d3dDepthStencilBuffer.Reset();

    // Resize the swap chain buffers.
    m_d3dSwapChain->ResizeBuffers( 1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0 );

    // Next initialize the back buffer of the swap chain and associate it to a 
    // render target view.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_d3dSwapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), &backBuffer );
    if ( FAILED( hr ) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to retrieve the swap chain back buffer.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    hr = m_d3dDevice->CreateRenderTargetView( backBuffer.Get(), nullptr, &m_d3dRenderTargetView );
    if ( FAILED( hr ) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create the RenderTargetView.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    backBuffer.Reset();

    // Create the depth buffer for use with the depth/stencil view.
    D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
    ZeroMemory( &depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC) );

    depthStencilBufferDesc.ArraySize = 1;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0; // No CPU access required.
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilBufferDesc.Width = width;
    depthStencilBufferDesc.Height = height;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = m_d3dDevice->CreateTexture2D( &depthStencilBufferDesc, nullptr, &m_d3dDepthStencilBuffer );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create the Depth/Stencil texture.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    hr = m_d3dDevice->CreateDepthStencilView( m_d3dDepthStencilBuffer.Get(), nullptr, &m_d3dDepthStencilView );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create DepthStencilView.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    return true;
}


bool Game::Initialize()
{
    if ( !m_Window.IsValid() )
    {
        // Whoops.. We need a valid window to initialize our DirectX demo.
        return false;
    }

    if ( m_bIsInitialized )
    {
        // We are already initialized. Don't try to initialize this demo again!
        return true;
    }

    // Check for DirectX Math library support.
    if ( !DirectX::XMVerifyCPUSupport() )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to verify DirectX Math library support.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    HRESULT hr = 0;

    UINT createDeviceFlags = 0;
#if _DEBUG
    createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

    // These are the feature levels that we will accept.
    D3D_FEATURE_LEVEL featureLevels[] = 
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    // This will be the feature level that 
    // is used to create our device and swap chain.
    D3D_FEATURE_LEVEL featureLevel;

    hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, 
        nullptr, createDeviceFlags, featureLevels, _countof(featureLevels),
        D3D11_SDK_VERSION, &m_d3dDevice, &featureLevel, &m_d3dDeviceContext );
    
    if ( hr == E_INVALIDARG )
    {
        hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, 
            nullptr, createDeviceFlags, &featureLevels[1], _countof(featureLevels) - 1, 
            D3D11_SDK_VERSION, &m_d3dDevice, &featureLevel, &m_d3dDeviceContext );
    }

    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create DirectX 11 Device.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    hr = CreateDXGIFactory( __uuidof(IDXGIFactory2), &factory );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create IDXGIFactory2.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    ZeroMemory( &swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1) );

    swapChainDesc.BufferCount = 1;
    swapChainDesc.Width = m_Window.get_ClientWidth();
    swapChainDesc.Height = m_Window.get_ClientHeight();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // Use Alt-Enter to switch between full screen and windowed mode.

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc;
    ZeroMemory( &swapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC) );

    swapChainFullScreenDesc.RefreshRate = QueryRefreshRate( m_Window );
    swapChainFullScreenDesc.Windowed = m_Window.get_Windowed();

    hr = factory->CreateSwapChainForHwnd( m_d3dDevice.Get(), m_Window.get_WindowHandle(), 
        &swapChainDesc, &swapChainFullScreenDesc, nullptr, &m_d3dSwapChain );

    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create swap chain.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    if ( !ResizeSwapChain( m_Window.get_ClientWidth(), m_Window.get_ClientHeight() ) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to resize the swap chain.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Setup depth/stencil state.
    D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
    ZeroMemory( &depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC) );

    depthStencilStateDesc.DepthEnable = TRUE;
    depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilStateDesc.StencilEnable = FALSE;

    hr = m_d3dDevice->CreateDepthStencilState( &depthStencilStateDesc, &m_d3dDepthStencilState );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create a DepthStencilState object.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Setup rasterizer state.
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory( &rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC) );

    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    // Create the rasterizer state object.
    hr = m_d3dDevice->CreateRasterizerState( &rasterizerDesc, &m_d3dRasterizerState );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create a RasterizerState object.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    ZeroMemory( &m_PresentParameters, sizeof(DXGI_PRESENT_PARAMETERS) );

    m_bIsInitialized = true;

    return true;
}

void Game::Clear( const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil )
{
    assert( m_d3dDeviceContext );
    m_d3dDeviceContext->ClearRenderTargetView( m_d3dRenderTargetView.Get(), clearColor );
    m_d3dDeviceContext->ClearDepthStencilView( m_d3dDepthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, clearDepth, clearStencil );
}

void Game::Present()
{
    if ( m_Window.get_VSync() )
    {
        m_d3dSwapChain->Present1( 1, 0, &m_PresentParameters );
    }
    else
    {
        m_d3dSwapChain->Present1( 0, 0, &m_PresentParameters  );
    }
}

void Game::Cleanup()
{
    if ( m_d3dSwapChain )
    {
        // Before we shutdown, exit the fullscreen state. If we shutdown while we are 
        // in the fullscreen state, the swap chain will thorw an exception.
        m_d3dSwapChain->SetFullscreenState( false, nullptr );
    }
}

void Game::OnUpdate( UpdateEventArgs& e )
{

}

void Game::OnRender( RenderEventArgs& e )
{

}

void Game::OnKeyPressed( KeyEventArgs& e )
{
    // By default, do nothing.
}

void Game::OnKeyReleased( KeyEventArgs& e )
{
    // By default, do nothing.
}

void Game::OnMouseMoved(class MouseMotionEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnMouseButtonPressed( MouseButtonEventArgs& e )
{
    // By default, do nothing.
}

void Game::OnMouseButtonReleased( MouseButtonEventArgs& e )
{
    // By default, do nothing.
}

void Game::OnMouseWheel( MouseWheelEventArgs& e )
{
    // By default, do nothing.
}

void Game::OnResize( ResizeEventArgs& e )
{
    assert( m_d3dSwapChain );
    ResizeSwapChain( e.Width, e.Height );
}

void Game::OnWindowDestroy()
{
    // If the Window which we are registered to is 
    // destroyed, then any resources which are associated 
    // to the window must be released.
    UnloadContent();
    Cleanup();
}

