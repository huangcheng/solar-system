#version 330 core
in vec3 vLocal;
in vec3 vWorld;
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uDay;
uniform sampler2D uNight;
uniform sampler2D uNormal;    // tangent-space bump (subtle terrain relief)
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

    // Base day/night lighting from the GEOMETRIC normal so the surface can
    // never go dark from a bad bump. This keeps the photoreal day map dominant.
    float cosGeo = dot(baseN, sun);
    float dayFactor = smoothstep(-0.10, 0.20, cosGeo);

    vec3 day   = (uHasDay   > 0.5) ? texture(uDay,   uv).rgb : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);

    vec3 color = mix(night * 0.8, day, dayFactor);

    // Subtle terrain relief: perturb the normal and apply only a gentle
    // directional modulation on top of the base lighting (degenerate-guarded,
    // blended with the geometric normal so no hard seam can form).
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
            bumped = normalize(mix(baseN, bumped, 0.6));
            float relief = dot(bumped, sun) - cosGeo;   // +/- around the base
            color *= 1.0 + 0.20 * relief;               // gentle shading only
        }
    }

    // warm twilight tint near the terminator
    float twi = dayFactor * (1.0 - smoothstep(0.18, 0.40, cosGeo));
    color += vec3(0.18, 0.08, 0.0) * twi;

    // Subtle ocean sun-glint (Blinn-Phong on the geometric normal, water only).
    if (uHasSpecular > 0.5 && dayFactor > 0.05) {
        float water = texture(uSpecular, uv).r;
        vec3 viewDir = normalize(uViewPos - vWorld);
        vec3 H = normalize(sun + viewDir);
        float spec = pow(max(dot(baseN, H), 0.0), 80.0) * water * dayFactor;
        color += vec3(0.6, 0.7, 0.85) * spec * 0.5;
    }

    FragColor = vec4(color, 1.0);
}
