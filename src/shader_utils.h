#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ShaderLoader {
    std::vector<uint32_t> readSpirv(const std::string& path);
};