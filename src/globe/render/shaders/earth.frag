#version 330 core
in vec3 vLocal;
in vec3 vWorld;
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uDay;
uniform sampler2D uNight;
uniform sampler2D uNormal;    // tangent-space bump (terrain relief)
uniform sampler2D uSpecular;  // water mask for ocean sun-glint
uniform vec3  uSunDir;        // unit, world space (ECEF)
uniform vec3  uViewPos;       // world-space camera position
uniform float uHasDay;
uniform float uHasNight;
uniform float uHasNormal;
uniform float uHasSpecular;

void main() {
    vec3 baseN = normalize(vWorld);          // geometric world normal (unit sphere)
    vec2 uv = vUV;
    vec3 sun = normalize(uSunDir);

    // Perturb the normal with the tangent-space normal map. Tangent basis is
    // built from screen-space derivatives (no precomputed tangents), so the
    // relief aligns to the actual UV mapping of the sphere.
    vec3 N = baseN;
    if (uHasNormal > 0.5) {
        vec3 dp1 = dFdx(vWorld);
        vec3 dp2 = dFdy(vWorld);
        vec2 duv1 = dFdx(uv);
        vec2 duv2 = dFdy(uv);
        vec3 dp2perp = cross(baseN, dp2);
        vec3 dp1perp = cross(dp1, baseN);
        vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
        vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
        float det = max(dot(T, T), dot(B, B));
        if (det > 1e-6) {
            float invmax = inversesqrt(det);
            mat3 TBN = mat3(T * invmax, B * invmax, baseN);
            vec3 bumped = normalize(TBN * (texture(uNormal, uv).xyz * 2.0 - 1.0));
            // Blend with the geometric normal so relief stays subtle and no
            // hard seam (e.g. at the equator/limb) can appear.
            N = normalize(mix(baseN, bumped, 0.5));
        }
    }

    float cosSun = dot(N, sun);
    float dayFactor = smoothstep(-0.10, 0.20, cosSun);

    vec3 day   = (uHasDay   > 0.5) ? texture(uDay,   uv).rgb : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);

    vec3 color = mix(night * 0.8, day, dayFactor);

    // warm twilight tint near the terminator
    float twi = dayFactor * (1.0 - smoothstep(0.18, 0.40, cosSun));
    color += vec3(0.18, 0.08, 0.0) * twi;

    // ocean specular glint (Blinn-Phong) masked by water
    if (uHasSpecular > 0.5 && dayFactor > 0.05) {
        float water = texture(uSpecular, uv).r;
        vec3 viewDir = normalize(uViewPos - vWorld);
        vec3 H = normalize(sun + viewDir);
        float spec = pow(max(dot(N, H), 0.0), 60.0) * water * dayFactor;
        color += vec3(0.9, 0.95, 1.0) * spec;
    }

    FragColor = vec4(color, 1.0);
}
