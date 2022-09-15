#version 460

out vec4 fragColor;

uniform vec2 resolution;
uniform float time;

const float MAX_ITER = 128.0;

float hue2rgb(float p, float q, float t) {
    if(t < 0) t += 1;
    if(t > 1) t -= 1;
    if(t < 1/6.0) return p + (q - p) * 6 * t;
    if(t < 1/2.0) return q;
    if(t < 2/3.0) return p + (q - p) * (2/3.0 - t) * 6;
    return p;
}

// converter taken from https://github.com/ratkins/RGBConverter/blob/master/RGBConverter.cpp
vec3 hslToRgb(float h, float s, float l) {
    float r, g, b;

    if (s == 0) {
        r = g = b = l; // achromatic
    } else {
        float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        r = hue2rgb(p, q, h + 1/3.0);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1/3.0);
    }

    return vec3(r, g, b);
}

float mandelbrot(vec2 uv) {
  vec2 c = 4.0 * uv - vec2(0.7, 0.0);
  c = c / pow(time, 4.0) - vec2(0.65, 0.45);
  vec2 z = vec2(0.0);
  for (float i = 0.0; i < MAX_ITER; ++i) {
    z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
    if (dot(z, z) > 4.0) {
      // algorithm to smooth color and avoid color quantification
      // explain in https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set#Continuous_(smooth)_coloring
      float log_zn = log(z.x * z.x + z.y * z.y) / 2;
      float nu = log(log_zn / log(2)) / log(2);
      i = i + 1 - nu;
      return i / MAX_ITER;
    }
  }
  return 0.0;
}

void main()
{
  vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
  vec3 col = vec3(0.0);

  float m = mandelbrot(uv);

  if (m != 0.0) {
    col = hslToRgb(m + 0.6, 0.5, 0.5);
  }
  fragColor = vec4(col, 1.0);
}