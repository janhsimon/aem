#include <glad/gl.h>
#include <glfw/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <fstream>
#include <iostream>
#include <vector>

namespace
{

struct Header
{
  uint32_t numVertices, numTriangles, numMeshes, numBones, numKeyframes;
};

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

struct Bone
{
  glm::mat4 inverseBindPoseMatrix;
  int parentIndex;
};

struct Keyframe
{
  float time;
  std::vector<glm::mat4> matrices;
};

struct DecomposedMatrix
{
  glm::vec3 position;
  glm::quat rotation;
  glm::vec3 scale;
};

// Grid overlay constants
constexpr bool overlayGrid = true;                                   // Show grid overlay
constexpr size_t numGridOverlayCells = 10u;                          // The number of cells in a grid quadrant
constexpr size_t numGridOverlayLines = numGridOverlayCells * 2u + 1; // The number of lines in a grid dimension

// Bone overlay constants
constexpr bool overlayBones = true;                // Show bone overlay
constexpr bool forceBoneOverlayToBindPose = false; // Show bind instead of animated pose overlay
constexpr float boneOverlayPointSize = 3.0f;

// Window constants
constexpr char windowTitle[] = "AEM Viewer";

// OpenGL constants
constexpr GLsizei shaderInfoLogLength = 512;

// Model constants
constexpr float modelRotation = 0.0f; // In degrees
constexpr float modelScale = 0.01f;

// Animation constants
constexpr float animationSpeed = 0.5f;

// Color constants
constexpr glm::vec4 backgroundColor = { 0.25f, 0.25f, 0.25f, 1.0f };
constexpr glm::vec4 geometryColor = { 0.9f, 0.4f, 0.1f, 1.0f };
constexpr glm::vec4 gridOverlayMajorColor = { 0.5f, 0.5f, 0.5f, 1.0f };
constexpr glm::vec4 gridOverlayMinorColor = { 0.35f, 0.35f, 0.35f, 1.0f };
constexpr glm::vec4 boneOverlayColor = { 0.9f, 0.95f, 0.99f, 1.0f };

// Camera constants
constexpr float cameraMinDistance = 0.5f;
constexpr float cameraNear = 0.1f;
constexpr float cameraFar = 100.0f;
constexpr float cameraFov = glm::radians(45.0f); // Vertical field of view in radians

// Camera variables
glm::vec2 lastMousePosition;
bool leftMouseButtonDown = false, rightMouseButtonDown = false;
glm::vec3 cameraPosition = { 0.0f, 0.4f, -4.0f }, cameraPivot = { 0.0f, 0.4f, 0.0f };
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

// Window variables
int windowWidth = 640;
int windowHeight = 400;

// Geometry variables
std::vector<MeshVertex> meshVertices;
std::vector<DebugVertex> gridOverlayVertices, boneOverlayVertices;
std::vector<uint32_t> indices;
std::vector<uint32_t> meshLengths;
std::vector<Bone> bones;
std::vector<Keyframe> keyframes;

// Animation variables
std::vector<glm::mat4> fromBoneTransforms, toBoneTransforms, finalBoneTransforms;
float animationTime = 0.0f;

void cursorPositionCallback(GLFWwindow* window, double x, double y)
{
  const glm::vec2 delta = glm::vec2(static_cast<float>(x), static_cast<float>(y)) - lastMousePosition;

  const glm::vec3 forward = glm::normalize(cameraPivot - cameraPosition);
  glm::vec3 up = { 0.0f, 1.0f, 0.0f };
  glm::vec3 right = glm::normalize(glm::cross(up, forward));
  up = glm::normalize(glm::cross(forward, right));

  // Tumble the camera
  if (leftMouseButtonDown && !rightMouseButtonDown)
  {
    // Horizontal
    {
      const float angle = (delta.x * glm::pi<float>()) / static_cast<float>(windowWidth);
      const glm::mat3 tumble = glm::rotate(glm::mat4(1.0f), -angle, up);
      const glm::vec3 cameraVector = cameraPosition - cameraPivot;
      cameraPosition = cameraPivot + tumble * cameraVector;
    }

    // Vertical
    {
      const float angle = (delta.y * glm::pi<float>()) / static_cast<float>(windowHeight);
      const glm::mat3 tumble = glm::rotate(glm::mat4(1.0f), angle, right);
      const glm::vec3 cameraVector = cameraPosition - cameraPivot;
      cameraPosition = cameraPivot + tumble * cameraVector;
    }
  }
  // Pan the camera
  else if (rightMouseButtonDown && !leftMouseButtonDown)
  {
    const float cameraToPivotLength = glm::length(cameraPivot - cameraPosition);

    // Horizontal
    {
      const glm::vec3 pan = (right * delta.x) / static_cast<float>(windowWidth) * cameraToPivotLength;
      cameraPosition += pan;
      cameraPivot += pan;
    }

    // Vertical
    {
      const glm::vec3 pan = (up * delta.y) / static_cast<float>(windowHeight) * cameraToPivotLength;
      cameraPosition += pan;
      cameraPivot += pan;
    }
  }

  lastMousePosition.x = static_cast<float>(x);
  lastMousePosition.y = static_cast<float>(y);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  // Store mouse button state
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    leftMouseButtonDown = (action == GLFW_PRESS);
  }
  else if (button == GLFW_MOUSE_BUTTON_RIGHT)
  {
    rightMouseButtonDown = (action == GLFW_PRESS);
  }
}

