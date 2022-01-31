#include <glfw/glfw3.h>

#include <iostream>

namespace
{

// Window constants
constexpr char windowTitle[] = "AEM Viewer";
constexpr int windowWidth = 640;
constexpr int windowHeight = 400;

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

    window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
    if (!window)
    {
      std::cerr << "Failed to create window";
      glfwTerminate();
      return EXIT_FAILURE;
    }
  }

  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}