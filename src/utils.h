#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>
#include <glm/ext/matrix_transform.hpp>

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

class Rec2D final {
public:
	Rec2D(glm::vec2 position, glm::vec2 size_) :pos(position), size(size_) {}
	template<typename Mat>
	Mat CreateMat() {
		Mat res = glm::translate(Mat(1.0f), glm::vec3(pos, 0.0f)) * glm::scale(Mat(1.0f), glm::vec3(size, 1.0f));
		return res;
		//Mat mat1 = glm::identity<Mat>(), mat2 = glm::identity<Mat>();
		//mat1[3][0] = pos.x;
		//mat1[3][1] = pos.y;
		//mat2[0][0] = size.x;
		//mat2[1][1] = size.y;
		//return mat1*mat2;
	}
private:
	glm::vec2 pos;
	glm::vec2 size;
};

#endif