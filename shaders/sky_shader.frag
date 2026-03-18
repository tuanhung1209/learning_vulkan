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
    vec4 fogColor;
    float fogNear;
    float fogFar;
    vec4 horizonColor;
    vec4 skyColor;
    vec4 sunDirection;
    float time;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

float algebraicSigmoid(float x, float a, float b, float c) {
    return (a * x - b) / sqrt(c + pow(a * x - b, 2.0));
}

float horizonCurve(float x, float blendFactor, float blendHeight) {
    return 0.5 * algebraicSigmoid(x, 1.0, blendHeight, blendFactor) + 0.5;
}

void main(){
    float speed = 0.002;
    vec2 uv = fragTexCoord;
    uv.x = fract(uv.x + ubo.time * speed);
    vec3 texColor = texture(texSampler, uv).rgb;

    vec3 dir = normalize(fragDir);
    float texY = -dir.y * 0.5 + 0.5;
    float horizonHandle = horizonCurve(texY, 0.005, 0.57);

    // sky gradient
    vec3 finalColor = mix(ubo.horizonColor.rgb, ubo.skyColor.rgb, horizonHandle);

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

    finalColor += sunDisc * sunColor * 2.0;
    finalColor += sunGlow * glowColor * 0.5;
    finalColor += sunHaze * hazeColor * 0.15;
    finalColor *= texColor;

    // pink sky
    // finalColor *= vec3(0.941, 0.322, 0.875)

    outColor = vec4(finalColor, 1.0);
}
