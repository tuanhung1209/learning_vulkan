#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in float fragHeightFactor;

layout(location = 0) out vec4 outColor;

struct PointLight {
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
    PointLight pointlights[10];
    int numLights;
    vec4 fogColor;
    float fogNear;
    float fogFar;
    float time;
} ubo;

void main() {
    // Directional light (sun)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    float NdotL = max(dot(vec3(0.0, 1.0, 0.0), lightDir), 0.0);

    vec3 ambient = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 diffuse = vec3(1.0) * NdotL * 0.8;

    // AO at base
    vec3 aoColor = vec3(0.01, 0.05, 0.005);
    vec3 ao = mix(aoColor, vec3(1.0), fragHeightFactor);

    // Tip brightening
    vec3 tipBoost = vec3(0.15, 0.2, 0.05) * fragHeightFactor * fragHeightFactor;

    vec3 grassColor = (fragColor + tipBoost) * ao * (ambient + diffuse);

    // Fog
    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    float dist = length(cameraPosWorld - fragPosWorld);
    float fogFactor = clamp((ubo.fogFar - dist) / (ubo.fogFar - ubo.fogNear), 0.0, 1.0);
    vec3 finalColor = mix(ubo.fogColor.rgb, grassColor, fogFactor);

    outColor = vec4(finalColor, 1.0);
}
