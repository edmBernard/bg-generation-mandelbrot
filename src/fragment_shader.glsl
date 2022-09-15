#version 460

uniform vec2 resolution;

out vec4 fragColor;

void main()
{
  vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
  vec3 col = vec3(0.0);

  col += length(uv);
  fragColor = vec4(col, 1.0);
}