#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in float fragHeight;

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

struct WaterWave {
    vec2 direction;
    float frequency;
    float amplitude;
    float steepness;
    float speed;
};

layout(set = 1, binding = 0) uniform OceanUbo {
    WaterWave waves[4];
    vec4 sunDirection;
    vec4 horizonColor;
    vec4 skyColor;
    vec4 deepColor;
} oceanUbo;

layout(set = 2, binding = 0) uniform sampler2D skySampler;

#define PI 3.14159265359

void main() {
    vec3 N = normalize(fragNormal);
    vec3 cameraPos = ubo.inverseView[3].xyz;
    vec3 V = normalize(cameraPos - fragWorldPos);

    float fresnel = 0.02 + 0.98 * pow(1.0 - abs(dot(N, V)), 5.0);

    vec3 R = reflect(-V, N);
    vec2 uv = vec2(atan(R.z, R.x) / (2.0 * PI) + 0.5, asin(clamp(R.y, -1.0, 1.0)) / PI + 0.5);
    vec3 reflectedColor = texture(skySampler, uv).rgb;

    vec3 sunDir = normalize(oceanUbo.sunDirection.xyz);
    vec3 H = normalize(sunDir + V);
    float sunSpec = pow(max(dot(N, H), 0.0), 512.0);
    vec3 sunColor = vec3(1.0, 0.98, 0.92);

    vec3 specularLight = vec3(0.0);
    for (int i = 0; i < ubo.numLights; i++) {
        vec3 L = ubo.pointlights[i].position.xyz - fragWorldPos;
        float attenuation = 1.0 / dot(L, L);
        L = normalize(L);
        vec3 intensity = ubo.pointlights[i].color.xyz * ubo.pointlights[i].color.w * attenuation;
        vec3 halfAngle = normalize(L + V);
        float spec = pow(max(dot(N, halfAngle), 0.0), 256.0);
        specularLight += intensity * spec;
    }

    vec3 waterColor = mix(oceanUbo.deepColor.rgb, reflectedColor, fresnel);
    vec3 litColor = waterColor + sunSpec * sunColor + specularLight;

    float dist = length(cameraPos - fragWorldPos);
    float fogFactor = clamp((ubo.fogFar - dist) / (ubo.fogFar - ubo.fogNear), 0.0, 1.0);
    vec3 finalColor = mix(ubo.fogColor.rgb, litColor, fogFactor);

    outColor = vec4(finalColor, 1.0);
}
