#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>

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

#endif