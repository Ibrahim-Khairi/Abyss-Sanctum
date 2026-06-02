#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3  viewPos;
uniform vec3  objectColor;
uniform float specularStrength;
uniform float shininess;

uniform vec3  fogColor;
uniform float fogDensity;

uniform bool  isEmissive;
uniform float emissiveStrength;

#define MAX_LIGHTS 12
uniform vec3  lightPositions[MAX_LIGHTS];
uniform vec3  lightColors[MAX_LIGHTS];
uniform float lightIntensities[MAX_LIGHTS];
uniform int   numLights;

uniform bool useProceduralTexture;
uniform bool isBone;
uniform float time;

uniform bool useTexture;
uniform sampler2D textureSampler;

// NEW — much softer falloff for a large cave
const float ATT_CONSTANT  = 1.3;
const float ATT_LINEAR    = 0.13;
const float ATT_QUADRATIC = 0.048;

// ─────────────────────────────────────────────────────────────────────
// PROCEDURAL ROCK TEXTURE
//
// hash() — takes a 2D point, returns a pseudo-random float 0-1
// This is a standard hash function used in shader noise generation.
// The magic numbers are chosen to scatter values evenly.
// ─────────────────────────────────────────────────────────────────────
float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

// smoothNoise() — bilinear interpolation between 4 hash values
// Creates smooth bumpy noise from the hash grid.
// fract(p) = position within current cell
// floor(p) = which cell we're in
float smoothNoise(vec2 p) {
    vec2 i = floor(p);   // integer cell
    vec2 f = fract(p);   // position within cell 0-1

    // Smoothstep curve: makes transitions between cells smooth
    // instead of linear (avoids blocky grid look)
    vec2 u = f * f * (3.0 - 2.0 * f);

    // Sample 4 corners of the cell and interpolate
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x),
               mix(c, d, u.x), u.y);
}

// fbm() — Fractional Brownian Motion
// Layers multiple octaves of noise at increasing frequency.
// Each octave: double the frequency (amplitude), halve the amplitude.
// Result: large shapes with fine detail on top — exactly like rock.
float fbm(vec2 p) {
    float value     = 0.0;
    float amplitude = 0.5;   // start at half contribution
    float frequency = 1.0;

    // 5 octaves — good balance of detail vs performance
    for (int i = 0; i < 5; i++) {
        value     += amplitude * smoothNoise(p * frequency);
        frequency *= 2.1;    // slightly non-power-of-2 avoids repetition
        amplitude *= 0.5;    // each octave contributes half as much
    }
    return value;
}

// ─────────────────────────────────────────────────────────────────────
// getCaustics()
// Simulates underwater light caustic patterns.
//
// How it works:
// Two layers of animated fbm noise, rotating slowly in opposite
// directions. Where both layers are bright simultaneously, we get
// a bright caustic spot. The result is multiplied into the lighting.
//
// time = glfwGetTime() passed as uniform
// ─────────────────────────────────────────────────────────────────────

float getCaustics(vec3 fragPos, vec3 normal) {
    // Only apply caustics to surfaces that roughly face upward
    // (floor gets strongest caustics, walls get softer)
    // dot(normal, up) = 1 on floor, 0 on walls, -1 on ceiling
    float upFacing = abs(dot(normalize(normal), vec3(0.0, 1.0, 0.0)));
    float wallFactor = 0.3 + upFacing * 0.7; // walls get 0.3, floor gets 1.0

    // Use world XZ position as base UV for caustics
    // Scale controls the size of caustic patterns
    vec2 uv = fragPos.xz * 0.25;

    // Layer 1 — slow rotation clockwise
    float s1 = sin(time * 0.3);
    float c1 = cos(time * 0.3);
    vec2 uv1 = vec2(uv.x * c1 - uv.y * s1,
                    uv.x * s1 + uv.y * c1);

    // Layer 2 — faster rotation counter-clockwise
    float s2 = sin(-time * 0.5);
    float c2 = cos(-time * 0.5);
    vec2 uv2 = vec2(uv.x * c2 - uv.y * s2,
                    uv.x * s2 + uv.y * c2);

    // Offset uvs over time so pattern drifts
    uv1 += vec2(time * 0.04, time * 0.03);
    uv2 += vec2(-time * 0.03, time * 0.05);

    // Sample noise at both layers
    float n1 = fbm(uv1 * 3.0);
    float n2 = fbm(uv2 * 3.0);

    // Caustic = bright where BOTH layers are bright simultaneously
    // pow sharpens the bright spots into thin lines
    float caustic = pow(n1 * n2 * 2.5, 2.5);

    // Also add Y-based variation — caustics ripple vertically too
    float yRipple = fbm(vec2(fragPos.y * 0.3 + time * 0.2,
                             fragPos.x * 0.2));
    caustic *= (0.7 + yRipple * 0.3);

    return caustic * wallFactor;
}

