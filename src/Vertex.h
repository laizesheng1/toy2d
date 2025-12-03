#ifndef VERTEX_H
#define VERTEX_H
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace toy2d {
    struct Vertex final {
        glm::vec3 pos;
        glm::vec2 texCoord;
        //glm::vec3 color;

        static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 2> attributes;

            //属性 0: 位置 (pos)
            attributes[0].setBinding(0);           // 绑定到顶点缓冲区绑定点 0
            attributes[0].setLocation(0);           // 在顶点着色器中的 location = 0
            attributes[0].setFormat(vk::Format::eR32G32B32Sfloat);     // 3个 32 位浮点数
            attributes[0].setOffset(offsetof(Vertex, pos));         // 在结构体中的偏移量

            //属性 1:
            attributes[1].setBinding(0);
            attributes[1].setLocation(1);           // 在顶点着色器中的 location = 1
            attributes[1].setFormat(vk::Format::eR32G32Sfloat);      // 2个 32 位浮点数
            attributes[1].setOffset(offsetof(Vertex, texCoord));           // 在结构体中的偏移量

            return attributes;
        }

        /*static vk::VertexInputAttributeDescription GetAttributeDescription()
        {
            vk::VertexInputAttributeDescription description;
            description.setBinding(0)
                .setFormat(vk::Format::eR32G32Sfloat)
                .setLocation(0)
                .setOffset(0);
            return description;
        }*/

        static vk::VertexInputBindingDescription GetBinding()
        {
            vk::VertexInputBindingDescription binding;
            binding.setBinding(0)
                .setInputRate(vk::VertexInputRate::eVertex)
                .setStride(sizeof(Vertex));
            return binding;
        }
    };
}
#endif // !VERTEX_H