#ifndef VERTEX_H
#define VERTEX_H
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace toy2d {
    struct Vertex final {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 3> attributes;

            //pos
            attributes[0].setBinding(0);           // 绑定到顶点缓冲区绑定点 0
            attributes[0].setLocation(0);           // 在顶点着色器中的 location = 0
            attributes[0].setFormat(vk::Format::eR32G32B32Sfloat);     // 3个 32 位浮点数
            attributes[0].setOffset(offsetof(Vertex, pos));         // 在结构体中的偏移量
            //color
            attributes[1].setBinding(0);
            attributes[1].setLocation(1);           // 在顶点着色器中的 location = 1
            attributes[1].setFormat(vk::Format::eR32G32B32Sfloat);      // 2个 32 位浮点数
            attributes[1].setOffset(offsetof(Vertex, color));
            //texCoord
            attributes[2].setBinding(0);
            attributes[2].setLocation(2);           // 在顶点着色器中的 location = 2
            attributes[2].setFormat(vk::Format::eR32G32Sfloat);      // 2个 32 位浮点数
            attributes[2].setOffset(offsetof(Vertex, texCoord));           // 在结构体中的偏移量

            return attributes;
        }

        static vk::VertexInputBindingDescription GetBinding()
        {
            vk::VertexInputBindingDescription binding;
            binding.setBinding(0)
                .setInputRate(vk::VertexInputRate::eVertex)
                .setStride(sizeof(Vertex));
            return binding;
        }

        bool operator==(const Vertex& other) const {
            return pos == other.pos && color == other.color && texCoord == other.texCoord;
        }
    };
}

namespace std {
    template<> struct hash<toy2d::Vertex> {
        size_t operator()(toy2d::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

#endif // !VERTEX_H