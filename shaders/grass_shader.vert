#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

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
} ubo;

struct GrassTransformData {
    vec4 translation;
    vec2 scale;
};

layout(std430, set = 1, binding = 0) readonly buffer GrassBuffer {
    GrassTransformData transforms[];
};

mat3 rotateY(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c, 0, s, 0, 1, 0, -s, 0, c);
}

void main() {
    GrassTransformData t = transforms[gl_InstanceIndex];

    vec3 scaled = position * vec3(t.scale.x, -t.scale.y, t.scale.x);
    vec3 rotated = rotateY(t.translation.w) * scaled;
    vec3 worldPos = rotated + t.translation.xyz;

    gl_Position = ubo.projection * ubo.view * vec4(worldPos, 1.0);
    fragColor = vec3(0.1, 0.5 + position.y * 0.3, 0.1);
}
