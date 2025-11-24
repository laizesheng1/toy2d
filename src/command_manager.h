#pragma once
#include "vulkan/vulkan.hpp"
#include <functional>

namespace toy2d {

    class CommandManager final {
    public:
        CommandManager();
        ~CommandManager();

        vk::CommandBuffer CreateOneCommandBuffer();
        std::vector<vk::CommandBuffer> CreateCommandBuffers(std::uint32_t count);
        void ResetCmds();
        void FreeCmd(vk::CommandBuffer);
        using RecordCmdFuc = std::function<void(vk::CommandBuffer&)>;
        void ExecuteCmd(vk::Queue queue, RecordCmdFuc func);            //实现某个需要在队列进行传输的命令，如缓冲区到图像的复制操作、unifrom的数据到buffer的传输

    private:
        vk::CommandPool pool_;

        vk::CommandPool createCommandPool();
    };

}