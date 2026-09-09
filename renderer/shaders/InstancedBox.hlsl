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
    float3 worldPosition : TEXCOORD0;
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
    output.worldPosition = worldPosition.xyz;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    // Derivative normals keep the instanced mesh format unchanged while making
    // furniture, walls and characters legible in the orthographic view.
    float3 normal = normalize(cross(ddx(input.worldPosition), ddy(input.worldPosition)));
    float lighting = 0.52 + 0.48 * dot(abs(normal), normalize(float3(0.45, 0.85, 0.30)));
    return float4(input.color.rgb * lighting, input.color.a);
}
