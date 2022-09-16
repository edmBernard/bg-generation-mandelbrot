
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>
#include <fmt/core.h>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "linmath.h/linmath.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

std::string readFile(const char *filePath) {
  std::string content;
  std::ifstream fileStream(filePath, std::ios::in);

  if(!fileStream.is_open()) {
    spdlog::error("Could not read file {}. File does not exist.", filePath);
    throw std::runtime_error("File not found");
  }

  std::string line = "";
  while(!fileStream.eof()) {
      std::getline(fileStream, line);
      content.append(line + "\n");
  }

  fileStream.close();
  return content;
}

// Build and compile shader program, return its ID.
GLuint common_get_shader_program(
    const char *vertex_shader_source,
    const char *fragment_shader_source
) {
    GLint log_length, success;
    GLuint fragment_shader, program, vertex_shader;

    // Vertex shader
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
    if (!success) {
        printf("vertex shader compile error\n");
        exit(EXIT_FAILURE);
    }

    // Fragment shader
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
    if (!success) {
        printf("fragment shader compile error\n");
        exit(EXIT_FAILURE);
    }

    // Link shaders
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    if (!success) {
        printf("shader link error");
        exit(EXIT_FAILURE);
    }

    // Cleanup.
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

GLuint common_get_compute_program(const char *source) {

    GLint log_length, success;
    GLuint program, shader;
    std::string log;

    // Shader.
    shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 0) {
      log.resize(log_length);
      glGetShaderInfoLog(shader, log_length, NULL, log.data());
      // printf("compute shader:\n\n%s\n", log);
      spdlog::error("compute shader log : \n{}", log);
    }
    if (!success) {
        printf("error: compute shader compile\n");
        exit(EXIT_FAILURE);
    }

    // Program.
    program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

    if (!success) {
        printf("shader link error");
        exit(EXIT_FAILURE);
    }

    glDeleteShader(shader);
    return program;
}

static const GLfloat vertices_xy_uv[] = {
    -1.0,  1.0, 0.0, 1.0,
     1.0,  1.0, 0.0, 0.0,
     1.0, -1.0, 1.0, 0.0,
    -1.0, -1.0, 1.0, 1.0,
};
static const GLuint indices[] = {
    0, 1, 2,
    0, 2, 3,
};

struct ParticlesEngine {

  struct Particle
  {
    float x;
    float y;
    float vx;
    float vy;
  };

  ParticlesEngine(size_t num, float radius, vec2 center) : gen(rd()) {

    std::uniform_real_distribution<> dist(-radius, radius);
    while (particles_xy_vxy.size() < num) {
      const float x = dist(gen);
      const float y = dist(gen);
      if (x * x + y * y < radius * radius) {
        particles_xy_vxy.push_back(Particle{x + center[0], y + center[1], y, -x});
      }
    }
    spdlog::info("number of particules generated : {}", particles_xy_vxy.size());
  }
  std::random_device rd;
  std::mt19937 gen;

  std::vector<Particle> particles_xy_vxy;
};

int main(void) {
    const uint32_t width = 2000;
    const uint32_t height = 1000;

    // Window.
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow* window = glfwCreateWindow(width, height, __FILE__, NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL();

    // Display Shader
    auto vertex_shader_text = readFile("shaders/slime.vert");
    const char* vertex_shader_source = vertex_shader_text.c_str();
    auto fragment_shader_text = readFile("shaders/slime.frag");
    const char* fragment_shader_source = fragment_shader_text.c_str();
    GLuint program = common_get_shader_program(vertex_shader_source, fragment_shader_source);
    GLuint coord2d_location = glGetAttribLocation(program, "coord2d");
    GLuint vertexUv_location = glGetAttribLocation(program, "vertexUv");
    GLuint textureSampler_location = glGetUniformLocation(program, "textureSampler");

    // vbo
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_xy_uv), vertices_xy_uv, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ebo
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // vao
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(coord2d_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(vertices_xy_uv[0]), (GLvoid*)0);
    glEnableVertexAttribArray(coord2d_location);
    glVertexAttribPointer(vertexUv_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(vertices_xy_uv[0]), (GLvoid*)(2 * sizeof(vertices_xy_uv[0])));
    glEnableVertexAttribArray(vertexUv_location);
    glBindVertexArray(0);

    // Particule to trailmap shader
    auto compute_shader_text = readFile("shaders/slime.compute");
    const char* compute_shader_source = compute_shader_text.c_str();
    GLuint compute_program = common_get_compute_program(compute_shader_source);

    // Blur shader
    auto blur_compute_shader_text = readFile("shaders/blur.compute");
    const char* blur_compute_shader_source = blur_compute_shader_text.c_str();
    GLuint blur_compute_program = common_get_compute_program(blur_compute_shader_source);


    // Trailmap
    GLuint trailmap;
    glGenTextures(1, &trailmap);
    glBindTexture(GL_TEXTURE_2D, trailmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, trailmap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);


    // TrailMap Blurred
    GLuint trailmapBlurred;
    glGenTextures(1, &trailmapBlurred);
    glBindTexture(GL_TEXTURE_2D, trailmapBlurred);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, trailmapBlurred, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // Particles Buffer
    constexpr int numParticles = 1000;
    ParticlesEngine particles(numParticles, 100, vec2{width/2.f, height/2.f});
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticlesEngine::Particle) * particles.particles_xy_vxy.size(), particles.particles_xy_vxy.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind


    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Compute
        glUseProgram(compute_program);
        glDispatchCompute(static_cast<GLuint>(numParticles), 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glUseProgram(blur_compute_program);
        glDispatchCompute(static_cast<GLuint>(width), static_cast<GLuint>(height), 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // the display show trailmap and trailmapBlurrer blended but I only want trailmapBlurred

        // Global state
        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glUniform1i(textureSampler_location, 0);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &trailmap);
    glDeleteTextures(1, &trailmapBlurred);
    glDeleteBuffers(1, &ssbo);

    glDeleteProgram(program);
    glDeleteProgram(compute_program);
    glDeleteProgram(blur_compute_program);
    glfwTerminate();
    return EXIT_SUCCESS;
}
