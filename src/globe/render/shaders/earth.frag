#version 330 core
// Earth surface: photoreal day/night maps + terrain relief (tangent-space normal
// map) + a subtle water-masked sun glint. Lighting is done in WORLD space so the
// normal, sun and view directions all share one frame — that consistency is what
// makes the relief shading and the specular half-vector correct.
//
// Two seam-avoidance measures (the reason normal mapping was disabled before):
//   * UV is recomputed PER FRAGMENT, so the longitude wrap (date line) lands on
//     identical texture columns and never smears.
//   * the tangent basis is built ANALYTICALLY from the sphere geometry, not from
//     screen-space derivatives (dFdx/dFdy of a wrapping UV blew up at the seam).
in vec3 vLocal;   // object-space unit-sphere position (== object normal)
in vec3 vWorld;   // world-space position
out vec4 FragColor;

uniform sampler2D uDay;
uniform sampler2D uNight;
uniform sampler2D uNormal;     // tangent-space relief (RGB = XYZ, +Z out)
uniform sampler2D uSpecular;   // water mask (bright = ocean)
uniform mat4  uModel;
uniform vec3  uSunWorld;       // unit sun direction, world space
uniform vec3  uViewPos;        // camera position, world space
uniform float uHasDay;
uniform float uHasNight;
uniform float uHasNormal;
uniform float uHasSpecular;

const float kReliefStrength = 2.2;   // how strongly the normal map tilts the normal
const float kReliefShade    = 1.7;   // how much a slope darkens/brightens the day map
const float kGlint          = 0.30;  // ocean sun-glint intensity (low = not glassy)

vec2 sphereToUV(vec3 n) {
    float lon = atan(n.y, n.x);
    float lat = asin(clamp(n.z, -1.0, 1.0));
    return vec2((lon / 3.14159265 + 1.0) * 0.5,
                (lat / 1.5707963 + 1.0) * 0.5);
}

void main() {
    vec3 nObj = normalize(vLocal);
    vec2 uv   = sphereToUV(nObj);
    vec3 nGeo = normalize(vWorld);          // geometric world normal
    vec3 sun  = normalize(uSunWorld);
    mat3 M3   = mat3(uModel);

    // --- terrain relief: perturb the world normal with the normal map --------
    vec3 N = nGeo;
    if (uHasNormal > 0.5) {
        // Analytic sphere tangent frame: east (dP/dlon) and north (dP/dlat),
        // built in object space then rotated into world. Continuous across the
        // date line; only degenerate exactly at the poles, handled below.
        vec3 axis = vec3(0.0, 0.0, 1.0);
        vec3 Tobj = cross(axis, nObj);
        float tl  = length(Tobj);
        Tobj = (tl > 1e-4) ? Tobj / tl : vec3(1.0, 0.0, 0.0);
        vec3 Bobj = cross(nObj, Tobj);
        vec3 Tw = normalize(M3 * Tobj);
        vec3 Bw = normalize(M3 * Bobj);
        vec3 nm = texture(uNormal, uv).xyz * 2.0 - 1.0;
        nm.xy *= kReliefStrength;
        N = normalize(mat3(Tw, Bw, nGeo) * nm);
    }

    float cosGeo = dot(nGeo, sun);
    // Soft terminator from the GEOMETRIC normal so relief never makes it jagged.
    float dayFactor = smoothstep(-0.10, 0.20, cosGeo);

    vec3 day   = (uHasDay   > 0.5) ? texture(uDay,   uv).rgb : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);

    // Relief shading: how much the bumped slope faces the sun vs the flat sphere.
    // Centered on 1.0 so flat ground is unchanged; the effect is strongest near
    // the terminator (grazing light), exactly where real relief stands out.
    float relief = clamp((dot(N, sun) - cosGeo) * kReliefShade + 1.0, 0.55, 1.45);
    day *= relief;

    // Faint earthshine/airglow so the night side isn't pure black.
    vec3 darkSide = night * 0.8 + day * 0.05 + vec3(0.012, 0.018, 0.035);
    vec3 color = mix(darkSide, day, dayFactor);

    // Warm dusk that hugs the terminator, faded at the grazing limb and over dark
    // ocean (so it reads as sunset on land, not a pink stain on the sea).
    float facing = dot(nGeo, normalize(uViewPos - vWorld));
    float limb   = smoothstep(0.18, 0.45, facing);
    float twi    = smoothstep(-0.14, 0.0, cosGeo) * (1.0 - smoothstep(0.0, 0.14, cosGeo));
    float dayLum = dot(day, vec3(0.299, 0.587, 0.114));
    color += vec3(0.20, 0.09, 0.025) * twi * limb * (0.25 + 0.75 * dayLum);

    // Ocean sun-glint: subtle, water-masked, day-side only — no glassy sheen.
    if (uHasSpecular > 0.5) {
        float water = texture(uSpecular, uv).r;
        vec3 viewDir = normalize(uViewPos - vWorld);
        vec3 H = normalize(sun + viewDir);
        float spec = pow(max(dot(N, H), 0.0), 70.0);
        color += vec3(0.7, 0.82, 1.0) * spec * water * dayFactor * kGlint;
    }

    FragColor = vec4(color, 1.0);
}
