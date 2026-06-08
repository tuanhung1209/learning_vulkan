#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out float fragHeight;

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

vec3 gerstnerDisplacement(vec3 p, WaterWave w) {
    float theta = dot(w.direction, p.xz) * w.frequency + ubo.time * w.speed;
    float c = cos(theta);
    return vec3(
        w.steepness * w.amplitude * w.direction.x * c,
        w.amplitude * sin(theta),
        w.steepness * w.amplitude * w.direction.y * c
    );
}

vec3 gerstnerNormal(vec3 p, WaterWave w) {
    float theta = dot(w.direction, p.xz) * w.frequency + ubo.time * w.speed;
    float wa = w.frequency * w.amplitude;
    return vec3(
        w.direction.x * wa * cos(theta),
        w.steepness * wa * sin(theta),
        w.direction.y * wa * cos(theta)
    );
}

void main() {
    vec3 worldPos = inPosition;
    vec3 N = vec3(0.0, 1.0, 0.0);

    for (int i = 0; i < 4; i++) {
        worldPos += gerstnerDisplacement(worldPos, oceanUbo.waves[i]);
        N += gerstnerNormal(worldPos, oceanUbo.waves[i]);
    }
    N = normalize(N);

    gl_Position = ubo.projection * ubo.view * vec4(worldPos, 1.0);

    fragWorldPos = worldPos;
    fragNormal   = N;
    fragTexCoord = inTexCoord;
    fragHeight   = worldPos.y;
}
