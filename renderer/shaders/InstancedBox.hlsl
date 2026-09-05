cbuffer CameraConstants : register(b0)
{
    row_major float4x4 viewProjection;
};

struct VSInput
{
    float3 position : POSITION;
    float4 world0 : WORLD0;
    float4 world1 : WORLD1;
    float4 world2 : WORLD2;
    float4 world3 : WORLD3;
    float4 color : COLOR0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    row_major float4x4 world = float4x4(
        input.world0,
        input.world1,
        input.world2,
        input.world3);

    const float4 worldPosition = mul(float4(input.position, 1.0f), world);
    output.position = mul(worldPosition, viewProjection);
    output.color = input.color;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return input.color;
}
