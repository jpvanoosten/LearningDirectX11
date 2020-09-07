struct PixelShaderInput
{
    float4 color : COLOR;
};

float4 SimplePixelShader( PixelShaderInput IN ) : SV_TARGET
{
    return IN.color;
}