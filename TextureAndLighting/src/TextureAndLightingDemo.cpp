#include <TextureAndLightingPCH.h>
#include <TextureAndLightingDemo.h>

#include <Window.h>

#if _DEBUG
#include <SimpleVertexShader_d.h>
#include <InstancedVertexShader_d.h>
#include <TexturedLitPixelShader_d.h>
#else
#include <SimpleVertexShader.h>
#include <InstancedVertexShader.h>
#include <TexturedLitPixelShader.h>
#endif

using namespace DirectX;

// Per-vertex data.
struct VertexPosNormTex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 Tex0;
};
// Per-instance data (must be 16 byte aligned)
__declspec(align(16)) struct PlaneInstanceData
{
    XMMATRIX WorldMatrix;
    XMMATRIX InverseTransposeWorldMatrix;
};

// Vertices for a unit plane.
VertexPosNormTex g_PlaneVerts[4] =
{
    { XMFLOAT3( -0.5f, 0.0f,  0.5f ), XMFLOAT3( 0.0f, 1.0f, 0.0f ), XMFLOAT2( 0.0f, 0.0f ) }, // 0
    { XMFLOAT3(  0.5f, 0.0f,  0.5f ), XMFLOAT3( 0.0f, 1.0f, 0.0f ), XMFLOAT2( 1.0f, 0.0f ) }, // 1
    { XMFLOAT3(  0.5f, 0.0f, -0.5f ), XMFLOAT3( 0.0f, 1.0f, 0.0f ), XMFLOAT2( 1.0f, 1.0f ) }, // 2
    { XMFLOAT3( -0.5f, 0.0f, -0.5f ), XMFLOAT3( 0.0f, 1.0f, 0.0f ), XMFLOAT2( 0.0f, 1.0f ) }  // 3
};

// Index buffer for the unit plane.
WORD g_PlaneIndex[6] =
{
    0, 1, 3, 1, 2, 3
};

// A structure to hold the data for a per-object constant buffer
// defined in the vertex shader.
struct PerFrameConstantBufferData
{
    XMMATRIX ViewProjectionMatrix;
};

// This structure is used in the simple vertex shader.
struct PerObjectConstantBufferData
{
    XMMATRIX WorldMatrix;
    XMMATRIX InverseTransposeWorldMatrix;
    XMMATRIX WorldViewProjectionMatrix;
};

TextureAndLightingDemo::TextureAndLightingDemo( Window& window )
    : base(window)
    , m_W( 0 )
    , m_A( 0 )
    , m_S( 0 )
    , m_D( 0 )
    , m_Q( 0 )
    , m_E( 0 )
    , m_bShift( false )
    , m_Pitch( 0.0f )
    , m_Yaw( 0.0f )
    , m_bAnimate( false )
    , m_NumInstances( 6 )
{
    pData = (AlignedData*)_aligned_malloc( sizeof(AlignedData), 16 );
    
    XMVECTOR cameraPos = XMVectorSet( 0, 5, -20, 1 );
    XMVECTOR cameraTarget = XMVectorSet( 0, 5, 0, 1 );
    XMVECTOR cameraUp = XMVectorSet( 0, 1, 0, 0 );

    m_Camera.set_LookAt( cameraPos, cameraTarget, cameraUp );

    pData->m_InitialCameraPos = m_Camera.get_Translation();
    pData->m_InitialCameraRot = m_Camera.get_Rotation();
}

TextureAndLightingDemo::~TextureAndLightingDemo()
{
    // Make sure the content is unloaded.
    UnloadContent();
    _aligned_free(pData);
}

