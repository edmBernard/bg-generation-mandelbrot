#version 410

in vec2 vPos;

void main() {
  // Don't change vertex position
  gl_Position = vec4(vPos, 0.0, 1.0);
}