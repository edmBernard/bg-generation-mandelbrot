#version 430 core

out vec3 color;

in vec2 fragmentUv;

uniform sampler2D textureSampler;

void main() {
    color = texture(textureSampler, fragmentUv.yx).rgb;
};