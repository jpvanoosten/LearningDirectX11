cbuffer PerFrame : register( b0 )
{
    matrix ViewProjectionMatrix;
}

struct AppData
{
    // Per-vertex data
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    // Per-instance data
    matrix Matrix   : WORLDMATRIX;
    matrix InverseTranspose : INVERSETRANSPOSEWORLDMATRIX;
};

struct VertexShaderOutput
{
    float4 PositionWS   : TEXCOORD1;
    float3 NormalWS     : TEXCOORD2;
    float2 TexCoord     : TEXCOORD0;
    float4 Position     : SV_Position;
};

VertexShaderOutput InstancedVertexShader( AppData IN )
{
    VertexShaderOutput OUT;

    matrix MVP = mul( ViewProjectionMatrix, IN.Matrix );

    OUT.Position = mul( MVP, float4( IN.Position, 1.0f ) );
    OUT.PositionWS = mul( IN.Matrix, float4( IN.Position, 1.0f ) );
    OUT.NormalWS = mul( (float3x3)IN.InverseTranspose, IN.Normal );
    OUT.TexCoord = IN.TexCoord;

    return OUT;
}