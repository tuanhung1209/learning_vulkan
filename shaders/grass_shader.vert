#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out float fragHeightFactor;

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
    float fogDensity;
    float time;
} ubo;

struct GrassTransformData {
    vec4 translation;
    vec2 scale;
};

layout(std430, set = 1, binding = 0) readonly buffer GrassBuffer {
    GrassTransformData transforms[];
};

layout(std430, set = 1, binding = 2) readonly buffer VisibleGrassBuffer {
    GrassTransformData visibleGrass[];
};

layout(push_constant) uniform Push {
    uint gridSize;
    uint terrainResolution;
    float heightScale;
    float spacing;
    float bladeHeight;
    float windDirX;
    float windDirZ;
    float windFreq;
    float windAmplitude;
    float turbPower;
    float turbSize;
    float droopStrength;
    float xPeriod;
    float yPeriod;
    float windBias;
    vec4 baseColor;
    vec4 tipColor;
} push;

mat3 rotateY(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c, 0, s, 0, 1, 0, -s, 0, c);
}

// Same hash as compute shader
uint wang_hash(uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return seed;
}

uint rand_xorshift(uint state) {
    state ^= (state << 13u);
    state ^= (state >> 17u);
    state ^= (state << 5u);
    return state;
}

float hash(uint seed) {
    return float(rand_xorshift(wang_hash(seed))) / 4294967296.0;
}

// 3D simplex noise (adapted from webgl-noise by Ashima Arts)
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x * 34.0) + 10.0) * x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v) {
    const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    i = mod289(i);
    vec4 p = permute(permute(permute(
        i.z + vec4(0.0, i1.z, i2.z, 1.0))
      + i.y + vec4(0.0, i1.y, i2.y, 1.0))
      + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    float n_ = 0.142857142857;
    vec3 ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);
    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w;

    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
}

void main() {
    uint id = uint(gl_InstanceIndex);
    GrassTransformData t = visibleGrass[id];

    // Stable seed (deterministic per original grid slot, survives compaction)
    uint seed = uint(t.translation.w * 4294967296.0);
    float rotAngle = t.translation.w * 6.28318;

    // Per-blade random values
    float idHash = hash(seed * 31u + 37u);
    float swayVar = mix(0.8, 1.0, hash(seed * 41u + 43u));
    float droopVar = mix(0.5, 1.0, idHash);

    // --- Build local-space blade (like Acerola's ModelGrass) ---
    vec3 localPos = position;

    // Scale width and height
    localPos *= vec3(t.scale.x, t.scale.y, t.scale.x);

    // Rotate blade around Y by random angle
    localPos = rotateY(rotAngle) * localPos;
    localPos.y = -localPos.y;

    // Per-blade droop in a random direction (not wind direction)
    float droopAngle = hash(seed * 13u + 7u) * 6.28318;
    vec2 droopDir = vec2(cos(droopAngle), sin(droopAngle));
    float bendAmount = position.y * position.y;
    float droopStrength = push.droopStrength * droopVar * t.scale.y;
    localPos.x += droopDir.x * droopStrength * bendAmount;
    localPos.z += droopDir.y * droopStrength * bendAmount;

    // --- Wind flow field (sampled at blade ROOT, not per-vertex) ---
    vec3 rootPos = t.translation.xyz;

    // Per-blade wind direction variation
    vec2 baseWindDir = normalize(vec2(push.windDirX, push.windDirZ));
    float windAngleVar = (hash(seed * 53u + 59u) - 0.5) * 0.6; // +-0.3 radians (~17 degrees)
    float cw = cos(windAngleVar);
    float sw = sin(windAngleVar);
    vec2 windDir = vec2(baseWindDir.x * cw - baseWindDir.y * sw,
                        baseWindDir.x * sw + baseWindDir.y * cw);

    float xyValue = rootPos.x * push.xPeriod + rootPos.z * push.yPeriod
                  + push.turbPower * snoise(vec3(rootPos.xz * push.turbSize, 0.0));
    float rawWind = sin((xyValue + ubo.time) * push.windFreq);

    // Biased remap: spends more time bent over, less time upright
    float windStrength = (rawWind * (1.0 - push.windBias) + push.windBias) * push.windAmplitude * swayVar;

    // Wind displacement — applied uniformly to all vertices of this blade
    localPos.x += windStrength * bendAmount * windDir.x * t.scale.y;
    localPos.z += windStrength * bendAmount * windDir.y * t.scale.y;

    // --- Final world position ---
    vec3 worldPos = localPos + rootPos;

    gl_Position = ubo.projection * ubo.view * vec4(worldPos, 1.0);

    // Pass data to fragment shader
    float heightFactor = clamp(position.y, 0.0, 1.0);
    fragColor = mix(push.baseColor.rgb, push.tipColor.rgb, heightFactor);
    fragPosWorld = worldPos;
    fragHeightFactor = heightFactor;
}
