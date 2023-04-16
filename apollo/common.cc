#include "common.h"
#include <cstring>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
using namespace apollo;

thread_local pid_t       t_cachedTid  = 0;
thread_local std::string t_threadName = "thread";

pid_t gettid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void cacheTid() {
    if (t_cachedTid == 0) {
        t_cachedTid = gettid();
    }
}

void setThreadName(std::thread* th, const std::string& name) {
    auto handle = th->native_handle();
    pthread_setname_np(handle, name.c_str());
}

pid_t ThreadHelper::ThreadId() {
    if (__builtin_expect(t_cachedTid == 0, 0)) {
        cacheTid();
    }
    return t_cachedTid;
}

const std::string& ThreadHelper::ThreadName() {
    return t_threadName;
}

void ThreadHelper::SetThreadName(std::thread* th, const std::string& name) {
    setThreadName(th, name);
}

static int __lstat(const char* file, struct stat* st = nullptr) {
    struct stat lst;
    int         ret = lstat(file, &lst);
    if (st) {
        *st = lst;
    }
    return ret;
}

static int __mkdir(const char* dirname) {
    if (access(dirname, F_OK) == 0) {
        return 0;
    }
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

bool FileHelper::Mkdir(const std::string& dirname) {
    if (__lstat(dirname.c_str()) == 0) {
        return true;
    }
    char* path = strdup(dirname.c_str());
    char* ptr  = strchr(path + 1, '/');
    do {
        for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
            *ptr = '\0';
            if (__mkdir(path) != 0) {
                break;
            }
        }
        if (ptr != nullptr) {
            break;
        } else if (__mkdir(path) != 0) {
            break;
        }
        free(path);
        return true;
    } while (0);
    free(path);
    return false;
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