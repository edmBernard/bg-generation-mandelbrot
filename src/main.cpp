
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

/* Build and compile shader program, return its ID. */
GLuint common_get_shader_program(
    const char *vertex_shader_source,
    const char *fragment_shader_source
) {
    GLint log_length, success;
    GLuint fragment_shader, program, vertex_shader;

    /* Vertex shader */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
    if (!success) {
        printf("vertex shader compile error\n");
        exit(EXIT_FAILURE);
    }

    /* Fragment shader */
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
    if (!success) {
        printf("fragment shader compile error\n");
        exit(EXIT_FAILURE);
    }

    /* Link shaders */
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

    /* Cleanup. */
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

GLuint common_get_compute_program(const char *source) {

    GLint log_length, success;
    GLuint program, shader;
    std::string log;

    /* Shader. */
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

    /* Program. */
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

static const GLuint WIDTH = 1000;
static const GLuint HEIGHT = 1000;
static const GLfloat vertices_xy_uv[] = {
    -1.0,  1.0, 0.0, 1.0,
     1.0,  1.0, 0.0, 0.0,
     1.0, -1.0, 1.0, 0.0,
    -1.0, -1.0, 1.0, 1.0,
};
// static const std::vector<GLfloat> particles_xy_vxy = {
//      1.0,  0.5, 0.1, 0.1,
//      0.7,  0.5, 0.1, -0.1,
//      0.5, -0.5, 0.1, 0.1,
//      0.2, -0.5, 0.1, -0.1,
// };
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
  std::random_device rd;  // Will be used to obtain a seed for the random number engine
  std::mt19937 gen; // Standard mersenne_twister_engine seeded with rd()

  std::vector<Particle> particles_xy_vxy;
};

int main(void) {
    GLFWwindow *window;
    GLint
        coord2d_location,
        textureSampler_location,
        vertexUv_location,
        p_coord2d_location,
        p_velocity_location
    ;
    GLuint
        ebo,
        program,
        compute_program,
        texture,
        vbo,
        vao
    ;
    unsigned int
        width = WIDTH,
        height = HEIGHT
    ;

    /* Window. */
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(width, height, __FILE__, NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL();

    /* Shader. */
    auto vertex_shader_text = readFile("shaders/slime.vert");
    const char* vertex_shader_source = vertex_shader_text.c_str();
    auto fragment_shader_text = readFile("shaders/slime.frag");
    const char* fragment_shader_source = fragment_shader_text.c_str();
    program = common_get_shader_program(vertex_shader_source, fragment_shader_source);
    coord2d_location = glGetAttribLocation(program, "coord2d");
    vertexUv_location = glGetAttribLocation(program, "vertexUv");
    textureSampler_location = glGetUniformLocation(program, "textureSampler");

    /* Compute shader. */
    auto compute_shader_text = readFile("shaders/slime.compute");
    const char* compute_shader_source = compute_shader_text.c_str();
    compute_program = common_get_compute_program(compute_shader_source);
    p_velocity_location = glGetAttribLocation(program, "p_velocity");

    /* vbo */
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_xy_uv), vertices_xy_uv, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // /* pab */
    // glGenBuffers(1, &pab);
    // glBindBuffer(GL_ARRAY_BUFFER, pab);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(particles_xy_vxy), particles_xy_vxy, GL_STATIC_DRAW);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* ebo */
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /* vao */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(coord2d_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(vertices_xy_uv[0]), (GLvoid*)0);
    glEnableVertexAttribArray(coord2d_location);
    glVertexAttribPointer(vertexUv_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(vertices_xy_uv[0]), (GLvoid*)(2 * sizeof(vertices_xy_uv[0])));
    glEnableVertexAttribArray(vertexUv_location);
    glBindVertexArray(0);

    /* Texture. */
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // glBindBuffer(GL_ARRAY_BUFFER, pab);
    // glVertexAttribPointer(p_coord2d_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(particles_xy_vxy[0]), (GLvoid*)0);
    // glEnableVertexAttribArray(p_coord2d_location);
    // glVertexAttribPointer(p_velocity_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(particles_xy_vxy[0]), (GLvoid*)(2 * sizeof(particles_xy_vxy[0])));
    // glEnableVertexAttribArray(p_velocity_location);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    /* Same internal format as compute shader input.
     * data=NULL to just allocate the memory but not set it to anything. */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    /* Bind to image unit, to allow writting to it from the compute shader. */
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    constexpr int numParticles = 1000;
    ParticlesEngine particles(numParticles, 100, vec2{500, 500});
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    // glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat) * particles.particles_xy_vxy.size(), particles.particles_xy_vxy.data(), GL_STATIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticlesEngine::Particle) * particles.particles_xy_vxy.size(), particles.particles_xy_vxy.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

    // GLuint particule_buffer_location;
    // particule_buffer_location = glGetAttribLocation(program, "particule_buffer");
    // glUniform1fv(particule_buffer_location, sizeof(particles.particles_xy_vxy[0]), particles.particles_xy_vxy.data());

    /* Main loop. */
    while (!glfwWindowShouldClose(window)) {
        /* Compute. */
        glUseProgram(compute_program);
        /* Dimensions given here appear in gl_GlobalInvocationID.xy in the shader. */
        // glDispatchCompute((GLuint)width, (GLuint)height, 1);
        glDispatchCompute(static_cast<GLuint>(numParticles), 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        /* Global state. */
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

    /* Cleanup. */
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glDeleteProgram(compute_program);
    glfwTerminate();
    return EXIT_SUCCESS;
}
