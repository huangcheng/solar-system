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
    float dayFactor = smoothstep(-0.10, 0.20, cosSun);

    vec3 day   = (uHasDay   > 0.5) ? texture(uDay,   uv).rgb : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);

    vec3 color = mix(night * 0.8, day, dayFactor);

    // warm twilight at the terminator
    float twi = dayFactor * (1.0 - smoothstep(0.18, 0.40, cosSun));
    color += vec3(0.18, 0.08, 0.0) * twi;

    FragColor = vec4(color, 1.0);
}
