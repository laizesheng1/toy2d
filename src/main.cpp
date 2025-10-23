#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <toy2d.h>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    
    //获取当前"系统"支持的所有实例扩展的列表
    vkEnumerateInstanceExtensionProperties(nullptr,
        &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported" << std::endl;

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    //获取创建"窗口"所必需的扩展列表
    unsigned int count;
    const char** glfwExtensions;
    glfwExtensions= glfwGetRequiredInstanceExtensions(&count);
    const std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
    /*for (auto& extension : extensions)
    {
        std::cout << extension << std::endl;
    }*/

    toy2d::Init(extensions, [&](vk::Instance instance) {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("create surface is failed");
        }
        return surface;
        }, 800, 600);
    auto renderer = toy2d::Getrender();
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.render();
    }

    toy2d::Quit();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
