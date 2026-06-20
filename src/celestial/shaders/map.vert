#version 330 core
// Flat equirectangular map: a fullscreen quad. UV maps directly to lon/lat
// because the day/night textures ARE equirectangular 2:1 maps.
layout(location = 0) in vec2 aPos;   // clip-space [-1,1]
layout(location = 1) in vec2 aUv;    // [0,1]
out vec2 vUv;
void main() {
    vUv = aUv;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
