#version 450

layout(location = 0) in vec3 fragDir;
layout(location = 1) in vec2 fragTexCoord;

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
    vec4 sunDirection;
    vec4 fogColor;
    float fogNear;
    float fogFar;
    float fogDensity;
    float time;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform Push {
    vec4 skyColor;
    vec4 skyTextureColor;
} push;

#define PI 3.14159265359

float algebraicSigmoid(float x, float a, float b, float c) {
    return (a * x - b) / sqrt(c + pow(a * x - b, 2.0));
}

float horizonCurve(float x, float blendFactor, float blendHeight) {
    return 0.5 * algebraicSigmoid(x, 1.0, blendHeight, blendFactor) + 0.5;
}

void main(){
    vec3 dir = normalize(fragDir);

    // scrolling texture
    float a1 = ubo.time * 0.01;
    float c1 = cos(a1), s1 = sin(a1);
    vec3 d1 = vec3(dir.x * c1 - dir.z * s1, dir.y, dir.x * s1 + dir.z * c1);

    float a2 = ubo.time * 0.006;
    float c2 = cos(a2), s2 = sin(a2);
    vec3 d2 = vec3(dir.x * c2 - dir.z * s2, dir.y, dir.x * s2 + dir.z * c2);

    // convert rotated directions to equirectangular UVs
    vec2 uv1 = vec2(atan(d1.z, d1.x) / (2.0 * PI) + 0.5, asin(clamp(d1.y, -1.0, 1.0)) / PI + 0.5);
    vec2 uv2 = vec2(atan(d2.z, d2.x) / (2.0 * PI) + 0.5, asin(clamp(d2.y, -1.0, 1.0)) / PI + 0.5);

    vec3 sample1 = texture(texSampler, uv1).rgb;
    vec3 sample2 = texture(texSampler, uv2).rgb;
    vec3 texColor = mix(sample1, sample2, 0.5);

    float texY = -dir.y * 0.5 + 0.5;
    float horizonHandle = horizonCurve(texY, 0.005, 0.57);

    // sky gradient
    vec3 skyColor = mix(ubo.fogColor.rgb, push.skyColor.rgb, horizonHandle);

    // sun
    vec3 sunDir = normalize(ubo.sunDirection.xyz);
    float sunAngle = dot(dir, sunDir);

    // hard sun disc
    float sunDisc = smoothstep(0.9990, 0.9995, sunAngle);
    vec3 sunColor = vec3(1.0, 1.0, 1.0);

    // soft glow around the sun
    float sunGlow = pow(max(sunAngle, 0.0), 32.0);
    vec3 glowColor = vec3(1.0, 1.0, 1.0);

    // wider warm haze near sun
    float sunHaze = pow(max(sunAngle, 0.0), 8.0);
    vec3 hazeColor = vec3(1.0, 1.0, 1.0);

    // tint the gradient by the clouds first...
    skyColor *= texColor;
    skyColor *= push.skyTextureColor.rgb;

    // ...then add the sun on top so the clouds don't dim it (no longer "behind")
    skyColor += sunDisc * sunColor * 2.0;
    skyColor += sunGlow * glowColor * 0.5;
    skyColor += sunHaze * hazeColor * 0.15;

    /*
    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    float dist = length(cameraPosWorld - fragPosWorld);
    ubo.fogDensity = 4.605 / ubo.fogFar;
    float fogFactor = clamp(exp(-pow(dist * ubo.fogDensity, 2.0)), 0.0, 1.0);
    vec3 finalColor = mix(ubo.fogColor.rgb, skyColor, fogFactor);
    */

    outColor = vec4(skyColor, 1.0);
}
