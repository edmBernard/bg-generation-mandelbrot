#version 430 core

out vec2 fragmentUv;

in vec2 coord2d;
in vec2 vertexUv;

void main() {
    gl_Position = vec4(coord2d, 0, 1);
    fragmentUv = vertexUv;
};