// ─────────────────────────────────────────────────────────────────────
// getRockColor()
// Uses FragPos (world position) as UV — this means the texture is
// projected from world space, not UV mapped. Advantage: no seams
// at panel joins, texture flows continuously across all surfaces.
// ─────────────────────────────────────────────────────────────────────
vec3 getRockColor(vec3 fragPos, vec3 normal) {

    // Use world XZ for floors/ceilings, XY or YZ for walls
    // based on which axis the normal points along
    vec2 texCoord;
    float nx = abs(normal.x);
    float ny = abs(normal.y);
    float nz = abs(normal.z);

    if (ny > nx && ny > nz) {
        // Floor or ceiling — project from above (XZ plane)
        texCoord = fragPos.xz * 0.15;
    } else if (nx > nz) {
        // Left or right wall — project from side (YZ plane)
        texCoord = fragPos.yz * 0.15;
    } else {
        // Front or back wall — project from front (XY plane)
        texCoord = fragPos.xy * 0.15;
    }

    // Large scale shape — the big rock formations
    float largeNoise = fbm(texCoord * 1.0);

    // Medium detail — patches and blotches
    float medNoise   = fbm(texCoord * 2.5 + vec2(3.7, 1.9));

    // Fine grain — surface micro-texture
    float fineNoise  = fbm(texCoord * 6.0 + vec2(1.2, 8.4));

    // Combine: large shapes dominate, fine detail adds surface interest
    float combined = largeNoise * 0.55
                   + medNoise   * 0.30
                   + fineNoise  * 0.15;

    // Dark brown-grey cave rock colour palette
    // Wet cave rock: very dark, slightly purple-brown
    // We lerp between two rock colours based on noise value
    vec3 darkRock  = vec3(0.08, 0.07, 0.06);  // #141210 — near black crevices
    vec3 midRock   = vec3(0.10, 0.09, 0.07);  // #1A1613 — your target base
    vec3 lightRock = vec3(0.15, 0.12, 0.09);  // #261F17 — raised rock catches light

    vec3 rockColor;
    if (combined < 0.5) {
        rockColor = mix(darkRock, midRock,   combined * 2.0);
    } else {
        rockColor = mix(midRock,  lightRock, (combined - 0.5) * 2.0);
    }

    // Add subtle wetness variation — underwater rock has dark wet patches
    float wetness = fbm(texCoord * 3.0 + vec2(5.5, 2.1));
    vec3 warmTint  = vec3(1.08, 1.02, 0.94); // slightly warm highlight
    vec3 coolTint  = vec3(0.92, 0.94, 0.98); // slightly cool shadow
    rockColor *= mix(coolTint, warmTint, wetness * 0.85 + 0.15);

    return rockColor;
}

// ─────────────────────────────────────────────────────────────────────
// Point light calculation — unchanged
// ─────────────────────────────────────────────────────────────────────
vec3 calcPointLight(vec3 lightPos, vec3 lColor, float intensity,
                    vec3 norm, vec3 fragPos, vec3 viewDir) {
    vec3  lightDir    = normalize(lightPos - fragPos);
    float dist        = length(lightPos - fragPos);
    float attenuation = intensity / (
        ATT_CONSTANT  +
        ATT_LINEAR    * dist +
        ATT_QUADRATIC * dist * dist
    );
    float diff    = abs(dot(norm, lightDir));
    vec3  diffuse = diff * lColor * attenuation;

    vec3  reflectDir = reflect(-lightDir, norm);
    float spec       = pow(max(abs(dot(viewDir, reflectDir)), 0.0), shininess);
    vec3  specular   = specularStrength * spec * lColor * attenuation;

    return diffuse + specular;
}

