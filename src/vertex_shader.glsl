#version 460

attribute vec2 vPos;

in vec3 in_position;

void main() {
  // Don't change vertex position
  gl_Position = vec4(vPos, 0.0, 1.0);
}