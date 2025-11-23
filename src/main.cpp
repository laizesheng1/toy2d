#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <toy2d.h>

toy2d::Renderer* renderer = nullptr;
float x = 100, y = 100;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_A:
            x -= 10;
            break;
        case GLFW_KEY_D:
            x += 10;
            break;
        case GLFW_KEY_W:
            y -= 10;
            break;
        case GLFW_KEY_S:
            y += 10;
            break;
        case GLFW_KEY_1:
            renderer->SetDrawColor(toy2d::Color{ 1, 0, 0 });
            break;
        case GLFW_KEY_2:
            renderer->SetDrawColor(toy2d::Color{ 0, 1, 0 });
            break;
        case GLFW_KEY_3:
            renderer->SetDrawColor(toy2d::Color{ 0, 0, 1 });
            break;
        }
    }
}

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    
    //获取当前"系统"支持的所有实例扩展的列表
    vkEnumerateInstanceExtensionProperties(nullptr,
        &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported" << std::endl;

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
    
    renderer = toy2d::Getrender();
    glfwSetKeyCallback(window, key_callback);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        renderer->render(Rec2D({ x,y }, { 200,300 }));
    }

    toy2d::Quit();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}