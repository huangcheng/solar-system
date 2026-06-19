#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 vLocal;   // un-spun object position -> cloud texture lookup (lets clouds drift)
out vec3 vWorld;   // spun world position    -> correct day/night lighting
uniform mat4 uMVP;
uniform mat4 uModel;
void main() {
    vLocal = aPos;
    vWorld = vec3(uModel * vec4(aPos, 1.0));
    gl_Position = uMVP * vec4(aPos, 1.0);
}
