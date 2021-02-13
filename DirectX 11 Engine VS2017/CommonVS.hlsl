cbuffer cbPerVertex : register(b0)
{
    float4x4 wvp;
    uint enableSpherical;
}

struct VS_INPUT
{
    float3 inPos : POSITION;
    float4 inColor : COLOR;
    float2 inTexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 outPosition : SV_POSITION;
    float4 outColor : COLOR;
    float2 outTexCoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    const float pi = 3.14159265f;
    const float angle = pi/2;

    const float4x4 rotation = float4x4
    (
        cos(angle),  0, sin(angle), 0,
        0,           1, 0,          0,
        -sin(angle), 0, cos(angle), 0,
        0,           0, 0,          1
    );

    VS_OUTPUT output;

    const float inX = input.inPos.x;
    const float inY = input.inPos.y;
    const float inZ = input.inPos.z;
    const float r = inZ;
    const float phi = (inY/r);
    const float theta = (pi/2 - inX/r);

    if (enableSpherical != 0)
    {
        float4 projectToSphere;
        projectToSphere.x = r * cos(phi) * sin(theta);
        projectToSphere.y = r * sin(phi) * sin(theta);
        projectToSphere.z = r * cos(theta);
        projectToSphere.w = 1.0f;
        float4 rotatedProjectToSphere = mul(projectToSphere, rotation);
        output.outPosition = mul(rotatedProjectToSphere, wvp);
    }
    else
    {
        output.outPosition = mul(float4(input.inPos, 1.0f), wvp);
    }

    output.outColor = input.inColor;
    output.outTexCoord = input.inTexCoord;
    return output;
}