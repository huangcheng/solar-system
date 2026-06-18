#version 330 core
in vec3 vLocal;
out vec4 FragColor;
uniform sampler2D uClouds;
uniform vec3 uSunDir;
uniform float uHasClouds;
void main() {
    vec3 n = normalize(vLocal);
    float lon = atan(n.y, n.x);
    float lat = asin(clamp(n.z, -1.0, 1.0));
    vec2 uv = vec2((lon/3.14159265 + 1.0)*0.5, (lat/1.5707963 + 1.0)*0.5);
    float a = (uHasClouds > 0.5) ? texture(uClouds, uv).r : 0.0;
    float light = smoothstep(-0.15, 0.25, dot(n, normalize(uSunDir)));
    FragColor = vec4(vec3(1.0) * (0.25 + 0.75*light), a * 0.85);
}
