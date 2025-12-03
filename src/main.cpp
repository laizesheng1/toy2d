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
    //float x = 400, y = 300;
    glm::vec3 pos;
    glm::vec3 size;
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
        pos = ScreenToWorldCenter(-0.95, width, height, renderer->viewMat_, renderer->projectMat_);
    }

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_A:
                app->pos= MoveInScreenSpace(app->pos, -10, 0,
                    app->width, app->height,
                    app->renderer->viewMat_, app->renderer->projectMat_);
                //app->x -= 10;
                break;
            case GLFW_KEY_D:
                app->pos = MoveInScreenSpace(app->pos, 10, 0,
                    app->width, app->height,
                    app->renderer->viewMat_, app->renderer->projectMat_);
                //app->x += 10;
                break;
            case GLFW_KEY_W:
                app->pos = MoveInScreenSpace(app->pos, 0, -10,
                    app->width, app->height,
                    app->renderer->viewMat_, app->renderer->projectMat_);
                //app->y -= 10;
                break;
            case GLFW_KEY_S:
                app->pos = MoveInScreenSpace(app->pos, 0, 10,
                    app->width, app->height,
                    app->renderer->viewMat_, app->renderer->projectMat_);
                //app->y += 10;
                break;
            case GLFW_KEY_1:
                app->renderer->SetDrawColor(toy2d::Color{ 1, 0, 0 });
                break;
            case GLFW_KEY_2:
                app->renderer->SetDrawColor(toy2d::Color{ 0, 1, 0 });
                break;
            case GLFW_KEY_3:
                app->renderer->SetDrawColor(toy2d::Color{ 0, 0, 1 });
                break;
            case GLFW_KEY_0:
                app->renderer->SetDrawColor(toy2d::Color{ 1, 1, 1 });
                break;
            }
        }
    }

    void mainLoop() {
        toy2d::Image* image1 = toy2d::ImageManager::GetInstance().Get(0);
        toy2d::Image* image2 = toy2d::ImageManager::GetInstance().Get(1);
        const std::vector<toy2d::Image*> images = { image1,image2 };
        glfwSetKeyCallback(window, key_callback);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            if (framebufferResized) {
                framebufferResized = false;
                glfwGetFramebufferSize(window, &width, &height);
                toy2d::recreateSwapChain(width, height);
                renderer->SetVPMat(width, height);
                continue;
            }
            renderer->startRender();
            //renderer->DrawTexture(Rec2D({ 0,0 }, { 100,150 }), image1);       
            renderer->DrawTexture(RecX(pos, { 1,1 ,1}), image2);
            renderer->endRender();
        }
        toy2d::DestroyTexture(image1);
        toy2d::DestroyTexture(image2);
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