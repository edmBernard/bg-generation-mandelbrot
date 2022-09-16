/*
Compute shader hello world.

Does a simple computation, and writes it directly to the
texture seen by the frament shader.

This could be done easily on a fragment shader,
so this is is just an useless sanity check example.

The main advantage of compute shaders (which we are not doing here),
is that they can keep state data on the GPU between draw calls.

This is basically the upper limit speed of compute to texture operations,
since we are only doing a very simple operaiton on the shader.

TODO understand:

GL_MAX_COMPUTE_WORK_GROUP_COUNT
GL_MAX_COMPUTE_WORK_GROUP_SIZE
GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
glDispatchCompute
glMemoryBarrier
local_size_x
binding
*/


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

    /* Shader. */
    shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
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

static const GLuint WIDTH = 400;
static const GLuint HEIGHT = 400;
static const GLfloat vertices_xy_uv[] = {
    -1.0,  1.0, 0.0, 1.0,
     1.0,  1.0, 0.0, 0.0,
     1.0, -1.0, 1.0, 0.0,
    -1.0, -1.0, 1.0, 1.0,
};
static const std::vector<GLfloat> particles_xy_vxy = {
     1.0,  0.5, 0.1, 0.1,
     0.7,  0.5, 0.1, 0.1,
     0.5, -0.5, 0.1, 0.1,
     0.2, -0.5, 0.1, 0.1,
};
static const GLuint indices[] = {
    0, 1, 2,
    0, 2, 3,
};

static const GLchar *vertex_shader_source =
    "#version 330 core\n"
    "in vec2 coord2d;\n"
    "in vec2 vertexUv;\n"
    "out vec2 fragmentUv;\n"
    "void main() {\n"
    "    gl_Position = vec4(coord2d, 0, 1);\n"
    "    fragmentUv = vertexUv;\n"
    "}\n";
static const GLchar *fragment_shader_source =
    "#version 330 core\n"
    "in vec2 fragmentUv;\n"
    "out vec3 color;\n"
    "uniform sampler2D textureSampler;\n"
    "void main() {\n"
    "    color = texture(textureSampler, fragmentUv.yx).rgb;\n"
    "}\n";
static const char *compute_shader_source =
    "#version 430\n"
    // "in vec2 p_coord2d;\n"
    // "in vec2 p_velocity;\n"
    "uniform vec4 particule_buffer;\n"
    "layout (local_size_x = 1, local_size_y = 1) in;\n"
    "layout (rgba32f, binding = 0) uniform image2D img_output;\n"
    // "layout (rgba32f, binding = 3) uniform image2D particles;\n"
    "struct data\n"
    "{\n"
    "  float x;\n"
    "  float y;\n"
    "  float vx;\n"
    "  float vy;\n"
    "};\n"
    "layout(std430, binding = 3) buffer layoutName\n"
    "{\n"
    "  data data_SSBO[];\n"
    "};\n"
    "void main () {\n"
    "    float value = particule_buffer[gl_GlobalInvocationID.x];\n"
    "        data_SSBO[gl_GlobalInvocationID.x].vx += 0.1;\n"
    "        data_SSBO[gl_GlobalInvocationID.x].vy += 0.1;\n"
    "    for (int i = 0; i < 100; ++i) {\n"
    "      for (int j = 0; j < 100; ++j) {\n"
    // "        ivec2 gid = ivec2(gl_GlobalInvocationID.xy);\n" //  particles[gl_GlobalInvocationID.xy].xy;\n"
    "        ivec2 gid = ivec2(i +  100 * (gl_GlobalInvocationID.x+1) + 100 * data_SSBO[gl_GlobalInvocationID.x].vx, j * (gl_GlobalInvocationID.y+1) + 100 * data_SSBO[gl_GlobalInvocationID.x].vy);\n" //  particles[gl_GlobalInvocationID.xy].xy;\n"
    "        ivec2 dims = imageSize(img_output);\n"
    "        vec4 pixel = vec4(data_SSBO[gl_GlobalInvocationID.x].x, 0.0, 1.0, 1.0);\n"
    "        imageStore(img_output, gid, pixel);\n"
    "      }\n"
    "    }\n"
    "}\n";

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
    program = common_get_shader_program(vertex_shader_source, fragment_shader_source);
    coord2d_location = glGetAttribLocation(program, "coord2d");
    vertexUv_location = glGetAttribLocation(program, "vertexUv");
    textureSampler_location = glGetUniformLocation(program, "textureSampler");

    /* Compute shader. */
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

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(particles_xy_vxy), particles_xy_vxy.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

    GLuint particule_buffer_location;
    particule_buffer_location = glGetAttribLocation(program, "particule_buffer");
    glUniform1fv(particule_buffer_location, sizeof(particles_xy_vxy[0]), particles_xy_vxy.data());

    /* Main loop. */
    while (!glfwWindowShouldClose(window)) {
        /* Compute. */
        glUseProgram(compute_program);
        /* Dimensions given here appear in gl_GlobalInvocationID.xy in the shader. */
        // glDispatchCompute((GLuint)width, (GLuint)height, 1);
        glDispatchCompute((GLuint)5, (GLuint)1, 1);
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