// ─────────────────────────────────────────────────────────────────────
// getBoneColor()
// Procedural fossilised bone material.
//
// Base: warm beige/tan
// Variation 1: darker pitted areas (porous bone surface)
// Variation 2: green-grey moss/algae patches in crevices
// Both driven by FBM noise using world position as UV.
// ─────────────────────────────────────────────────────────────────────
vec3 getBoneColor(vec3 fragPos, vec3 normal) {
    // Use all three axes for bone texture — it's a 3D object
    vec2 uv = fragPos.xy * 0.8 + fragPos.z * 0.3;

    // Large scale surface variation
    float largePit  = fbm(uv * 1.2);
    // Fine surface pitting — porous bone detail
    float finePit   = fbm(uv * 4.5 + vec2(2.3, 7.1));
    // Micro roughness
    float microRough = fbm(uv * 9.0 + vec2(5.5, 1.2));

    float combined = largePit  * 0.50
                   + finePit   * 0.35
                   + microRough * 0.15;

    // Bone colour palette
    vec3 boneLight = vec3(0.80, 0.74, 0.60);  // warm beige highlight
    vec3 boneMid   = vec3(0.65, 0.58, 0.44);  // tan mid tone
    vec3 boneDark  = vec3(0.42, 0.36, 0.26);  // dark brown crevice

    vec3 boneColor;
    if (combined < 0.4)
        boneColor = mix(boneDark, boneMid, combined * 2.5);
    else
        boneColor = mix(boneMid, boneLight, (combined - 0.4) * 1.67);

    // ── Moss/algae patches ────────────────────────────────────────────
    // Moss grows in crevices (dark areas) and sheltered spots.
    // Use a separate noise layer for moss distribution.
    float mossNoise  = fbm(uv * 2.1 + vec2(11.3, 4.7));
    float mossNoise2 = fbm(uv * 5.0 + vec2(3.1, 9.2));
    float mossMask   = smoothstep(0.55, 0.72,
                                  mossNoise * 0.6 + mossNoise2 * 0.4);

    // Moss prefers dark bone areas — crevices collect algae
    mossMask *= (1.0 - combined) * 1.5;
    mossMask  = clamp(mossMask, 0.0, 0.6);

    // Underwater moss: dark green-grey, slightly desaturated
    vec3 mossColor = vec3(0.18, 0.22, 0.16);

    boneColor = mix(boneColor, mossColor, mossMask);

    // ── Ambient occlusion approximation ──────────────────────────────
    // Darken areas where combined noise is low (crevices)
    float ao = 0.5 + combined * 0.5;
    boneColor *= ao;

    return boneColor;
}

void main() {
    float dist      = length(viewPos - FragPos);
    float fogFactor = clamp(exp(-fogDensity * dist), 0.0, 1.0);

    vec3 result;

    if (isEmissive) {
        // Crystal — just glow
        result = objectColor * emissiveStrength;

    } else {
        vec3 norm     = normalize(Normal);
        vec3 viewDir  = normalize(viewPos - FragPos);

        vec3 surfaceColor;
        if (isBone) {
            surfaceColor = getBoneColor(FragPos, norm);
        } else if (useProceduralTexture) {
            surfaceColor = getRockColor(FragPos, norm);
        } else if (useTexture) {
            surfaceColor = texture(textureSampler, TexCoord).rgb;
        } else {
            surfaceColor = objectColor;
        }

        result = vec3(0.10, 0.06, 0.05) * surfaceColor;

        vec3 lightAccum = vec3(0.0);
        for (int i = 0; i < numLights; i++) {
            lightAccum += calcPointLight(
                lightPositions[i],
                lightColors[i],
                lightIntensities[i],
                norm, FragPos, viewDir
            );
        }

        // ── Caustics — animated water light patterns ──────────────────
        if (useProceduralTexture) {
            float caustic = getCaustics(FragPos, norm);
            // Caustic colour — slightly blue-white, like sunlight through water
            vec3 causticColor = vec3(0.3, 0.55, 0.8) * caustic * 0.4;
            result += causticColor;
        }

        float lightGrey    = dot(lightAccum, vec3(0.299, 0.587, 0.114));
        vec3  neutralLight = mix(lightAccum, vec3(lightGrey), 0.65);
        result += surfaceColor * neutralLight;
    }

    result = mix(fogColor, result, fogFactor);
    FragColor = vec4(result, 1.0);
}