#include "common.hpp"
using namespace apollo;

bool FileHelper::Mkdir(const std::string& dirname) {
    return true;
}

std::string FileHelper::Dirname(const std::string& filename) {
    if (filename.empty()) {
        return ".";
    }
    auto pos = filename.rfind('/');
    if (pos == 0) {
        return "/";
    } else if (pos == std::string::npos) {
        return ".";
    } else {
        return filename.substr(0, pos);
    }
}

bool FileHelper::OpenForWrite(std::ofstream& os, const std::string& filename, std::ios_base::openmode mode) {
    os.open(filename.c_str());
    if (!os.is_open()) {
        std::string dir = Dirname(filename);
        Mkdir(dir);
        os.open(filename.c_str(), mode);
    }
    return os.is_open();
}