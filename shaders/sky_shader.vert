#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragDir;
layout(location = 1) out vec2 fragTexCoord;

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

void main(){
    mat4 viewNoTranslation = mat4(mat3(ubo.view));
    gl_Position = ubo.projection * viewNoTranslation * vec4(position, 1.0);

    fragTexCoord = vec2(uv.x, 1.0 - uv.y);
    fragDir = position;
}
