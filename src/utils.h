#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>
#include <glm/ext/matrix_transform.hpp>
#include <variant>

inline std::string ReadShaderFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);			//ate：从文件尾部开始读取

	if (!file.is_open()) {
		std::cout << "read " << filename << " failed" << std::endl;
		return std::string{};
	}

	auto size = file.tellg();
	std::string content;
	content.resize(size);
	file.seekg(0);

	file.read(content.data(), content.size());
	return content;
}

class RecX final {
public:
	RecX(glm::vec2 position, glm::vec2 size_) :pos(position), size(size_) {}
	RecX(glm::vec3 position, glm::vec3 size_) :pos(position), size(size_) {}
	template<typename Mat>
    Mat CreateMat() const
    {
        Mat model(1.0f);
        std::visit([&](auto&& p) {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, glm::vec2>) {
                model = glm::translate(model, glm::vec3(p, 0.0f));  // 2D → Z=0
            }
            else {
                model = glm::translate(model, p);                   // 3D
            }
            }, pos);

        //scale
        std::visit([&](auto&& s) {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, glm::vec2>) {
                model = glm::scale(model, glm::vec3(s, 1.0f));     // 2D → Z scale = 1
            }
            else {
                model = glm::scale(model, s);                      // 3D
            }
            }, size);

        return model;
    }

private:
	std::variant<glm::vec2, glm::vec3> pos;
	std::variant<glm::vec2, glm::vec3> size;
};

inline glm::vec3 MoveInScreenSpace(
    const glm::vec3& worldPos,
    float pixelX, float pixelY,
    int screenWidth, int screenHeight,
    const glm::mat4& view,
    const glm::mat4& proj)
{
    // world -> clip
    glm::vec4 clip = proj * view * glm::vec4(worldPos, 1.0f);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    // screen coordinates
    glm::vec2 screen;
    screen.x = (ndc.x * 0.5f + 0.5f) * screenWidth;
    screen.y = (ndc.y * 0.5f + 0.5f) * screenHeight;

    // move in screen space (pixels)
    screen.x += pixelX;
    screen.y += pixelY;

    // screen -> NDC
    glm::vec2 ndc2;
    ndc2.x = (screen.x / screenWidth) * 2.0f - 1.0f;
    ndc2.y = (screen.y / screenHeight) * 2.0f - 1.0f;

    // keep original depth
    float depth = ndc.z;

    glm::vec4 newClip(ndc2.x * depth, ndc2.y * depth, depth, 1.0f);

    glm::mat4 invVP = glm::inverse(proj * view);
    glm::vec4 newWorld = invVP * newClip;
    newWorld /= newWorld.w;

    return glm::vec3(newWorld);
}

inline glm::vec3 ScreenToWorldCenter(
    float distance,
    int screenWidth, int screenHeight,
    const glm::mat4& view,
    const glm::mat4& proj)
{
    // 屏幕中心
    float screenX = screenWidth * 0.5f;
    float screenY = screenHeight * 0.5f;

    // 转为NDC
    glm::vec2 ndc;
    ndc.x = (screenX / screenWidth) * 2.0f - 1.0f;
    ndc.y = (screenY / screenHeight) * 2.0f - 1.0f;

    // 给定深度（负Z方向，camera forward）
    float z = -distance;

    glm::vec4 clip(ndc.x * z, ndc.y * z, z, 1.0f);

    glm::mat4 invVP = glm::inverse(proj * view);
    glm::vec4 world = invVP * clip;
    world /= world.w;

    return glm::vec3(world);
}
#endif