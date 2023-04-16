#ifndef __APOLLO_COMMON_H__
#define __APOLLO_COMMON_H__

#include <fstream>
#include <ios>
#include <string>
#include <thread>

namespace apollo {

class ThreadHelper {
public:
    /**
     * @brief 获取线程id
     * 
     * @return pid_t 
     */
    static pid_t ThreadId();

    /**
     * @brief 获取线程名称
     * 
     * @return const std::string& 
     */
    static const std::string& ThreadName();

    /**
     * @brief 设置线程名称
     * 
     * @param th 线程指针
     * @param name 线程名称
     */
    static void SetThreadName(std::thread* th, const std::string& name);
};

class FileHelper {
public:
    /**
     * @brief 创建文件夹
     *
     * @param dirname 文件夹名称
     * @return true 创建成功
     * @return false 创建失败
     */
    static bool Mkdir(const std::string& dirname);

    /**
     * @brief 获取路径信息
     *
     * @param filename 文件名
     * @return std::string 返回路径信息
     */
    static std::string Dirname(const std::string& filename);

    /**
     * @brief 以写方式打开文件
     *
     * @param os 文件流
     * @param filename 文件名称
     * @param mode 打开方式
     * @return true 打开成功
     * @return false 打开失败
     */
    static bool OpenForWrite(std::ofstream& os, const std::string& filename, std::ios_base::openmode mode);
};
} // namespace apollo

#endif // !__APOLLO_COMMON_H__