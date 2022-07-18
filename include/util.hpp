#pragma once

#include <fstream>
#include <vector>

#include "constants.hpp"

namespace gfx {

inline std::vector<char> read_file(const char* filename)
{
	std::ifstream file((Root::path / filename).string(), std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("failed to open file");

	size_t file_size = (size_t)file.tellg();
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
};

}
