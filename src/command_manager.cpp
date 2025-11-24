#include "command_manager.h"
#include "context.h"

namespace toy2d {

    CommandManager::CommandManager() {
        pool_ = createCommandPool();
    }

    CommandManager::~CommandManager() {
        auto& ctx = Context::Getinstance();
        ctx.device.destroyCommandPool(pool_);
    }

    void CommandManager::ResetCmds() {
        Context::Getinstance().device.resetCommandPool(pool_);
    }

    vk::CommandPool CommandManager::createCommandPool() {
        auto& ctx = Context::Getinstance();

        vk::CommandPoolCreateInfo createInfo;

        createInfo.setQueueFamilyIndex(ctx.queueFamilyIndices.graphicsFamily.value())               //
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        return ctx.device.createCommandPool(createInfo);
    }

    std::vector<vk::CommandBuffer> CommandManager::CreateCommandBuffers(std::uint32_t count) {
        auto& ctx = Context::Getinstance();

        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.setCommandPool(pool_)
            .setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary);

        return ctx.device.allocateCommandBuffers(allocInfo);
    }

    vk::CommandBuffer CommandManager::CreateOneCommandBuffer() {
        return CreateCommandBuffers(1)[0];
    }

    void CommandManager::FreeCmd(vk::CommandBuffer buf) {
        Context::Getinstance().device.freeCommandBuffers(pool_, buf);
    }
    
    void CommandManager::ExecuteCmd(vk::Queue queue, RecordCmdFuc func)
    {
        auto cmdBuf = Context::Getinstance().commandManager->CreateOneCommandBuffer();
        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmdBuf.begin(beginInfo);
        {
            if (func)
                func(cmdBuf);
        }
        cmdBuf.end();

        vk::SubmitInfo submit;
        submit.setCommandBuffers(cmdBuf);
        queue.submit(submit);
        queue.waitIdle();
        Context::Getinstance().device.waitIdle();			//等待传输完成
        Context::Getinstance().commandManager->FreeCmd(cmdBuf);
    }

}