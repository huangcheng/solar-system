#version 330 core
// Earth surface: photoreal day map + night city lights + a soft real-time
// terminator. UV is computed PER FRAGMENT (not interpolated) so the longitude
// wrap (date line) lands on identical texture columns and never smears.
in vec3 vLocal;   // object/ECEF unit-sphere position (== geometric normal)
out vec4 FragColor;

uniform sampler2D uDay;
uniform sampler2D uNight;
uniform vec3  uSunDir;     // unit, ECEF
uniform float uHasDay;
uniform float uHasNight;

vec2 sphereToUV(vec3 n) {
    float lon = atan(n.y, n.x);
    float lat = asin(clamp(n.z, -1.0, 1.0));
    return vec2((lon / 3.14159265 + 1.0) * 0.5,
                (lat / 1.5707963 + 1.0) * 0.5);
}

void main() {
    vec3 n = normalize(vLocal);
    vec2 uv = sphereToUV(n);
    vec3 sun = normalize(uSunDir);

    float cosSun = dot(n, sun);
    // Broad, soft day/night transition so there's no hard border at the terminator.
    float dayFactor = smoothstep(-0.45, 0.55, cosSun);

    vec3 day   = (uHasDay   > 0.5) ? texture(uDay,   uv).rgb : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);

    // Faint ambient on the dark side (earthshine / airglow) so the night side
    // isn't near-black; this drops the day/night contrast and softens the edge.
    vec3 darkSide = night * 0.8 + day * 0.05 + vec3(0.012, 0.018, 0.035);
    vec3 color = mix(darkSide, day, dayFactor);

    // broad warm twilight belt centered on the terminator (cosSun ~ 0)
    float twi = smoothstep(-0.45, 0.0, cosSun) * (1.0 - smoothstep(0.0, 0.55, cosSun));
    color += vec3(0.30, 0.13, 0.0) * twi;

    FragColor = vec4(color, 1.0);
}
