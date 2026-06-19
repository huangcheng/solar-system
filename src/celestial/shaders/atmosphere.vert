#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 vLocal;
out vec3 vWorld;
uniform mat4 uMVP;
uniform mat4 uModel;
void main() {
    vLocal = aPos;
    vWorld = vec3(uModel * vec4(aPos, 1.0));
    gl_Position = uMVP * vec4(aPos, 1.0);
}
