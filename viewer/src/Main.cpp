#include <glad/gl.h>
#include <glfw/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <iostream>

namespace
{

// Window constants
constexpr char windowTitle[] = "AEM Viewer";
constexpr int windowWidth = 640;
constexpr int windowHeight = 400;
constexpr float windowAspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

// OpenGL constants
constexpr GLsizei shaderInfoLogLength = 512;

// Geometry constants
constexpr std::array<glm::vec3, 4> vertices = { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f),
                                                glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) };
constexpr std::array<unsigned int, 6> indices = { 0u, 1u, 2u, 1u, 3u, 2u };

// Color constants
constexpr glm::vec4 clearColor = { 0.1f, 0.4f, 0.9f, 1.0f };
constexpr glm::vec4 quadColor = { 0.9f, 0.4f, 0.1f, 1.0f };

// Camera constants
constexpr float cameraMinDistance = 0.5f;
constexpr float cameraNear = 0.1f;
constexpr float cameraFar = 100.0f;
constexpr float cameraFov = 45.0f; // Vertical field of view in degrees

// Camera variables
bool mouseDown = false;
float cameraAngle = glm::radians(45.0f);
float cameraDistance = 5.0f;
double lastMouseX;

void cursorPositionCallback(GLFWwindow* window, double x, double y)
{
  // Tumble the camera
  if (mouseDown)
  {
    const double deltaX = x - lastMouseX;
    cameraAngle -= (static_cast<float>(deltaX * glm::pi<double>()) / windowWidth);
  }
  lastMouseX = x;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  // Store mouse button state
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    mouseDown = (action == GLFW_PRESS);
  }
}

void scrollCallback(GLFWwindow* window, double x, double y)
{
  // Dolly the camera
  cameraDistance -= static_cast<float>(y);
  if (cameraDistance < cameraMinDistance)
  {
    cameraDistance = cameraMinDistance;
  }
}

} // namespace

int main()
{
  // Create window and load OpenGL
  GLFWwindow* window;
  {
    if (!glfwInit())
    {
      std::cerr << "Failed to initialize GLFW";
      return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
    if (!window)
    {
      std::cerr << "Failed to create window";
      glfwTerminate();
      return EXIT_FAILURE;
    }

    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glfwMakeContextCurrent(window);
    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
      std::cerr << "Failed to load OpenGL";
      return EXIT_FAILURE;
    }

    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
  }

  // Set up geometry
  {
    // Generate and bind a vertex array to capture the following vertex and index buffer
    {
      GLuint vertexArray;
      glGenVertexArrays(1, &vertexArray);
      glBindVertexArray(vertexArray);
    }

    // Generate and fill a vertex buffer
    {
      GLuint vertexBuffer;
      glGenBuffers(1, &vertexBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vertices.at(0u)) * vertices.size()), vertices.data(),
                   GL_STATIC_DRAW);
    }

    // Generate and fill an index buffer
    {
      GLuint indexBuffer;
      glGenBuffers(1, &indexBuffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(indices.at(0)) * indices.size()),
                   indices.data(), GL_STATIC_DRAW);
    }

    // Apply the vertex definition
    {
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices.at(0u)), 0);
    }
  }

  // Set up a shader program
  GLint viewUniformLocation;
  {
    // Compile the vertex shader
    GLuint vertexShader;
    {
      vertexShader = glCreateShader(GL_VERTEX_SHADER);

      const GLchar* source = R"(#version 330 core
                                uniform mat4 view;
                                uniform mat4 projection;
                                layout(location = 0) in vec3 inPosition;
                                void main() { gl_Position = projection * view * vec4(inPosition, 1.0); })";

      glShaderSource(vertexShader, 1, &source, nullptr);
      glCompileShader(vertexShader);

      GLint success;
      glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetShaderInfoLog(vertexShader, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to compile vertex shader:\n" << infoLog;
        glfwTerminate();
        return EXIT_FAILURE;
      }
    }

    // Compile the fragment shader
    GLuint fragmentShader;
    {
      fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

      const GLchar* source = R"(#version 330 core
                                uniform vec4 color;
                                out vec4 outColor;
                                void main() { outColor = color; })";

      glShaderSource(fragmentShader, 1, &source, nullptr);
      glCompileShader(fragmentShader);

      GLint success;
      glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetShaderInfoLog(fragmentShader, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to compile fragment shader:\n" << infoLog;
        glfwTerminate();
        return EXIT_FAILURE;
      }
    }

    // Link shader program
    GLuint program;
    {
      program = glCreateProgram();

      glAttachShader(program, vertexShader);
      glAttachShader(program, fragmentShader);

      glLinkProgram(program);

      GLint success;
      glGetProgramiv(program, GL_LINK_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetProgramInfoLog(program, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to link shader program:\n" << infoLog;
        glfwTerminate();
        return EXIT_FAILURE;
      }

      glDeleteShader(vertexShader);
      glDeleteShader(fragmentShader);
    }

    // Use shader program, retrieve uniform locations and set constant uniform values
    {
      glUseProgram(program);

      // Retrieve view matrix uniform location
      {
        viewUniformLocation = glGetUniformLocation(program, "view");
        if (viewUniformLocation < 0)
        {
          std::cerr << "Failed to get view matrix uniform location";
          glfwTerminate();
          return EXIT_FAILURE;
        }
      }

      // Set projection matrix uniform
      {
        const GLint location = glGetUniformLocation(program, "projection");
        if (location < 0)
        {
          std::cerr << "Failed to get projection matrix uniform location";
          glfwTerminate();
          return EXIT_FAILURE;
        }

        const glm::mat4 projectionMatrix = glm::perspective(cameraFov, windowAspectRatio, cameraNear, cameraFar);
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
      }

      // Set color uniform
      {
        const GLint location = glGetUniformLocation(program, "color");
        if (location < 0)
        {
          std::cerr << "Failed to get color uniform location";
          glfwTerminate();
          return EXIT_FAILURE;
        }

        const glm::vec4 color = glm::vec4(quadColor.r, quadColor.g, quadColor.b, quadColor.a);
        glUniform4fv(location, 1, glm::value_ptr(color));
      }
    }
  }

  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    // Set view matrix uniform
    {
      const glm::vec3 eye = glm::vec3(glm::sin(cameraAngle), 0.0f, glm::cos(cameraAngle)) * cameraDistance;
      const glm::mat4 viewMatrix = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    }

    // Render
    {
      glClear(GL_COLOR_BUFFER_BIT);
      glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
      glfwSwapBuffers(window);
    }

    glfwPollEvents();
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}