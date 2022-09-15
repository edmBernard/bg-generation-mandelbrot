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

static const struct { float x, y; } vertices[4] = {
  {-1.f, -1.f},
  {-1.f,  1.f},
  { 1.f,  1.f},
  { 1.f, -1.f}};

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

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  GLFWwindow *window = glfwCreateWindow(3840, 2400, "MandelBrot Set", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwSetKeyCallback(window, key_callback);

  glfwMakeContextCurrent(window);
  gladLoadGL();
  glfwSwapInterval(1);

  // TODO: OpenGL error checks

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  GLuint vertex_shader;
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  auto vertex_shader_text = readFile("shaders/vertex_shader.glsl");
  const char* vertex_shader_data = vertex_shader_text.c_str();
  glShaderSource(vertex_shader, 1, &vertex_shader_data, NULL);
  glCompileShader(vertex_shader);

  GLuint fragment_shader;
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  auto fragment_shader_text = readFile("shaders/fragment_shader.glsl");
  const char* fragment_shader_data = fragment_shader_text.c_str();
  glShaderSource(fragment_shader, 1, &fragment_shader_data, NULL);
  glCompileShader(fragment_shader);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  GLint vpos_location = glGetAttribLocation(program, "vPos");
  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void *)0);

  GLint resolution_location = glGetUniformLocation(program, "resolution");
  GLint cursor_location = glGetUniformLocation(program, "cursor");
  GLint time_location = glGetUniformLocation(program, "time");

  while (!glfwWindowShouldClose(window)) {

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    const vec2 cursorPosition{static_cast<float>(xpos), static_cast<float>(ypos)};
    glUniform2fv(cursor_location, 1, static_cast<const GLfloat *>(cursorPosition));

    const vec2 windowsSize{static_cast<float>(width), static_cast<float>(height)};
    glUniform2fv(resolution_location, 1, static_cast<const GLfloat *>(windowsSize));

    glUniform1f(time_location, static_cast<float>(glfwGetTime()));

    glUseProgram(program);

    glDrawArrays(GL_QUADS, 0, 4);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);

  glfwTerminate();
  return EXIT_SUCCESS;

} catch (const std::exception &e) {
  spdlog::error("{}", e.what());
  return EXIT_FAILURE;
}