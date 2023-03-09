#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <fstream>
#include <string>
#include <ios>

namespace apollo {

class ThreadHelper {
public:
    // 获取线程id
    static pid_t ThreadId();
};

class FileHelper {
public:
    static bool        Mkdir(const std::string& dirname);
    static std::string Dirname(const std::string& filename);
    static bool        OpenForWrite(std::ofstream& os, const std::string& filename, std::ios_base::openmode mode);
};
}

#endif // !__COMMON_HPP__