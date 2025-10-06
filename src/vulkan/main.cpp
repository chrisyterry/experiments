#include "vulkan/modern_render_triangle.hpp"

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