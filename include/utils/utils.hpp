#pragma once

#include <string>
#include <iostream>
#include <unistd.h>

namespace experiments::utils {

/**
 * @brief get the path of the executable/binary the function is called in
 * @note the returned path includes the binary name
 * 
 * @return the binary path
 */
std::string getExecutablePath() {
    char buffer[256];
    int  len = sizeof(buffer);
#if defined _WIN32
    int bytes = GetModuleFileName(NULL, buffer, len);
#else
    int bytes = std::min(static_cast<int>(readlink("/proc/self/exe", buffer, len)), len - 1);
    if (bytes >= 0) {
        buffer[bytes] = '\0';
    }
#endif
    std::string path(buffer);
    return path;
}
}  // namespace experiments::utils