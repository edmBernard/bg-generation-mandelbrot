#include <vector>

namespace eb {

constexpr float MAX_ITER = 128.0f;

struct vec2 {
  float x;
  float y;
};

vec2 operator*(float value, vec2 vec) {
  return {vec.x * value, vec.y * value};
}
vec2 operator/(vec2 vec, float value) {
  return {vec.x / value, vec.y / value};
}
vec2 operator-(vec2 v1, vec2 v2) {
  return {v1.x - v2.x, v1.y - v2.y};
}
vec2 operator+(vec2 v1, vec2 v2) {
  return {v1.x + v2.x, v1.y + v2.y};
}

struct vec3 {
  float x;
  float y;
  float z;
};

float hue2rgb(float p, float q, float t) {
  if (t < 0.)
    t += 1.;
  if (t > 1.)
    t -= 1.;
  if (t < 1. / 6.0)
    return p + (q - p) * 6. * t;
  if (t < 1. / 2.0)
    return q;
  if (t < 2. / 3.0)
    return p + (q - p) * (2. / 3.0 - t) * 6.;
  return p;
}

// converter taken from https://github.com/ratkins/RGBConverter/blob/master/RGBConverter.cpp
vec3 hslToRgb(float h, float s, float l) {
  float r, g, b;

  if (s == 0.) {
    r = g = b = l; // achromatic
  } else {
    float q = l < 0.5 ? l * (1. + s) : l + s - l * s;
    float p = 2. * l - q;
    r = hue2rgb(p, q, h + 1. / 3.0);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1. / 3.0);
  }

  return vec3{r, g, b};
}

// c normalized and centered
std::vector<uint8_t> mandelbrot(const vec2 &c, const int widthExport, const int heightExport) {

  std::vector<uint8_t> cpu_buffer(widthExport * heightExport * 3, 0.f);
  for (int y = 0; y < heightExport; ++y) {

    for (int x = 0; x < widthExport; ++x) {

      // Julia on cursor position
      const eb::vec2 current{static_cast<float>(x), static_cast<float>(y)};
      eb::vec2 z = (current - 0.5f * eb::vec2{static_cast<float>(widthExport), static_cast<float>(heightExport)}) / static_cast<float>(heightExport);

      for (float i = 0.0; i < MAX_ITER; ++i) {
        z = eb::vec2{z.x * z.x - z.y * z.y, 2.0f * z.x * z.y} + c;
        if ((z.x * z.x + z.y * z.y) > 4.0) {
          // algorithm to smooth color and avoid color quantification
          // explain in https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set#Continuous_(smooth)_coloring
          float log_zn = log(z.x * z.x + z.y * z.y) / 2.;
          float nu = log(log_zn / log(2.)) / log(2.);
          i = i + 1. - nu;
          eb::vec3 col = eb::hslToRgb(0.6, 0.8, i / MAX_ITER);
          cpu_buffer[x * 3 + y * widthExport * 3 + 0] = std::clamp(col.x, 0.f, 1.f) * 255;
          cpu_buffer[x * 3 + y * widthExport * 3 + 1] = std::clamp(col.y, 0.f, 1.f) * 255;
          cpu_buffer[x * 3 + y * widthExport * 3 + 2] = std::clamp(col.z, 0.f, 1.f) * 255;
          break;
        }
      }
    }
  }
  return cpu_buffer;
}

} // namespace eb