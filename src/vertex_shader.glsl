#version 460

attribute vec3 vCol;
attribute vec2 vPos;

in vec3 in_position;
out vec3 color;

void main() {
    gl_Position = vec4(vPos, 0.0, 1.0);
    color = vCol;
}