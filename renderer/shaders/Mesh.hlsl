cbuffer CameraConstants : register(b0)
{
    row_major float4x4 viewProjection;
};

cbuffer MeshConstants : register(b1)
{
    row_major float4x4 world;
    row_major float4x4 normalWorld;
    float4 color;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float3 worldNormal : NORMAL0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    const float4 worldPosition = mul(float4(input.position, 1.0f), world);
    output.position = mul(worldPosition, viewProjection);
    output.worldNormal = normalize(mul(float4(input.normal, 0.0f), normalWorld).xyz);
    output.color = color;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    const float3 lightDirection = normalize(float3(0.45f, 0.85f, 0.30f));
    const float diffuse = saturate(dot(input.worldNormal, lightDirection));
    const float lighting = 0.48f + 0.52f * diffuse;
    return float4(input.color.rgb * lighting, input.color.a);
}