void scrollCallback(GLFWwindow* window, double x, double y)
{
  // Dolly the camera
  const glm::vec3 cameraVector = cameraPivot - cameraPosition;
  const float length = glm::length(cameraVector);
  float newLength = length - static_cast<float>(y) * length / 10.0f;
  cameraPosition = cameraPivot - glm::normalize(cameraVector) * newLength;
}

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
  windowWidth = width;
  windowHeight = height;

  // Recalculate projection matrix
  {
    const float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    projectionMatrix = glm::perspective(cameraFov, aspectRatio, cameraNear, cameraFar);
  }
}

void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

void evaluatePose(size_t keyframeIndex, std::vector<glm::mat4>& matrices)
{
  const Keyframe& keyframe = keyframes.at(keyframeIndex);

  for (size_t i = 0u; i < bones.size(); ++i)
  {
    Bone& bone = bones.at(i);

    glm::mat4 posedTransform = keyframe.matrices.at(i);
    int index = bone.parentIndex;
    while (index > -1)
    {
      posedTransform *= keyframe.matrices.at(index);
      index = bones.at(index).parentIndex;
    }

    matrices.at(i) = posedTransform * bone.inverseBindPoseMatrix;
  }
}

DecomposedMatrix decomposeMatrix(const glm::mat4& matrix)
{
  DecomposedMatrix decomposedMatrix;

  decomposedMatrix.position = matrix[3];

  glm::mat3 matrix3 = glm::mat3(matrix); // Copy the upper-left 3x3 of the matrix
  decomposedMatrix.scale = { glm::length(matrix3[0]), glm::length(matrix3[1]), glm::length(matrix3[2]) };

  // Normalize for glm::quat_cast to work properly
  matrix3[0] /= decomposedMatrix.scale.x;
  matrix3[1] /= decomposedMatrix.scale.y;
  matrix3[2] /= decomposedMatrix.scale.z;
  decomposedMatrix.rotation = glm::quat_cast(matrix3);

  return decomposedMatrix;
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
      std::cout << "Number of keyframes: " << header.numKeyframes << "\n";

      meshVertices.resize(header.numVertices);
      indices.resize(static_cast<size_t>(header.numTriangles) * 3u);
      meshLengths.resize(header.numMeshes);
      bones.resize(header.numBones);
      keyframes.resize(header.numKeyframes);
    }

    // Read data
    file.read(reinterpret_cast<char*>(meshVertices.data()), sizeof(meshVertices.at(0u)) * meshVertices.size());
    file.read(reinterpret_cast<char*>(indices.data()), sizeof(indices.at(0u)) * indices.size());
    file.read(reinterpret_cast<char*>(meshLengths.data()), sizeof(meshLengths.at(0u)) * meshLengths.size());
    file.read(reinterpret_cast<char*>(bones.data()), sizeof(bones.at(0u)) * bones.size());

    for (Keyframe& keyframe : keyframes)
    {
      // Read keyframe time
      file.read(reinterpret_cast<char*>(&keyframe.time), sizeof(keyframe.time));

      // Read animated bone matrices at the keyframe
      keyframe.matrices.resize(bones.size());
      file.read(reinterpret_cast<char*>(keyframe.matrices.data()),
                sizeof(keyframe.matrices.at(0u)) * keyframe.matrices.size());
    }

    fromBoneTransforms.resize(bones.size());
    toBoneTransforms.resize(bones.size());
    finalBoneTransforms.resize(bones.size());

    file.close();
  }

  // Generate debug geometry
  {
    if (overlayGrid)
    {
      gridOverlayVertices.resize(numGridOverlayLines * 2u * 2u);

      // Minor lines
      constexpr float length = static_cast<float>(numGridOverlayCells);
      size_t index = 0u;
      {
        for (size_t i = 0; i < numGridOverlayLines; ++i)
        {
          if (i == numGridOverlayCells)
          {
            continue;
          }

          const float offset = static_cast<float>(i) - static_cast<float>(numGridOverlayCells);
          gridOverlayVertices.at(index++).position = glm::vec3(offset, 0.0f, -length);
          gridOverlayVertices.at(index++).position = glm::vec3(offset, 0.0f, length);
          gridOverlayVertices.at(index++).position = glm::vec3(-length, 0.0f, offset);
          gridOverlayVertices.at(index++).position = glm::vec3(length, 0.0f, offset);
        }
      }

      // Major lines
      {

        gridOverlayVertices.at(index++).position = glm::vec3(-length, 0.0f, 0.0f);
        gridOverlayVertices.at(index++).position = glm::vec3(length, 0.0f, 0.0f);
        gridOverlayVertices.at(index++).position = glm::vec3(0.0f, 0.0f, -length);
        gridOverlayVertices.at(index++).position = glm::vec3(0.0f, 0.0f, length);
      }
    }

    if (overlayBones)
    {
      boneOverlayVertices.resize(1u);
      boneOverlayVertices.at(0u).position = glm::vec3(0.0f);
    }
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
    glfwSetWindowSizeCallback(window, windowResizeCallback);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    glfwMakeContextCurrent(window);
    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
      std::cerr << "Failed to load OpenGL";
      return EXIT_FAILURE;
    }

    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(boneOverlayPointSize);

    // Initialize projection matrix
    {
      const float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
      projectionMatrix = glm::perspective(cameraFov, aspectRatio, cameraNear, cameraFar);
    }
  }

  // Set up geometry
  GLuint meshVertexArray, gridOverlayVertexArray, boneOverlayVertexArray;
  {
    // Generate mesh vertex array
    {
      glGenVertexArrays(1, &meshVertexArray);
      glBindVertexArray(meshVertexArray);

      // Generate and fill a vertex buffer
      constexpr GLsizei vertexSize = sizeof(meshVertices.at(0u));
      {
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexSize * meshVertices.size()), meshVertices.data(),
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

    // Generate grid overlay vertex array
    {
      glGenVertexArrays(1, &gridOverlayVertexArray);
      glBindVertexArray(gridOverlayVertexArray);

      // Generate and fill a vertex buffer
      constexpr GLsizei vertexSize = sizeof(gridOverlayVertices.at(0u));
      {
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexSize * gridOverlayVertices.size()),
                     gridOverlayVertices.data(), GL_STATIC_DRAW);
      }

      // Apply the vertex definition
      {
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize,
                              reinterpret_cast<void*>(offsetof(DebugVertex, position)));
      }
    }

    // Generate bone overlay vertex array
    {
      glGenVertexArrays(1, &boneOverlayVertexArray);
      glBindVertexArray(boneOverlayVertexArray);

      // Generate and fill a vertex buffer
      constexpr GLsizei vertexSize = sizeof(boneOverlayVertices.at(0u));
      {
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexSize * boneOverlayVertices.size()),
                     boneOverlayVertices.data(), GL_STATIC_DRAW);
      }

      // Apply the vertex definition
      {
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize,
                              reinterpret_cast<void*>(offsetof(DebugVertex, position)));
      }
    }
  }

  // Set up a mesh shader program
  GLuint meshShaderProgram;
  GLint meshShaderWorldUniformLocation, meshShaderViewUniformLocation, meshShaderProjectionUniformLocation,
    meshShaderBoneTransformsUniformLocation;
  {
    // Compile the vertex shader
    GLuint vertexShader;
    {
      vertexShader = glCreateShader(GL_VERTEX_SHADER);

      const GLchar* source = R"(#version 330 core
                                uniform mat4 world;
                                uniform mat4 view;
                                uniform mat4 projection;
                                uniform mat4 boneTransforms[100];
                                layout(location = 0) in vec3 inPosition;
                                layout(location = 1) in vec3 inNormal;
                                layout(location = 2) in vec3 inTangent;
                                layout(location = 3) in vec2 inUV;
                                layout(location = 4) in ivec4 inBoneIds;
                                layout(location = 5) in vec4 inBoneWeights;
                                out vec3 normal;
                                void main()
                                {
                                  mat4 boneTransform = mat4(0.0);
                                  for (int i = 0; i < 4; ++i)
                                  {
                                    boneTransform += boneTransforms[inBoneIds[i]] * inBoneWeights[i];
                                  }
                                  gl_Position = projection * view * world * boneTransform * vec4(inPosition, 1.0);
                                  normal = normalize((boneTransform * vec4(inNormal, 0.0)).xyz);
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
                                  float diffuse = dot(normal, normalize(vec3(0.4, 1.0, -0.85)));
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

      // Retrieve world matrix uniform location
      {
        meshShaderWorldUniformLocation = glGetUniformLocation(meshShaderProgram, "world");
        if (meshShaderWorldUniformLocation < 0)
        {
          std::cerr << "Failed to get world matrix uniform location in mesh shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }
      }

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

      // Retrieve projection matrix uniform location
      {
        meshShaderProjectionUniformLocation = glGetUniformLocation(meshShaderProgram, "projection");
        if (meshShaderProjectionUniformLocation < 0)
        {
          std::cerr << "Failed to get projection matrix uniform location in mesh shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }
      }

      // Retrieve bone transforms uniform location
      {
        meshShaderBoneTransformsUniformLocation = glGetUniformLocation(meshShaderProgram, "boneTransforms");
        if (meshShaderBoneTransformsUniformLocation < 0)
        {
          std::cerr << "Failed to get bone transforms uniform location in mesh shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }
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
  GLint debugShaderWorldUniformLocation, debugShaderViewUniformLocation, debugShaderProjectionUniformLocation,
    debugShaderColorUniformLocation;
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

      // Retrieve projection matrix uniform location
      {
        debugShaderProjectionUniformLocation = glGetUniformLocation(debugShaderProgram, "projection");
        if (debugShaderProjectionUniformLocation < 0)
        {
          std::cerr << "Failed to get projection matrix uniform location in debug shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }
      }

      // Retrieve color uniform location
      {
        debugShaderColorUniformLocation = glGetUniformLocation(debugShaderProgram, "color");
        if (debugShaderColorUniformLocation < 0)
        {
          std::cerr << "Failed to get color uniform location in debug shader program";
          glfwTerminate();
          return EXIT_FAILURE;
        }
      }
    }
  }

  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    // Update
    {
      // Populate the final bone transforms with a blend of the keyframe before and after our current animation time
      {
        animationTime += animationSpeed;

        // Ensure the animation time is somewhere between the beginning and end of the keyframe times
        {
          const float lastKeyframeTime = keyframes.at(keyframes.size() - 1u).time;
          while (animationTime > lastKeyframeTime)
          {
            animationTime -= lastKeyframeTime;
          }
        }

        // Find the keyframe index we are blending from (before our current animation time) and to (after)
        size_t fromKeyframeIndex = 0u, toKeyframeIndex = 1u;
        {
          while (animationTime > keyframes.at(fromKeyframeIndex + 1u).time)
          {
            ++fromKeyframeIndex;
            ++toKeyframeIndex;

            if (toKeyframeIndex >= keyframes.size())
            {
              toKeyframeIndex = 0u;
              break;
            }
          }
        }

        // Find the blend factor between the keyframes
        float blend;
        {
          const float fromTime = keyframes.at(fromKeyframeIndex).time;
          const float toTime = keyframes.at(toKeyframeIndex).time;
          blend = (animationTime - fromTime) / (toTime - fromTime);

          if (blend > 1.0f)
          {
            blend = 1.0f;
          }
          else if (blend < 0.0f)
          {
            blend = 0.0f;
          }
        }

        // Evaluate the animation at both keyframes and store the results for all bones
        evaluatePose(toKeyframeIndex, fromBoneTransforms);
        evaluatePose(toKeyframeIndex, toBoneTransforms);

        for (size_t i = 0u; i < finalBoneTransforms.size(); ++i)
        {
          // Decompose the bone transforms so they can be blended together
          const DecomposedMatrix from = decomposeMatrix(fromBoneTransforms.at(i));
          const DecomposedMatrix to = decomposeMatrix(toBoneTransforms.at(i));

          // Blend both transforms together and store the result as the final bone transform
          const glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::mix(from.position, to.position, blend));
          const glm::mat4 r = glm::toMat4(glm::slerp(from.rotation, to.rotation, blend));
          const glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::mix(from.scale, to.scale, blend));
          finalBoneTransforms.at(i) = t * r * s;
        }
      }

      // Update camera view matrix
      viewMatrix = glm::lookAt(cameraPosition, cameraPivot, { 0.0f, 1.0f, 0.0f });
    }

    // Render
    {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Render mesh
      {
        glBindVertexArray(meshVertexArray);
        glUseProgram(meshShaderProgram);

        // Set world matrix uniform
        {
          const glm::mat4 worldMatrix = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(modelScale)),
                                                    glm::radians(modelRotation), { 1.0f, 0.0f, 0.0f });
          glUniformMatrix4fv(meshShaderWorldUniformLocation, 1, GL_FALSE, glm::value_ptr(worldMatrix));
        }

        // Set view matrix uniform
        glUniformMatrix4fv(meshShaderViewUniformLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));

        // Set projection matrix uniform
        glUniformMatrix4fv(meshShaderProjectionUniformLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        // Set bone transforms uniform
        glUniformMatrix4fv(meshShaderBoneTransformsUniformLocation, static_cast<GLsizei>(finalBoneTransforms.size()),
                           GL_FALSE, glm::value_ptr(finalBoneTransforms.at(0)));

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
      }

      // Render grid overlay
      if (overlayGrid)
      {
        glEnable(GL_BLEND);

        glBindVertexArray(gridOverlayVertexArray);
        glUseProgram(debugShaderProgram);

        // Set world matrix uniform
        {
          const glm::mat4 worldMatrix = glm::mat4(1.0f);
          glUniformMatrix4fv(debugShaderWorldUniformLocation, 1, GL_FALSE, glm::value_ptr(worldMatrix));
        }

        // Set view matrix uniform
        glUniformMatrix4fv(debugShaderViewUniformLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));

        // Set projection matrix uniform
        glUniformMatrix4fv(debugShaderProjectionUniformLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        // Set color uniform
        {
          const glm::vec4 color = glm::vec4(gridOverlayMinorColor.r, gridOverlayMinorColor.g, gridOverlayMinorColor.b,
                                            gridOverlayMinorColor.a);
          glUniform4fv(debugShaderColorUniformLocation, 1, glm::value_ptr(color));
        }

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(gridOverlayVertices.size() - 4u));

        // Set color uniform
        {
          const glm::vec4 color = glm::vec4(gridOverlayMajorColor.r, gridOverlayMajorColor.g, gridOverlayMajorColor.b,
                                            gridOverlayMajorColor.a);
          glUniform4fv(debugShaderColorUniformLocation, 1, glm::value_ptr(color));
        }

        glDrawArrays(GL_LINES, static_cast<GLsizei>(gridOverlayVertices.size() - 4u), 4u);

        glDisable(GL_BLEND);
      }

      // Render bone overlay
      if (overlayBones)
      {
        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(boneOverlayVertexArray);

        // Set color uniform
        {
          const glm::vec4 color =
            glm::vec4(boneOverlayColor.r, boneOverlayColor.g, boneOverlayColor.b, boneOverlayColor.a);
          glUniform4fv(debugShaderColorUniformLocation, 1, glm::value_ptr(color));
        }

        for (size_t i = 0u; i < bones.size(); ++i)
        {
          const Bone& bone = bones.at(i);

          // The non-invertex bind pose matrix transforms from the model space origin to the bind pose of the bone
          glm::mat4 bindPoseMatrix = glm::inverse(bone.inverseBindPoseMatrix);

          // Transform it further to the animated pose of the bone if desired
          if (!forceBoneOverlayToBindPose)
          {
            bindPoseMatrix = finalBoneTransforms.at(i) * bindPoseMatrix;
          }

          // Set world matrix uniform
          {
            const glm::mat4 worldMatrix = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(modelScale)),
                                                      glm::radians(modelRotation), { 1.0f, 0.0f, 0.0f });
            glUniformMatrix4fv(debugShaderWorldUniformLocation, 1, GL_FALSE,
                               glm::value_ptr(worldMatrix * bindPoseMatrix));
          }

          glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(boneOverlayVertices.size()));
        }

        glEnable(GL_DEPTH_TEST);
      }

      glfwSwapBuffers(window);
    }

    glfwPollEvents();
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}