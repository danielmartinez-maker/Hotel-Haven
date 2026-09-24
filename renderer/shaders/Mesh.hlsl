cbuffer CameraConstants : register(b0)
{
    row_major float4x4 viewProjection;
    float4 cameraWorldPosition;
};

cbuffer MeshConstants : register(b1)
{
    row_major float4x4 world;
    row_major float4x4 normalWorld;
    float4 color;
    // x = metallic, y = roughness. Remaining components are reserved.
    float4 materialParameters;
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
    float3 worldPosition : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    const float4 worldPosition = mul(float4(input.position, 1.0f), world);
    output.position = mul(worldPosition, viewProjection);
    output.worldPosition = worldPosition.xyz;
    output.worldNormal = normalize(mul(float4(input.normal, 0.0f), normalWorld).xyz);
    output.color = color;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    const float metallic = saturate(materialParameters.x);
    const float roughness = clamp(materialParameters.y, 0.04f, 1.0f);

    const float3 normal = normalize(input.worldNormal);
    const float3 lightDirection = normalize(float3(0.45f, 0.85f, 0.30f));
    const float3 fillDirection = normalize(float3(-0.55f, 0.38f, -0.42f));
    const float3 viewDirection = normalize(cameraWorldPosition.xyz - input.worldPosition);
    const float3 halfVector = normalize(lightDirection + viewDirection);

    const float ndotl = saturate(dot(normal, lightDirection));
    const float fill = saturate(dot(normal, fillDirection));
    const float diffuseLighting = 0.34f + 0.56f * ndotl + 0.10f * fill;

    // Metals contribute less diffuse energy and take their specular hue from
    // the authored base color. Rough surfaces broaden and suppress highlights.
    const float3 diffuseColor =
        input.color.rgb * diffuseLighting * lerp(1.0f, 0.58f, metallic);
    const float specularPower = lerp(96.0f, 7.0f, roughness);
    const float specularMask =
        pow(saturate(dot(normal, halfVector)), specularPower) *
        lerp(0.055f, 0.46f, metallic) *
        lerp(1.0f, 0.35f, roughness);
    const float3 specularColor =
        lerp(float3(0.92f, 0.94f, 0.96f), input.color.rgb, metallic);

    const float3 shaded = saturate(diffuseColor + specularColor * specularMask);
    return float4(shaded, input.color.a);
}
