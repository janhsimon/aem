#include <glad/gl.h>
#include <glfw/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <vector>

namespace
{

// Header definition
struct Header
{
  uint32_t numVertices, numTriangles, numMeshes, numBones;
};

// Vertex definitions
struct MeshVertex
{
  glm::vec3 position, normal, tangent;
  glm::vec2 uv;
  glm::vec<4, uint32_t> boneIds;
  glm::vec4 boneWeights;
};

struct DebugVertex
{
  glm::vec3 position;
};

// Bone definition
struct Bone
{
  glm::mat4 inverseBindPoseMatrix;
  int parentIndex;
};

// Debug constants
constexpr bool overlayBones = true;

// Window constants
constexpr char windowTitle[] = "AEM Viewer";
constexpr int windowWidth = 640;
constexpr int windowHeight = 400;
constexpr float windowAspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

// OpenGL constants
constexpr GLsizei shaderInfoLogLength = 512;

// Color constants
constexpr glm::vec4 clearColor = { 0.1f, 0.4f, 0.9f, 1.0f };
constexpr glm::vec4 geometryColor = { 0.9f, 0.4f, 0.1f, 1.0f };
constexpr glm::vec4 boneOverlayColor = { 0.9f, 0.95f, 0.99f, 1.0f };

// Camera constants
constexpr float cameraHeight = 0.4f;
constexpr float cameraMinDistance = 0.5f;
constexpr float cameraNear = 0.1f;
constexpr float cameraFar = 100.0f;
constexpr float cameraFov = 45.0f; // Vertical field of view in degrees

// Camera variables
bool mouseDown = false;
float cameraAngle = glm::radians(25.0f);
float cameraDistance = 5.0f;
double lastMouseX;

// Geometry variables
std::vector<MeshVertex> meshVertices;
std::vector<DebugVertex> debugVertices;
std::vector<uint32_t> indices;
std::vector<uint32_t> meshLengths;
std::vector<Bone> bones;

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

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Missing command line argument: Model filename";
    return EXIT_FAILURE;
  }

  // Parse model
  {
    std::ifstream file(argv[1], std::ios::in | std::ios::binary);
    if (file.fail())
    {
      std::cerr << "Failed to load model file: File not found";
      return EXIT_FAILURE;
    }

    // Read and verify magic number
    {
      int data = 0;
      file.read(reinterpret_cast<char*>(&data), 3);
      if (data != 0x4D4541)
      {
        std::cerr << "Failed to load model file: Wrong file type";
        return EXIT_FAILURE;
      }
    }

    // Read and verify version number
    if (file.get() != 1)
    {
      std::cerr << "Failed to load model file: Wrong file version";
      return EXIT_FAILURE;
    }

    // Read header
    {
      Header header;
      file.read(reinterpret_cast<char*>(&header), sizeof(header));

      std::cout << "Number of vertices: " << header.numVertices << "\n";
      std::cout << "Number of triangles: " << header.numTriangles << "\n";
      std::cout << "Number of meshes: " << header.numMeshes << "\n";
      std::cout << "Number of bones: " << header.numBones << "\n";

      meshVertices.resize(header.numVertices);
      indices.resize(static_cast<size_t>(header.numTriangles) * 3u);
      meshLengths.resize(header.numMeshes);
      bones.resize(header.numBones);
    }

    // Read data
    file.read(reinterpret_cast<char*>(meshVertices.data()), sizeof(meshVertices.at(0u)) * meshVertices.size());
    file.read(reinterpret_cast<char*>(indices.data()), sizeof(indices.at(0u)) * indices.size());
    file.read(reinterpret_cast<char*>(meshLengths.data()), sizeof(meshLengths.at(0u)) * meshLengths.size());
    file.read(reinterpret_cast<char*>(bones.data()), sizeof(bones.at(0u)) * bones.size());

    if (overlayBones)
    {
      // Create a single vertex at the model space origin. That vertex will be transformed by the non-inverted bind pose
      // matrix to visualize each bone in bind pose
      debugVertices.resize(1u);
      debugVertices.at(0u).position = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    file.close();
  }

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
    glPointSize(3.0f);
  }

  // Set up geometry
  GLuint meshVertexArray, debugVertexArray;
  {
    // Generate mesh vertex array
    {
      glGenVertexArrays(1, &meshVertexArray);
      glBindVertexArray(meshVertexArray);

      // Generate and fill a vertex buffer
      {
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(meshVertices.at(0u)) * meshVertices.size()),
                     meshVertices.data(), GL_STATIC_DRAW);
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
        constexpr GLsizei vertexSize = sizeof(meshVertices.at(0u));

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize,
                              reinterpret_cast<void*>(offsetof(MeshVertex, position)));

        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize,
                              reinterpret_cast<void*>(offsetof(MeshVertex, normal)));

        // Tangent
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertexSize,
                              reinterpret_cast<void*>(offsetof(MeshVertex, tangent)));

        // UV
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertexSize, reinterpret_cast<void*>(offsetof(MeshVertex, uv)));

        // Bone IDs
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, vertexSize,
                               reinterpret_cast<void*>(offsetof(MeshVertex, boneIds)));

        // Bone weights
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, vertexSize,
                              reinterpret_cast<void*>(offsetof(MeshVertex, boneWeights)));
      }
    }

    // Generate debug vertex array
    {
      glGenVertexArrays(1, &debugVertexArray);
      glBindVertexArray(debugVertexArray);

      // Generate and fill a vertex buffer
      {
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(debugVertices.at(0u)) * debugVertices.size()),
                     debugVertices.data(), GL_STATIC_DRAW);
      }

      // Apply the vertex definition
      {
        constexpr GLsizei vertexSize = sizeof(debugVertices.at(0u));

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize,
                              reinterpret_cast<void*>(offsetof(DebugVertex, position)));
      }
    }
  }

  // Set up a mesh shader program
  GLuint meshShaderProgram;
  GLint meshShaderViewUniformLocation;
  {
    // Compile the vertex shader
    GLuint vertexShader;
    {
      vertexShader = glCreateShader(GL_VERTEX_SHADER);

      const GLchar* source = R"(#version 330 core
                                uniform mat4 view;
                                uniform mat4 projection;
                                layout(location = 0) in vec3 inPosition;
                                layout(location = 1) in vec3 inNormal;
                                out vec3 normal;
                                void main()
                                {
                                  gl_Position = projection * view * vec4(inPosition, 1.0);
                                  normal = inNormal;
                                })";

      glShaderSource(vertexShader, 1, &source, nullptr);
      glCompileShader(vertexShader);

      GLint success;
      glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetShaderInfoLog(vertexShader, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to compile mesh vertex shader:\n" << infoLog;
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
                                in vec3 normal;
                                out vec4 outColor;
                                void main()
                                {
                                  float diffuse = dot(normal, normalize(vec3(0.4, 1.0, 0.85)));
                                  outColor = vec4(color.rgb * diffuse, color.a);
                                })";

      glShaderSource(fragmentShader, 1, &source, nullptr);
      glCompileShader(fragmentShader);

      GLint success;
      glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetShaderInfoLog(fragmentShader, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to compile mesh fragment shader:\n" << infoLog;
        glfwTerminate();
        return EXIT_FAILURE;
      }
    }

    // Link shader program
    {
      meshShaderProgram = glCreateProgram();

      glAttachShader(meshShaderProgram, vertexShader);
      glAttachShader(meshShaderProgram, fragmentShader);

      glLinkProgram(meshShaderProgram);

      GLint success;
      glGetProgramiv(meshShaderProgram, GL_LINK_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetProgramInfoLog(meshShaderProgram, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to link mesh shader program:\n" << infoLog;
        glfwTerminate();
        return EXIT_FAILURE;
      }

      glDeleteShader(vertexShader);
      glDeleteShader(fragmentShader);
    }

    // Use shader program, retrieve uniform locations and set constant uniform values
    {
      glUseProgram(meshShaderProgram);

      // Retrieve view matrix uniform location
      {
        meshShaderViewUniformLocation = glGetUniformLocation(meshShaderProgram, "view");
        if (meshShaderViewUniformLocation < 0)
        {
          std::cerr << "Failed to get view matrix uniform location in mesh shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }
      }

      // Set projection matrix uniform
      {
        const GLint location = glGetUniformLocation(meshShaderProgram, "projection");
        if (location < 0)
        {
          std::cerr << "Failed to get projection matrix uniform location in mesh shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }

        const glm::mat4 projectionMatrix = glm::perspective(cameraFov, windowAspectRatio, cameraNear, cameraFar);
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
      }

      // Set color uniform
      {
        const GLint location = glGetUniformLocation(meshShaderProgram, "color");
        if (location < 0)
        {
          std::cerr << "Failed to get color uniform location in mesh shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }

        const glm::vec4 color = glm::vec4(geometryColor.r, geometryColor.g, geometryColor.b, geometryColor.a);
        glUniform4fv(location, 1, glm::value_ptr(color));
      }
    }
  }

  // Set up a debug shader program
  GLuint debugShaderProgram;
  GLint debugShaderWorldUniformLocation, debugShaderViewUniformLocation;
  {
    // Compile the vertex shader
    GLuint vertexShader;
    {
      vertexShader = glCreateShader(GL_VERTEX_SHADER);

      const GLchar* source = R"(#version 330 core
                                uniform mat4 world;
                                uniform mat4 view;
                                uniform mat4 projection;
                                layout(location = 0) in vec3 inPosition;
                                void main()
                                {
                                  gl_Position = projection * view * world * vec4(inPosition, 1.0);
                                })";

      glShaderSource(vertexShader, 1, &source, nullptr);
      glCompileShader(vertexShader);

      GLint success;
      glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetShaderInfoLog(vertexShader, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to compile debug vertex shader:\n" << infoLog;
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
                                void main()
                                {
                                  outColor = color;
                                })";

      glShaderSource(fragmentShader, 1, &source, nullptr);
      glCompileShader(fragmentShader);

      GLint success;
      glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetShaderInfoLog(fragmentShader, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to compile debug fragment shader:\n" << infoLog;
        glfwTerminate();
        return EXIT_FAILURE;
      }
    }

    // Link shader program
    {
      debugShaderProgram = glCreateProgram();

      glAttachShader(debugShaderProgram, vertexShader);
      glAttachShader(debugShaderProgram, fragmentShader);

      glLinkProgram(debugShaderProgram);

      GLint success;
      glGetProgramiv(debugShaderProgram, GL_LINK_STATUS, &success);
      if (!success)
      {
        GLchar infoLog[shaderInfoLogLength];
        glGetProgramInfoLog(debugShaderProgram, shaderInfoLogLength, nullptr, infoLog);
        std::cerr << "Failed to link debug shader program:\n" << infoLog;
        glfwTerminate();
        return EXIT_FAILURE;
      }

      glDeleteShader(vertexShader);
      glDeleteShader(fragmentShader);
    }

    // Use shader program, retrieve uniform locations and set constant uniform values
    {
      glUseProgram(debugShaderProgram);

      // Retrieve world matrix uniform location
      {
        debugShaderWorldUniformLocation = glGetUniformLocation(debugShaderProgram, "world");
        if (debugShaderWorldUniformLocation < 0)
        {
          std::cerr << "Failed to get world matrix uniform location in debug shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }
      }

      // Retrieve view matrix uniform location
      {
        debugShaderViewUniformLocation = glGetUniformLocation(debugShaderProgram, "view");
        if (debugShaderViewUniformLocation < 0)
        {
          std::cerr << "Failed to get view matrix uniform location in debug shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }
      }

      // Set projection matrix uniform
      {
        const GLint location = glGetUniformLocation(debugShaderProgram, "projection");
        if (location < 0)
        {
          std::cerr << "Failed to get projection matrix uniform location in debug shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }

        const glm::mat4 projectionMatrix = glm::perspective(cameraFov, windowAspectRatio, cameraNear, cameraFar);
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
      }

      // Set color uniform
      {
        const GLint location = glGetUniformLocation(debugShaderProgram, "color");
        if (location < 0)
        {
          std::cerr << "Failed to get color uniform location in debug shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }

        const glm::vec4 color =
          glm::vec4(boneOverlayColor.r, boneOverlayColor.g, boneOverlayColor.b, boneOverlayColor.a);
        glUniform4fv(location, 1, glm::value_ptr(color));
      }
    }
  }

  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    // Render
    {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Render mesh
      {
        glEnable(GL_DEPTH_TEST);

        glBindVertexArray(meshVertexArray);
        glUseProgram(meshShaderProgram);

        // Set view matrix uniform
        {
          const glm::vec3 eye = glm::vec3(glm::sin(cameraAngle), cameraHeight, glm::cos(cameraAngle)) * cameraDistance;
          const glm::mat4 viewMatrix =
            glm::lookAt(eye, glm::vec3(0.0f, cameraHeight, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
          glUniformMatrix4fv(meshShaderViewUniformLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        }

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
      }

      // Render bone overlay
      if (overlayBones)
      {
        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(debugVertexArray);
        glUseProgram(debugShaderProgram);

        // Set view matrix uniform
        {
          const glm::vec3 eye = glm::vec3(glm::sin(cameraAngle), cameraHeight, glm::cos(cameraAngle)) * cameraDistance;
          const glm::mat4 viewMatrix =
            glm::lookAt(eye, glm::vec3(0.0f, cameraHeight, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
          glUniformMatrix4fv(debugShaderViewUniformLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        }

        for (const Bone &bone : bones)
        {
          // Set world matrix uniform
          {
            // Use the non-inverted bone matrix to transform the origin vertex to the bone position in bind pose
            glm::mat4 worldMatrix = glm::inverse(bone.inverseBindPoseMatrix);
            glUniformMatrix4fv(debugShaderWorldUniformLocation, 1, GL_FALSE, glm::value_ptr(worldMatrix));
          }

          glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(debugVertices.size()));
        }
      }

      glfwSwapBuffers(window);
    }

    glfwPollEvents();
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}