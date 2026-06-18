#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 vLocal;
uniform mat4 uMVP;
uniform mat4 uModel;
void main() {
    vLocal = aPos;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
