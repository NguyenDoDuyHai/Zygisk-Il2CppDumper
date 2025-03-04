#include <cstring>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cinttypes>
#include <fstream>
#include <string>
#include "hack.h"
#include "zygisk.hpp"
#include "log.h"

std::string getPackageNameFromFile() {
    const char *filePath = "/data/local/tmp/game_package.txt";
    std::ifstream file(filePath);
    std::string packageName;

    if (file.is_open()) {
        std::getline(file, packageName);
        file.close();
        packageName.erase(packageName.find_last_not_of(" \n\r\t") + 1);
        return packageName;
    }

    return "";
}

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        auto package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        auto app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        preSpecialize(package_name, app_data_dir);
        env->ReleaseStringUTFChars(args->nice_name, package_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            std::thread hack_thread(hack_prepare, game_data_dir, data, length);
            hack_thread.detach();
        }
    }

private:
    Api *api;
    JNIEnv *env;
    bool enable_hack = false;
    char *game_data_dir = nullptr;
    void *data = nullptr;
    size_t length = 0;

    void preSpecialize(const char *package_name, const char *app_data_dir) {
        std::string filePackageName = getPackageNameFromFile();
        if (filePackageName.empty()) {
            LOGW("Không đọc được package name từ game_package.txt");
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        LOGI("Package thực tế từ Zygisk: %s", package_name);
        LOGI("Package từ game_package.txt: %s", filePackageName.c_str());

        if (strcmp(package_name, filePackageName.c_str()) == 0) {
            LOGI("Detect game: %s", package_name);
            enable_hack = true;

            game_data_dir = new char[strlen(app_data_dir) + 1];
            strcpy(game_data_dir, app_data_dir);

#if defined(__i386__)
            auto path = "zygisk/armeabi-v7a.so";
#endif
#if defined(__x86_64__)
            auto path = "zygisk/arm64-v8a.so";
#endif
#if defined(__i386__) || defined(__x86_64__)
            int dirfd = api->getModuleDir();
            int fd = openat(dirfd, path, O_RDONLY);
            if (fd != -1) {
                struct stat sb{};
                fstat(fd, &sb);
                length = sb.st_size;
                data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
                close(fd);
            } else {
                LOGW("Unable to open arm file");
            }
#endif
        } else {
            LOGW("Package không khớp, bỏ qua. (Expect: %s - Actual: %s)", filePackageName.c_str(), package_name);
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }
};

REGISTER_ZYGISK_MODULE(MyModule)
