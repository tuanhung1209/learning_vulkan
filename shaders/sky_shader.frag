#version 450

layout(location = 0) in vec3 fragDir;

layout(location = 0) out vec4 outColor;

struct PointLight{
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
    PointLight pointlights[10];
    int numLights;
    vec4 fogColor;
    float fogNear;
    float fogFar;
    vec4 horizonColor;
    vec4 skyColor;
} ubo;

float algebraicSigmoid(float x, float a, float b, float c) {
    return (a * x - b) / sqrt(c + pow(a * x - b, 2.0));
}

float horizonCurve(float x, float blendFactor, float blendHeight) {
    return 0.5 * algebraicSigmoid(x, 1.0, blendHeight, blendFactor) + 0.5;
}

void main(){
    float texY = -normalize(fragDir).y * 0.5 + 0.5; 
    float horizonHandle = horizonCurve(texY, 0.005, 0.57);

    vec3 finalColor = mix(ubo.horizonColor.rgb, ubo.skyColor.rgb, horizonHandle);
    outColor = vec4(finalColor, 1.0);
}
