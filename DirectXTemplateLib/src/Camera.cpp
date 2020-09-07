#include <DirectXTemplateLibPCH.h>
#include <Camera.h>

using namespace DirectX;

Camera::Camera( Handedness handedness )
    : m_ViewDirty( true )
    , m_InverseViewDirty( true )
    , m_ProjectionDirty( true )
    , m_InverseProjectionDirty( true )
    , m_Handedness( handedness )
    , m_vFoV( 45.0f )
    , m_AspectRatio( 1.0f )
    , m_zNear( 0.1f )
    , m_zFar( 100.0f )
{
    pData = (AlignedData*)_aligned_malloc( sizeof(AlignedData), 16 );
    if ( pData == NULL )
    {
        MessageBoxA( nullptr, "The data is NULL?!", "Error", MB_OK|MB_ICONERROR );
    }
    pData->m_Translation = XMVectorZero();
    pData->m_Rotation = XMQuaternionIdentity();
}

Camera::~Camera()
{
    _aligned_free(pData);
}

void Camera::set_Viewport( D3D11_VIEWPORT viewport )
{
    m_Viewport = viewport;
}

D3D11_VIEWPORT Camera::get_Viewport() const
{
    return m_Viewport;
}

void XM_CALLCONV Camera::set_LookAt( FXMVECTOR eye, FXMVECTOR target, FXMVECTOR up )
{
    switch ( m_Handedness )
    {
    case LeftHanded:
        {
            pData->m_ViewMatrix = XMMatrixLookAtLH( eye, target, up );
        }
        break;
    case RightHanded:
        {
            pData->m_ViewMatrix = XMMatrixLookAtRH( eye, target, up );
        }
        break;
    }

    pData->m_Translation = eye;
    pData->m_Rotation = XMQuaternionRotationMatrix( XMMatrixTranspose(pData->m_ViewMatrix) );

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

XMMATRIX Camera::get_ViewMatrix() const
{
    if ( m_ViewDirty )
    {
        UpdateViewMatrix();
    }
    return pData->m_ViewMatrix;
}

XMMATRIX Camera::get_InverseViewMatrix() const
{
    if ( m_InverseViewDirty )
    {
        pData->m_InverseViewMatrix = XMMatrixInverse( nullptr, pData->m_ViewMatrix );
        m_InverseViewDirty = false;
    }

    return pData->m_InverseViewMatrix;
}

void Camera::set_Projection( float fovy, float aspect, float zNear, float zFar )
{
    m_vFoV = fovy;
    m_AspectRatio = aspect;
    m_zNear = zNear;
    m_zFar = zFar;

    m_ProjectionDirty = true;
    m_InverseProjectionDirty = true;
}

XMMATRIX Camera::get_ProjectionMatrix() const
{
    if ( m_ProjectionDirty )
    {
        UpdateProjectionMatrix();
    }

    return pData->m_ProjectionMatrix;
}

XMMATRIX Camera::get_InverseProjectionMatrix() const
{
    if ( m_InverseProjectionDirty )
    {
        UpdateInverseProjectionMatrix();
    }

    return pData->m_InverseProjectionMatrix;
}

void Camera::set_Translation( FXMVECTOR translation )
{
    pData->m_Translation = translation;
    m_ViewDirty = true;
}

XMVECTOR Camera::get_Translation() const
{
    return pData->m_Translation;
}

void Camera::set_Rotation( FXMVECTOR rotation )
{
    pData->m_Rotation = rotation;
}

XMVECTOR Camera::get_Rotation() const
{
    return pData->m_Rotation;
}

void XM_CALLCONV Camera::Translate( FXMVECTOR translation, Space space )
{
    switch ( space )
    {
    case LocalSpace:
        {
            pData->m_Translation += XMVector3Rotate( translation, pData->m_Rotation );
        }
        break;
    case WorldSpace:
        {
            pData->m_Translation += translation;
        }
        break;
    }

    pData->m_Translation = XMVectorSetW( pData->m_Translation, 1.0f );

    m_ViewDirty = true;
    m_InverseViewDirty = true;
}

void Camera::Rotate( FXMVECTOR quaternion )
{
    pData->m_Rotation = XMQuaternionMultiply( pData->m_Rotation, quaternion );

    m_ViewDirty = true;
    m_InverseViewDirty = true;
}

void Camera::UpdateViewMatrix() const
{
    XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion( pData->m_Rotation ));
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector( -(pData->m_Translation) );

    pData->m_ViewMatrix = translationMatrix * rotationMatrix;

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

void Camera::UpdateInverseViewMatrix() const
{
    if ( m_ViewDirty )
    {
        UpdateViewMatrix();
    }

    pData->m_InverseViewMatrix = XMMatrixInverse( nullptr, pData->m_ViewMatrix );
    m_InverseViewDirty = false;
}

void Camera::UpdateProjectionMatrix() const
{
    switch( m_Handedness )
    {
    case LeftHanded:
        {
            pData->m_ProjectionMatrix = XMMatrixPerspectiveFovLH( XMConvertToRadians(m_vFoV), m_AspectRatio, m_zNear, m_zFar );
        }
        break;
    case RightHanded:
        {
            pData->m_ProjectionMatrix = XMMatrixPerspectiveFovRH( XMConvertToRadians(m_vFoV), m_AspectRatio, m_zNear, m_zFar );
        }
        break;
    }

    m_ProjectionDirty = false;
    m_InverseProjectionDirty = true;
}

void Camera::UpdateInverseProjectionMatrix() const
{
    if ( m_ProjectionDirty )
    {
        UpdateProjectionMatrix();
    }

    pData->m_InverseProjectionMatrix = XMMatrixInverse( nullptr, pData->m_ProjectionMatrix );
    m_InverseProjectionDirty = false;
}
