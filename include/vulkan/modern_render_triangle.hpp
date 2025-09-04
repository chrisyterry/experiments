#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

class ModernRenderTriangle {
  public:
    /**
     * @brief run the render triangle applciation
     */
    void run();

  private:
    /**
     * @brief initialize vulkan and its dependencies
     */
    void initVulkan();

    /**
     * @brief initialize the window to render to
     */
    void initWindow();

    /**
     * @brief main run loop
     */
    void mainLoop();

    /**
     * @brief cleansup vulkan and its dependencies
     */
    void cleanup();

    const std::pair<uint32_t, uint32_t> m_window_size = { 800, 800 };
    GLFWwindow*                         m_window;  // GLFW window to render to
};