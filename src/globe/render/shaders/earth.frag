#version 330 core
in vec3 vLocal;
in vec3 vWorld;
out vec4 FragColor;
uniform sampler2D uDay;
uniform sampler2D uNight;
uniform vec3  uSunDir;
uniform float uHasDay;
uniform float uHasNight;

vec2 sphereToUV(vec3 n) {
    float lon = atan(n.y, n.x);
    float lat = asin(clamp(n.z, -1.0, 1.0));
    return vec2((lon / 3.14159265 + 1.0) * 0.5, (lat / 1.5707963 + 1.0) * 0.5);
}

void main() {
    vec3 n = normalize(vLocal);
    vec2 uv = sphereToUV(n);

    float cosSun = dot(n, normalize(uSunDir));
    float dayFactor = smoothstep(-0.10, 0.20, cosSun);

    vec3 day   = (uHasDay   > 0.5) ? texture(uDay,   uv).rgb : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);

    vec3 color = mix(night * 0.8, day, dayFactor);
    float twi = smoothstep(-0.10, 0.20, cosSun) * (1.0 - smoothstep(0.18, 0.40, cosSun));
    color += vec3(0.18, 0.08, 0.0) * twi;

    FragColor = vec4(color, 1.0);
}
