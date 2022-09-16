#version 430 core

out vec3 color;

in vec2 fragmentUv;

layout (binding = 0) uniform sampler2D textureSampler;

void main() {
    color = vec3(texture(textureSampler, fragmentUv.yx).xy, 0.0);
};