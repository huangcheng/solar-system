#version 330 core
in vec3 vLocal;   // un-spun object position
in vec3 vWorld;   // spun world position
out vec4 FragColor;
uniform sampler2D uClouds;
uniform vec3 uSunWorld;
uniform float uHasClouds;
void main() {
    // UV from the un-spun object position: the texture rides the slowly spinning
    // cloud sphere so the clouds drift over the surface.
    vec3 n = normalize(vLocal);
    float lon = atan(n.y, n.x);
    float lat = asin(clamp(n.z, -1.0, 1.0));
    vec2 uv = vec2((lon / 3.14159265 + 1.0) * 0.5, (lat / 1.5707963 + 1.0) * 0.5);

    float raw = (uHasClouds > 0.5) ? texture(uClouds, uv).r : 0.0;
    float a = clamp(raw * 1.3, 0.0, 1.0);

    // Lighting from the SPUN world position vs the real world sun, so a cloud
    // that drifts onto the night side actually goes dark. Near-black floor on the
    // night side (just a touch of moonlight) so night clouds don't grey-haze the
    // dark hemisphere over the city lights.
    float light = smoothstep(-0.15, 0.25, dot(normalize(vWorld), normalize(uSunWorld)));
    FragColor = vec4(vec3(0.04 + 0.96 * light), a);
}
