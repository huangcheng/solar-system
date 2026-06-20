#version 330 core
// Star (Sun) vertex stage. This is an EXACT mirror of earth.vert: same
// attributes, varyings and uniforms, so the Sun can reuse the very same
// unit-sphere VAO and vertex pipeline as Earth. Only the fragment shader
// differs (star.frag). Do not change the in/out/uniform names without also
// updating earth.vert — they are intentionally identical.
layout(location = 0) in vec3 aPos;   // unit-sphere vertex == geometric normal
out vec3 vLocal;
out vec3 vWorld;
out vec2 vUV;
uniform mat4 uMVP;
uniform mat4 uModel;

vec2 sphereToUV(vec3 n) {
    float lon = atan(n.y, n.x);
    float lat = asin(clamp(n.z, -1.0, 1.0));
    return vec2((lon / 3.14159265 + 1.0) * 0.5, (lat / 1.5707963 + 1.0) * 0.5);
}

void main() {
    vLocal = aPos;
    vWorld = vec3(uModel * vec4(aPos, 1.0));
    vUV = sphereToUV(aPos);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
