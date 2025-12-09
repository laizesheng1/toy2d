#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <toy2d.h>
#include <image.h>

class Application {
private:
    GLFWwindow* window;
    toy2d::Renderer* renderer = nullptr;
    bool framebufferResized = true;
    int width=800, height=600;
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        if (width == 0 || height == 0) return;
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetWindowUserPointer(window, this);
    }

    void initVulkan()
    {
        uint32_t extensionCount = 0;
        //获取当前"系统"支持的所有实例扩展的列表
        vkEnumerateInstanceExtensionProperties(nullptr,
            &extensionCount, nullptr);
        std::cout << extensionCount << " extensions supported" << std::endl;

        //获取创建"窗口"所必需的扩展列表
        unsigned int count;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
        const std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);

        toy2d::Init(extensions, [&](vk::Instance instance) {
            VkSurfaceKHR surface;
            if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            {
                throw std::runtime_error("create surface is failed");
            }
            return surface;
            }, width, height);

        renderer = toy2d::Getrender();
    }

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_A:
                //app->x -= 10;
                break;
            case GLFW_KEY_D:
                //app->x += 10;
                break;
            case GLFW_KEY_W:
                //app->y -= 10;
                break;
            case GLFW_KEY_S:
                //app->y += 10;
                break;
            case GLFW_KEY_1:
                break;
            case GLFW_KEY_2:
                break;
            case GLFW_KEY_3:
                break;
            case GLFW_KEY_0:
                break;
            }
        }
    }

    void mainLoop() {
        toy2d::Image* image1 = toy2d::ImageManager::GetInstance().Get(0);
        glfwSetKeyCallback(window, key_callback);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            if (framebufferResized) {
                framebufferResized = false;
                glfwGetFramebufferSize(window, &width, &height);
                toy2d::recreateSwapChain(width, height);
                //renderer->SetVPMat(width, height);
                continue;
            }
            renderer->startRender();
            //renderer->DrawTexture(Rec2D({ 0,0 }, { 100,150 }), image1);       
            renderer->DrawTexture();
            renderer->endRender();
        }
        toy2d::DestroyTexture(image1);
    }
    void cleanup()
    {
        toy2d::Quit();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    Application app;
    app.run();
    return 0;
}