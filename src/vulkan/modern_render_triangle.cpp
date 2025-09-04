#include "vulkan/modern_render_triangle.hpp"

void ModernRenderTriangle::initWindow() {
    // initialize GLFW without openGL stuff
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create the GLFW window (fourth param is monitor to open window on, last is openGL only)
    m_window = glfwCreateWindow(m_window_size.first, m_window_size.second, "vulkan", nullptr, nullptr);
}

void ModernRenderTriangle::initVulkan() {

}

void ModernRenderTriangle::mainLoop() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
    }
}

void ModernRenderTriangle::cleanup() {
    // cleanup glfw
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void ModernRenderTriangle::run() {

}

int main() {
    ModernRenderTriangle renderer;

    try { 
        renderer.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}