#include <fmt/core.h>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "linmath.h/linmath.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::string readFile(const char *filePath) {
  std::string content;
  std::ifstream fileStream(filePath, std::ios::in);

  if (!fileStream.is_open()) {
    spdlog::error("Could not read file {}. File does not exist.", filePath);
    throw std::runtime_error("File not found");
  }

  std::string line = "";
  while (!fileStream.eof()) {
    std::getline(fileStream, line);
    content.append(line + "\n");
  }

  fileStream.close();
  return content;
}

static const struct { float x, y; } vertices[4] = {
    {-1.f, -1.f},
    {-1.f, 1.f},
    {1.f, -1.f},
    {1.f, 1.f}};

static void error_callback(int error, const char *description) {
  spdlog::error("Error in glfw {}. File does not exist.", description);
  throw std::runtime_error("Error in glfw");
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);

  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    std::vector<float> buffer(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    std::string filename = fmt::format("mandelbrot_{}.png", std::chrono::system_clock::now().time_since_epoch().count());
    stbi_write_png(filename.c_str(), width, height, 3, buffer.data(), width * 3);
  }
}

int main(int argc, char *argv[]) try {

  spdlog::cfg::load_env_levels();

  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    spdlog::error("Could not start GLFW3");
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *window = glfwCreateWindow(3840, 2400, "MandelBrot Set", NULL, NULL);
  if (!window) {
    spdlog::error("Fail to create windows");
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
    spdlog::error("Failed to initialize OpenGL context");
    return EXIT_FAILURE;
  }

  spdlog::info("Render: {}", glGetString(GL_RENDERER));
  spdlog::info("OpenGL version: {}", glGetString(GL_VERSION));
  glfwSwapInterval(1);

  GLint log_length, success;
  std::string log;

  // Build Vertex Shader
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  auto vertex_shader_text = readFile("shaders/vertex_shader.vert");
  const char *vertex_shader_data = vertex_shader_text.c_str();
  glShaderSource(vertex_shader, 1, &vertex_shader_data, NULL);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
  if (log_length > 0) {
    log.resize(log_length);
    glGetShaderInfoLog(vertex_shader, log_length, NULL, log.data());
    spdlog::error("Vertex shader log : \n{}\n", log);
  }
  if (!success) {
    spdlog::error("Vertex shader compile failed\n");
    return EXIT_FAILURE;
  }

  // Build Fragment Shader
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  auto fragment_shader_text = readFile("shaders/fragment_shader.frag");
  const char *fragment_shader_data = fragment_shader_text.c_str();
  glShaderSource(fragment_shader, 1, &fragment_shader_data, NULL);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
  if (log_length > 0) {
    log.resize(log_length);
    glGetShaderInfoLog(fragment_shader, log_length, NULL, log.data());
    spdlog::error("Fragment shader log : \n{}\n", log);
  }
  if (!success) {
    spdlog::error("Fragment shader compile failed\n");
    return EXIT_FAILURE;
  }
  // Build Program
  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, fragment_shader);
  glAttachShader(shader_program, vertex_shader);
  glLinkProgram(shader_program);
  glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &log_length);
  if (log_length > 0) {
    log.resize(log_length);
    glGetProgramInfoLog(shader_program, log_length, NULL, log.data());
    spdlog::error("Program link log : \n{}\n", log);
  }
  if (!success) {
    spdlog::error("Program link failed\n");
    return EXIT_FAILURE;
  }

  glEnable(GL_DEPTH_TEST); // enable depth-testing
  glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  GLuint vpos_location = glGetAttribLocation(shader_program, "vPos");
  glGenVertexArrays(1, &vpos_location);

  // Try to correclty bind buffer and array like in: https://github.com/g-truc/ogl-samples/blob/master/samples/gl-410-primitive-instanced.cpp
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(vpos_location);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
      glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  GLint resolution_location = glGetUniformLocation(shader_program, "resolution");
  GLint cursor_location = glGetUniformLocation(shader_program, "cursor");
  GLint time_location = glGetUniformLocation(shader_program, "time");

  while(!glfwWindowShouldClose(window)) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    // wipe the drawing surface clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    const vec2 cursorPosition{static_cast<float>(xpos), static_cast<float>(ypos)};
    glUniform2fv(cursor_location, 1, static_cast<const GLfloat *>(cursorPosition));

    const vec2 windowsSize{static_cast<float>(width), static_cast<float>(height)};
    glUniform2fv(resolution_location, 1, static_cast<const GLfloat *>(windowsSize));

    glUniform1f(time_location, static_cast<float>(glfwGetTime()));

    glUseProgram(shader_program);

    // Bind vertex array & draw
		glBindVertexArray(vpos_location);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // update other events like input handling
    glfwPollEvents();
    // put the stuff we've been drawing onto the display
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return EXIT_SUCCESS;

} catch (const std::exception &e) {
  spdlog::error("{}", e.what());
  return EXIT_FAILURE;
}