bool TextureAndLightingDemo::LoadContent()
{
    HRESULT hr = 0;

    m_EffectFactory = std::unique_ptr<EffectFactory>(new EffectFactory(m_d3dDevice.Get()));
    m_EffectFactory->SetDirectory( L"..\\data\\" );

    try 
    {
        m_EffectFactory->CreateTexture( L"Textures\\DirectX9.png", m_d3dDeviceContext.Get(), &m_DirectXTexture );
        m_EffectFactory->CreateTexture( L"Textures\\earth.dds", m_d3dDeviceContext.Get(), &m_EarthTexture );
    }
    catch ( std::exception& )
    {
        MessageBoxA(m_Window.get_WindowHandle(), "Failed to load texture.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Create a sampler state for texture sampling in the pixel shader
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory( &samplerDesc, sizeof(D3D11_SAMPLER_DESC) );

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.MinLOD = -FLT_MAX;
    samplerDesc.MaxLOD = FLT_MAX;

    hr = m_d3dDevice->CreateSamplerState( &samplerDesc, &m_d3dSamplerState );
    if ( FAILED( hr ) )
    {
        MessageBoxA(m_Window.get_WindowHandle(), "Failed to create texture sampler.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Create some materials
    MaterialProperties defaultMaterial;
    m_MaterialProperties.push_back( defaultMaterial );

    MaterialProperties greenMaterial;
    greenMaterial.Material.Ambient = XMFLOAT4(  0.07568f, 0.61424f, 0.07568f, 1.0f );
    greenMaterial.Material.Diffuse = XMFLOAT4( 0.07568f, 0.61424f, 0.07568f, 1.0f );
    greenMaterial.Material.Specular = XMFLOAT4( 0.07568f, 0.61424f, 0.07568f, 1.0f );
    greenMaterial.Material.SpecularPower = 76.8f;
    m_MaterialProperties.push_back( greenMaterial );

    MaterialProperties redPlasticMaterial;
    redPlasticMaterial.Material.Diffuse = XMFLOAT4( 0.6f, 0.1f, 0.1f, 1.0f );
    redPlasticMaterial.Material.Specular = XMFLOAT4( 1.0f, 0.2f, 0.2f, 1.0f );
    redPlasticMaterial.Material.SpecularPower = 32.0f;
    m_MaterialProperties.push_back( redPlasticMaterial );

    MaterialProperties pearlMaterial;
    pearlMaterial.Material.Ambient = XMFLOAT4( 0.25f, 0.20725f, 0.20725f, 1.0f );
    pearlMaterial.Material.Diffuse = XMFLOAT4( 1.0f, 0.829f, 0.829f, 1.0f );
    pearlMaterial.Material.Specular = XMFLOAT4( 0.296648f, 0.296648f, 0.296648f, 1.0f );
    pearlMaterial.Material.SpecularPower = 11.264f;
    m_MaterialProperties.push_back( pearlMaterial );

    // Create and initialize a vertex buffer for a plane.
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory( &vertexBufferDesc, sizeof(D3D11_BUFFER_DESC) );

    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.ByteWidth = sizeof( g_PlaneVerts );
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA resourceData;
    ZeroMemory( &resourceData, sizeof(D3D11_SUBRESOURCE_DATA) );

    resourceData.pSysMem = g_PlaneVerts;

    hr = m_d3dDevice->CreateBuffer( &vertexBufferDesc, &resourceData, &m_d3dPlaneVertexBuffer );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create vertex buffer.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Create and setup the per-instance buffer data
    PlaneInstanceData* planeInstanceData = (PlaneInstanceData*)_aligned_malloc( sizeof(PlaneInstanceData) * m_NumInstances, 16 );

    float scalePlane = 20.0f;
    float translateOffset = scalePlane / 2.0f;
    XMMATRIX scaleMatrix = XMMatrixScaling( scalePlane, 1.0f, scalePlane );
    XMMATRIX translateMatrix = XMMatrixTranslation( 0, 0, 0 );
    XMMATRIX rotateMatrix = XMMatrixRotationX( 0.0f );

    // Floor plane.
    XMMATRIX worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;
    planeInstanceData[0].WorldMatrix = worldMatrix;
    planeInstanceData[0].InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );
    
    // Back wall plane.
    translateMatrix = XMMatrixTranslation( 0, translateOffset, translateOffset );
    rotateMatrix = XMMatrixRotationX( XMConvertToRadians(-90) );
    worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

    planeInstanceData[1].WorldMatrix = worldMatrix;
    planeInstanceData[1].InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );

    // Ceiling plane.
    translateMatrix = XMMatrixTranslation( 0, translateOffset * 2.0f, 0 );
    rotateMatrix = XMMatrixRotationX( XMConvertToRadians(180) );
    worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

    planeInstanceData[2].WorldMatrix = worldMatrix;
    planeInstanceData[2].InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );

    // Front wall plane.
    translateMatrix = XMMatrixTranslation( 0, translateOffset, -translateOffset );
    rotateMatrix = XMMatrixRotationX( XMConvertToRadians(90) );
    worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

    planeInstanceData[3].WorldMatrix = worldMatrix;
    planeInstanceData[3].InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );

    // Left wall plane.
    translateMatrix = XMMatrixTranslation( -translateOffset, translateOffset, 0);
    rotateMatrix = XMMatrixRotationZ( XMConvertToRadians(-90) );
    worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

    planeInstanceData[4].WorldMatrix = worldMatrix;
    planeInstanceData[4].InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );

    // Right wall plane.
    translateMatrix = XMMatrixTranslation( translateOffset, translateOffset, 0);
    rotateMatrix = XMMatrixRotationZ( XMConvertToRadians(90) );
    worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

    planeInstanceData[5].WorldMatrix = worldMatrix;
    planeInstanceData[5].InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );


    // Create the per-instance vertex buffer.
    D3D11_BUFFER_DESC instanceBufferDesc;
    ZeroMemory( &instanceBufferDesc, sizeof(D3D11_BUFFER_DESC) );

    instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    instanceBufferDesc.ByteWidth = sizeof( PlaneInstanceData ) * m_NumInstances;
    instanceBufferDesc.CPUAccessFlags = 0;
    instanceBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    resourceData.pSysMem = planeInstanceData;

    hr = m_d3dDevice->CreateBuffer( &instanceBufferDesc, &resourceData, &m_d3dPlaneInstanceBuffer );

    _aligned_free( planeInstanceData );

    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create instance buffer.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }
    
    // Create and setup the index buffer for a plane.
    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory( &indexBufferDesc, sizeof(D3D11_BUFFER_DESC) );

    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.ByteWidth = sizeof( g_PlaneIndex );
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    resourceData.pSysMem = g_PlaneIndex;

    hr = m_d3dDevice->CreateBuffer( &indexBufferDesc, &resourceData, &m_d3dPlaneIndexBuffer );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create index buffer.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    hr = m_d3dDevice->CreateVertexShader( g_InstancedVertexShader, sizeof(g_InstancedVertexShader), nullptr, &m_d3dInstancedVertexShader );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to load vertex shader.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Create the input layout for rendering instanced vertex data.
    D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] = 
    {
        // Per-vertex data.
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        // Per-instance data.
        { "WORLDMATRIX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "WORLDMATRIX", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "WORLDMATRIX", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "WORLDMATRIX", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INVERSETRANSPOSEWORLDMATRIX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INVERSETRANSPOSEWORLDMATRIX", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INVERSETRANSPOSEWORLDMATRIX", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INVERSETRANSPOSEWORLDMATRIX", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    hr = m_d3dDevice->CreateInputLayout( vertexLayoutDesc, _countof(vertexLayoutDesc), g_InstancedVertexShader, sizeof(g_InstancedVertexShader), &m_d3dInstancedInputLayout );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create input layout.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    hr = m_d3dDevice->CreatePixelShader( g_TexturedLitPixelShader, sizeof(g_TexturedLitPixelShader), nullptr, &m_d3dTexturedLitPixelShader );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to load pixel shader.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Create a constant buffer for the per-frame data required by the instanced vertex shader.
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory( &constantBufferDesc, sizeof(D3D11_BUFFER_DESC) );

    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.ByteWidth = sizeof( PerFrameConstantBufferData );
    constantBufferDesc.CPUAccessFlags = 0;
    constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = m_d3dDevice->CreateBuffer( &constantBufferDesc, nullptr, &m_d3dPerFrameConstantBuffer );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create constant buffer for per-frame data.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Create a constant buffer for the per-object data required by teh simple vertex shader.
    constantBufferDesc.ByteWidth = sizeof( PerObjectConstantBufferData );

    hr = m_d3dDevice->CreateBuffer( &constantBufferDesc, nullptr, &m_d3dPerObjectConstantBuffer );
    if ( FAILED(hr) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create constant buffer for per-object data.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }
    
    // Create a constant buffer for the material properties required by the pixel shader.
    constantBufferDesc.ByteWidth = sizeof( MaterialProperties );

    hr = m_d3dDevice->CreateBuffer( &constantBufferDesc, nullptr, &m_d3dMaterialPropertiesConstantBuffer );
    if ( FAILED( hr ) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create constant buffer for material properties.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Create a constant buffer for the light properties required by the pixel shader.
    constantBufferDesc.ByteWidth = sizeof( LightProperties );
    hr = m_d3dDevice->CreateBuffer( &constantBufferDesc, nullptr, &m_d3dLightPropertiesConstantBuffer );
    if ( FAILED( hr ) )
    {
        MessageBoxA( m_Window.get_WindowHandle(), "Failed to create constant buffer for light properties.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }


    // Global ambient
    m_LightProperties.GlobalAmbient = XMFLOAT4( 0.2f, 0.2f, 0.2f, 1.0f );

    m_Sphere = Mesh::CreateSphere( m_d3dDeviceContext.Get(), 1.0f, 16, false );
    m_Cube = Mesh::CreateCube( m_d3dDeviceContext.Get(), 1.0f, false );
    m_Cone = Mesh::CreateCone( m_d3dDeviceContext.Get(), 1.0f, 1.0f, 32, false );
    m_Torus = Mesh::CreateTorus( m_d3dDeviceContext.Get(), 1.0f, 0.33f, 32, false );

    // Load a simple vertex shader that will be used to render the shapes.
    hr = m_d3dDevice->CreateVertexShader( g_SimpleVertexShader, sizeof(g_SimpleVertexShader), nullptr, &m_d3dSimplVertexShader );
    if ( FAILED(hr) )
    {
        MessageBoxA(m_Window.get_WindowHandle(), "Failed to create the simple vertex shader.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }

    // Create the input layout for the simple shapes.
    hr = m_d3dDevice->CreateInputLayout( VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount, g_SimpleVertexShader, sizeof(g_SimpleVertexShader), &m_d3dVertexPositionNormalTextureInputLayout );
    if ( FAILED(hr) )
    {
        MessageBoxA(m_Window.get_WindowHandle(), "Failed to create the input layout for the simple vertex shader.", "Error", MB_OK|MB_ICONERROR );
        return false;
    }
    
    
    // Force a resize event so the camera's projection matrix gets initialized.
    ResizeEventArgs resizeEventArgs( m_Window.get_ClientWidth(), m_Window.get_ClientHeight() );
    OnResize( resizeEventArgs );

    return true;
}

void TextureAndLightingDemo::OnUpdate( UpdateEventArgs& e )
{
    float speedMultipler = ( m_bShift ? 8.0f : 4.0f );

    XMVECTOR cameraTranslate = XMVectorSet( static_cast<float>(m_D - m_A), 0.0f, static_cast<float>(m_W - m_S), 1.0f ) * speedMultipler * e.ElapsedTime;
    XMVECTOR cameraPan = XMVectorSet( 0.0f, static_cast<float>(m_E - m_Q), 0.0f, 1.0f ) * speedMultipler * e.ElapsedTime;
    m_Camera.Translate( cameraTranslate, Camera::LocalSpace );
    m_Camera.Translate( cameraPan, Camera::WorldSpace );

    XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw( XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f );
    m_Camera.set_Rotation( cameraRotation );

    // Update the light properties
    XMStoreFloat4( &m_LightProperties.EyePosition, m_Camera.get_Translation() );

    static float totalTime = 0.0f;

    if ( m_bAnimate )
    {
        totalTime += e.ElapsedTime * 0.5f * XM_PI;
    }

    static const XMVECTORF32 LightColors[MAX_LIGHTS] = {
        Colors::White, Colors::Orange, Colors::Yellow, Colors::Green, Colors::Blue, Colors::Indigo, Colors::Violet, Colors::White
    };

    static const LightType LightTypes[MAX_LIGHTS] = {
        SpotLight, SpotLight, SpotLight, PointLight, SpotLight, SpotLight, SpotLight, PointLight
    };

    static const bool LightEnabled[MAX_LIGHTS] = {
        true, true, true, true, true, true, true, true
    };

    const int numLights = MAX_LIGHTS;
    float radius = 8.0f;
    float offset = 2.0f * XM_PI / numLights;
    for( int i = 0; i < numLights; ++i )
    {
        Light light;
        light.Enabled = static_cast<int>(LightEnabled[i]);
        light.LightType = LightTypes[i];
        light.Color = XMFLOAT4(LightColors[i]);
        light.SpotAngle = XMConvertToRadians(45.0f);
        light.ConstantAttenuation = 1.0f;
        light.LinearAttenuation = 0.08f;
        light.QuadraticAttenuation = 0.0f;
        XMFLOAT4 LightPosition = XMFLOAT4( std::sin( totalTime + offset * i ) * radius, 9.0f, std::cos( totalTime + offset * i ) * radius, 1.0f );
        light.Position = LightPosition;
        XMVECTOR LightDirection = XMVectorSet( -LightPosition.x, -LightPosition.y, -LightPosition.z, 0.0f );
        LightDirection = XMVector3Normalize( LightDirection );
        XMStoreFloat4( &light.Direction, LightDirection );

        m_LightProperties.Lights[i] = light;
    }

    // Update the light properties
    m_d3dDeviceContext->UpdateSubresource( m_d3dLightPropertiesConstantBuffer.Get(), 0, nullptr, &m_LightProperties, 0, 0 );
}

// Builds a look-at (world) matrix from a point, up and direction vectors.
XMMATRIX XM_CALLCONV LookAtMatrix( FXMVECTOR Position, FXMVECTOR Direction, FXMVECTOR Up )
{
    assert(!XMVector3Equal(Direction, XMVectorZero()));
    assert(!XMVector3IsInfinite(Direction));
    assert(!XMVector3Equal(Up, XMVectorZero()));
    assert(!XMVector3IsInfinite(Up));

    XMVECTOR R2 = XMVector3Normalize(Direction);

    XMVECTOR R0 = XMVector3Cross(Up, R2);
    R0 = XMVector3Normalize(R0);

    XMVECTOR R1 = XMVector3Cross(R2, R0);

    XMMATRIX M( R0, R1, R2, Position);

    return M;
}

void TextureAndLightingDemo::OnRender( RenderEventArgs& e )
{
    Clear( DirectX::Colors::CornflowerBlue, 1.0f, 0 );
    
    float aspectRatio = m_Window.get_ClientWidth() / (float)m_Window.get_ClientHeight();

    XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();
    XMMATRIX projectionMatrix = m_Camera.get_ProjectionMatrix();
    XMMATRIX viewProjectionMatrix = viewMatrix * projectionMatrix;

    PerFrameConstantBufferData constantBufferData;
    constantBufferData.ViewProjectionMatrix = viewProjectionMatrix;

    MaterialProperties wallMaterial = m_MaterialProperties[1];
    wallMaterial.Material.UseTexture = true;

    m_d3dDeviceContext->UpdateSubresource( m_d3dPerFrameConstantBuffer.Get(), 0, nullptr, &constantBufferData, 0, 0 );
    m_d3dDeviceContext->UpdateSubresource( m_d3dMaterialPropertiesConstantBuffer.Get(), 0, nullptr, &wallMaterial, 0, 0 );

    const UINT vertexStride[2] = { sizeof(VertexPosNormTex), sizeof(PlaneInstanceData) };
    const UINT offset[2] = { 0, 0 };
    ID3D11Buffer* buffers[2] = { m_d3dPlaneVertexBuffer.Get(), m_d3dPlaneInstanceBuffer.Get() };

    m_d3dDeviceContext->IASetVertexBuffers( 0, 2, buffers, vertexStride, offset );
    m_d3dDeviceContext->IASetInputLayout( m_d3dInstancedInputLayout.Get() );
    m_d3dDeviceContext->IASetIndexBuffer( m_d3dPlaneIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 );
    m_d3dDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    m_d3dDeviceContext->RSSetState( m_d3dRasterizerState.Get() );
    D3D11_VIEWPORT viewport = m_Camera.get_Viewport();
    m_d3dDeviceContext->RSSetViewports( 1, &viewport ); 

    m_d3dDeviceContext->VSSetShader( m_d3dInstancedVertexShader.Get(), nullptr, 0 );
    m_d3dDeviceContext->VSSetConstantBuffers( 0, 1, m_d3dPerFrameConstantBuffer.GetAddressOf() );

    m_d3dDeviceContext->PSSetShader( m_d3dTexturedLitPixelShader.Get(), nullptr, 0 );

    ID3D11Buffer* pixelShaderConstantBuffers[2] = { m_d3dMaterialPropertiesConstantBuffer.Get(), m_d3dLightPropertiesConstantBuffer.Get() };
    m_d3dDeviceContext->PSSetConstantBuffers( 0, 2, pixelShaderConstantBuffers );

    m_d3dDeviceContext->PSSetSamplers( 0, 1, m_d3dSamplerState.GetAddressOf() );
    m_d3dDeviceContext->PSSetShaderResources( 0, 1, m_DirectXTexture.GetAddressOf() );

    m_d3dDeviceContext->OMSetRenderTargets( 1, m_d3dRenderTargetView.GetAddressOf(), m_d3dDepthStencilView.Get() );
    m_d3dDeviceContext->OMSetDepthStencilState( m_d3dDepthStencilState.Get(), 0 );

    m_d3dDeviceContext->DrawIndexedInstanced( _countof(g_PlaneIndex), m_NumInstances, 0, 0, 0 );

    // Draw the sphere
    XMMATRIX translationMatrix = XMMatrixTranslation( -4.0f, 2.0f, -4.0f );
    XMMATRIX rotationMatrix = XMMatrixIdentity();
    XMMATRIX scaleMatrix = XMMatrixScaling( 4.0f, 4.0f, 4.0f );
    XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

    PerObjectConstantBufferData perObjectConstantBufferData;
    perObjectConstantBufferData.WorldMatrix = worldMatrix;
    perObjectConstantBufferData.InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );
    perObjectConstantBufferData.WorldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

    m_d3dDeviceContext->UpdateSubresource( m_d3dPerObjectConstantBuffer.Get(), 0, nullptr, &perObjectConstantBufferData, 0, 0 );

    MaterialProperties sphereMaterial = m_MaterialProperties[0];
    sphereMaterial.Material.UseTexture = true;

    m_d3dDeviceContext->UpdateSubresource( m_d3dMaterialPropertiesConstantBuffer.Get(), 0, nullptr, &sphereMaterial, 0, 0 );
    
    m_d3dDeviceContext->VSSetShader( m_d3dSimplVertexShader.Get(), nullptr, 0 );
    m_d3dDeviceContext->VSSetConstantBuffers( 0, 1, m_d3dPerObjectConstantBuffer.GetAddressOf() );
    
    m_d3dDeviceContext->PSSetShaderResources( 0, 1, m_EarthTexture.GetAddressOf() );

    m_d3dDeviceContext->IASetInputLayout( m_d3dVertexPositionNormalTextureInputLayout.Get() );

    m_Sphere->Draw( m_d3dDeviceContext.Get() );

    // Draw a cube
    translationMatrix = XMMatrixTranslation( 4.0f, 4.0f, 4.0f );
    rotationMatrix = XMMatrixRotationY( XMConvertToRadians(45.0f) );
    scaleMatrix = XMMatrixScaling( 4.0f, 8.0f, 4.0f );
    worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

    perObjectConstantBufferData.WorldMatrix = worldMatrix;
    perObjectConstantBufferData.InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );
    perObjectConstantBufferData.WorldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

    m_d3dDeviceContext->UpdateSubresource( m_d3dPerObjectConstantBuffer.Get(), 0, nullptr, &perObjectConstantBufferData, 0, 0 );

    m_d3dDeviceContext->UpdateSubresource( m_d3dMaterialPropertiesConstantBuffer.Get(), 0, nullptr, &m_MaterialProperties[2], 0, 0 );

    m_Cube->Draw( m_d3dDeviceContext.Get() );

    // Draw a torus
    translationMatrix = XMMatrixTranslation( 4.0f, 0.5f, -4.0f );
    rotationMatrix = XMMatrixRotationY( XMConvertToRadians(45.0f) );
    scaleMatrix = XMMatrixScaling( 4.0f, 4.0f, 4.0f );
    worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

    perObjectConstantBufferData.WorldMatrix = worldMatrix;
    perObjectConstantBufferData.InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );
    perObjectConstantBufferData.WorldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

    m_d3dDeviceContext->UpdateSubresource( m_d3dPerObjectConstantBuffer.Get(), 0, nullptr, &perObjectConstantBufferData, 0, 0 );
    m_d3dDeviceContext->UpdateSubresource( m_d3dMaterialPropertiesConstantBuffer.Get(), 0, nullptr, &m_MaterialProperties[3], 0, 0 );

    m_Torus->Draw( m_d3dDeviceContext.Get() );

    // Draw geometry at the position of the active lights in the scene.
    MaterialProperties lightMaterial = m_MaterialProperties[0];
    for ( int i = 0; i < MAX_LIGHTS; ++i )
    {
        Light* pLight = &(m_LightProperties.Lights[i]);
        if ( !pLight->Enabled ) continue;

        XMVECTOR lightPos = XMLoadFloat4( &(pLight->Position) );
        XMVECTOR lightDir = XMLoadFloat4( &(pLight->Direction) );
        XMVECTOR UpDirection = XMVectorSet( 0, 1, 0, 0 );

        scaleMatrix = XMMatrixScaling( 1.0f, 1.0f, 1.0f );
        rotationMatrix = XMMatrixRotationX( -90.0f );
        worldMatrix = scaleMatrix * rotationMatrix * LookAtMatrix( lightPos, lightDir, UpDirection );

        perObjectConstantBufferData.WorldMatrix = worldMatrix;
        perObjectConstantBufferData.InverseTransposeWorldMatrix = XMMatrixTranspose( XMMatrixInverse(nullptr, worldMatrix) );
        perObjectConstantBufferData.WorldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

        lightMaterial.Material.Emissive = pLight->Color;

        m_d3dDeviceContext->UpdateSubresource( m_d3dPerObjectConstantBuffer.Get(), 0, nullptr, &perObjectConstantBufferData, 0, 0 );
        m_d3dDeviceContext->UpdateSubresource( m_d3dMaterialPropertiesConstantBuffer.Get(), 0, nullptr, &lightMaterial, 0, 0 );
        switch( pLight->LightType )
        {
        case PointLight:
            {
                m_Sphere->Draw( m_d3dDeviceContext.Get() );
            }
            break;
        case DirectionalLight:
        case SpotLight:
            {
                m_Cone->Draw( m_d3dDeviceContext.Get() );
            }
            break;
        }
    }

    Present();
}

void TextureAndLightingDemo::UnloadContent()
{

}

void TextureAndLightingDemo::OnKeyPressed( KeyEventArgs& e )
{
    base::OnKeyPressed(e);

    switch( e.Key )
    {
    case KeyCode::Escape:
        {
            // Close the window associated with this demo.
            m_Window.Destroy();
        }
        break;
    case KeyCode::W:
        {
            m_W = 1;
        }
        break;
    case KeyCode::Left:
    case KeyCode::A:
        {
            m_A = 1;
        }
        break;
    case KeyCode::S:
        {
            m_S = 1;
        }
        break;
    case KeyCode::Right:
    case KeyCode::D:
        {
            m_D = 1;
        }
        break;
    case KeyCode::Down:
    case KeyCode::Q:
        {
            m_Q = 1;
        }
        break;
    case KeyCode::Up:
    case KeyCode::E:
        {
            m_E = 1;
        }
        break;
    case KeyCode::R:
        {
            // Reset camera position and orientation
            m_Camera.set_Translation( pData->m_InitialCameraPos );
            m_Camera.set_Rotation( pData->m_InitialCameraRot );
            m_Pitch = 0.0f;
            m_Yaw = 0.0f;
        }
        break;
    case KeyCode::ShiftKey:
        {
            m_bShift = true;
        }
        break;
    case KeyCode::Space:
        {
            m_bAnimate = !m_bAnimate;
        }
        break;
    }
}

void TextureAndLightingDemo::OnKeyReleased( KeyEventArgs& e )
{
    base::OnKeyReleased(e);

    switch( e.Key )
    {
    case KeyCode::W:
        {
            m_W = 0;
        }
        break;
    case KeyCode::Left:
    case KeyCode::A:
        {
            m_A = 0;
        }
        break;
    case KeyCode::S:
        {
            m_S = 0;
        }
        break;
    case KeyCode::Right:
    case KeyCode::D:
        {
            m_D = 0;
        }
        break;
    case KeyCode::Q:
    case KeyCode::Down:
        {
            m_Q = 0;
        }
        break;
    case KeyCode::E:
    case KeyCode::Up:
        {
            m_E = 0;
        }
        break;
    case KeyCode::ShiftKey:
        {
            m_bShift = false;
        }
        break;
    }
}

void TextureAndLightingDemo::OnMouseButtonPressed( MouseButtonEventArgs& e )
{
    base::OnMouseButtonPressed( e );

    m_PreviousMousePosition = XMINT2( e.X, e.Y );
}

// No minus operator for vector types in the DirectX Math library? I guess we'll create our own!
XMINT2 operator-( const XMINT2& x0, const XMINT2& x1 )
{
    return XMINT2( x0.x - x1.x, x0.y - x1.y );
}

void TextureAndLightingDemo::OnMouseMoved( MouseMotionEventArgs& e )
{
    base::OnMouseMoved( e );

    const float moveSpeed = 0.5f;

    XMINT2 currentMousePosition = XMINT2( e.X, e.Y );
    XMINT2 mouseDelta = m_PreviousMousePosition - currentMousePosition;
    m_PreviousMousePosition = currentMousePosition;

    if ( e.LeftButton )
    {
        m_Pitch -= mouseDelta.y * moveSpeed;
        m_Yaw -= mouseDelta.x * moveSpeed;
    }
}

void TextureAndLightingDemo::OnMouseWheel( MouseWheelEventArgs& e )
{
    base::OnMouseWheel( e );

}

void TextureAndLightingDemo::OnResize( ResizeEventArgs& e )
{
    // Don't forget to call the base class's resize method.
    // The base class handles resizing of the swap chain.
    base::OnResize(e);

    if ( e.Height < 1 )
    {
        e.Height = 1;
    }

    float aspectRatio = e.Width / (float)e.Height;

    m_Camera.set_Projection( 45.0f, aspectRatio, 0.1f, 100.0f );

    // Setup the viewports for the camera.
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<FLOAT>(m_Window.get_ClientWidth());
    viewport.Height = static_cast<FLOAT>(m_Window.get_ClientHeight());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    m_Camera.set_Viewport( viewport );

    m_d3dDeviceContext->RSSetViewports( 1, &viewport ); 
}
