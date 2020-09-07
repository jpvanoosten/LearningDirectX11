/**
 *   @brief A mesh class that can be used to draw geometry to the screen.
 */
#pragma once

#include <memory>

// Vertex struct holding position, normal vector, and texture mapping information.
struct VertexPositionNormalTexture
{
    VertexPositionNormalTexture()
    { }

    VertexPositionNormalTexture( const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT2& textureCoordinate)
        : position(position),
        normal(normal),
        textureCoordinate(textureCoordinate)
    { }

    VertexPositionNormalTexture( DirectX::FXMVECTOR position, DirectX::FXMVECTOR normal, DirectX::FXMVECTOR textureCoordinate)
    {
        XMStoreFloat3(&this->position, position);
        XMStoreFloat3(&this->normal, normal);
        XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
    }

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 textureCoordinate;

    static const int InputElementCount = 3;
    static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

typedef std::vector<VertexPositionNormalTexture> VertexCollection;
typedef std::vector<uint16_t> IndexCollection;

class Mesh
{
public:
    
    void Draw( ID3D11DeviceContext* pDeviceContext );

    static std::unique_ptr<Mesh> CreateCube( ID3D11DeviceContext* deviceContext, float size = 1, bool rhcoords = true);
    static std::unique_ptr<Mesh> CreateSphere( ID3D11DeviceContext* deviceContext, float diameter = 1, size_t tessellation = 16, bool rhcoords = true);
    static std::unique_ptr<Mesh> CreateCone( ID3D11DeviceContext* deviceContext, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true);
    static std::unique_ptr<Mesh> CreateTorus( ID3D11DeviceContext* deviceContext, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true);

protected:

private:
    friend struct std::default_delete<Mesh>;

    Mesh();
    Mesh( const Mesh& copy );
    virtual ~Mesh();

    void Initialize( ID3D11DeviceContext* deviceContext, VertexCollection& vertices, IndexCollection& indices, bool rhcoords );
    
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_IndexBuffer;

    UINT m_IndexCount;
};