#version 330 core
in vec3 vLocal;
in vec3 vWorld;
out vec4 FragColor;
uniform vec3 uSunDir;
uniform vec3 uViewPos;
void main() {
    // World-space normal (sphere centered at model origin): normalize the
    // world position so both n and viewDir are in the same (world) frame.
    vec3 n = normalize(vWorld);
    vec3 viewDir = normalize(uViewPos - vWorld);
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0);
    float sun = smoothstep(-0.3, 0.3, dot(n, normalize(uSunDir)));
    vec3 dayGlow   = vec3(0.38, 0.62, 1.0);
    vec3 nightGlow = vec3(0.08, 0.12, 0.24);
    vec3 col = mix(nightGlow, dayGlow, sun) * rim * 1.5;
    FragColor = vec4(col, clamp(rim * (0.45 + 0.7 * sun) * 1.5, 0.0, 1.0));
}
