/**
 * A DirectX camera class.
 */
#pragma once

class Camera
{
public:

    // When performing transformations on the camera, 
    // it is sometimes useful to express which space this 
    // transformation should be applied.
    enum Space
    {
        LocalSpace,
        WorldSpace,
    };

    // Express whether this is a left-handed coordinate system (commonly used for DirectX)
    // or a right-handed coordinate system (commonly used for OpenGL).
    enum Handedness
    {
        LeftHanded,
        RightHanded
    };

    Camera(Handedness handedness = LeftHanded);
    virtual ~Camera();

    void set_Viewport( D3D11_VIEWPORT viewport );
    D3D11_VIEWPORT get_Viewport() const;

    void XM_CALLCONV set_LookAt( DirectX::FXMVECTOR eye, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up );
    DirectX::XMMATRIX get_ViewMatrix() const;
    DirectX::XMMATRIX get_InverseViewMatrix() const;

    /**
     * Set the camera to a perspective projection matrix.
     * @param fovy The vertical field of view in degrees.
     * @param aspect The aspect ratio of the screen.
     * @param zNear The distance to the near clipping plane.
     * @param zFar The distance to the far clipping plane.
     */
    void set_Projection( float fovy, float aspect, float zNear, float zFar );
    DirectX::XMMATRIX get_ProjectionMatrix() const;
    DirectX::XMMATRIX get_InverseProjectionMatrix() const;

    /**
     * Set the camera's position in world-space.
     */
    void XM_CALLCONV set_Translation( DirectX::FXMVECTOR translation );
    DirectX::XMVECTOR get_Translation() const;

    /**
     * Set the camera's rotation in world-space.
     * @param rotation The rotation quaternion.
     */
    void set_Rotation( DirectX::FXMVECTOR rotation );
    /**
     * Query the camera's rotation.
     * @returns The camera's rotation quaternion.
     */
    DirectX::XMVECTOR get_Rotation() const;

    void XM_CALLCONV Translate( DirectX::FXMVECTOR translation, Space space = LocalSpace );
    void Rotate( DirectX::FXMVECTOR quaternion );

protected:
    virtual void UpdateViewMatrix() const;
    virtual void UpdateInverseViewMatrix() const;
    virtual void UpdateProjectionMatrix() const;
    virtual void UpdateInverseProjectionMatrix() const;

    // This data must be aligned otherwise the SSE intrinsics fail
    // and throw exceptions.
    __declspec(align(16)) struct AlignedData
    {
        // World-space position of the camera.
        DirectX::XMVECTOR m_Translation;
        // World-space rotation of the camera.
        // THIS IS A QUATERNION!!!!
        DirectX::XMVECTOR m_Rotation;

        DirectX::XMMATRIX m_ViewMatrix, m_InverseViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix, m_InverseProjectionMatrix;
    };
    AlignedData* pData;

    // projection parameters
    float m_vFoV;   // Vertical field of view.
    float m_AspectRatio; // Aspect ratio
    float m_zNear;      // Near clip distance
    float m_zFar;       // Far clip distance.

    D3D11_VIEWPORT m_Viewport;

    // True if the view matrix needs to be updated.
    mutable bool m_ViewDirty, m_InverseViewDirty;
    // True if the projection matrix needs to be updated.
    mutable bool m_ProjectionDirty, m_InverseProjectionDirty;

    Handedness m_Handedness;

private:

};