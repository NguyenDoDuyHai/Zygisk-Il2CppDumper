//
// Created by Perfare on 2020/7/4.
//

#ifndef ZYGISK_IL2CPPDUMPER_GAME_H
#define ZYGISK_IL2CPPDUMPER_GAME_H

#include <fstream>
#include <string>

std::string getPackageNameFromFile() {
    std::ifstream file("/data/local/tmp/game_package.txt");
    std::string packageName;
    if (file.is_open()) {
        std::getline(file, packageName);  // đọc đúng dòng đầu tiên
        file.close();
    } else {
        packageName = "com.default.game";  // fallback khi file không tồn tại hoặc lỗi
    }
    return packageName;
}


#endif //ZYGISK_IL2CPPDUMPER_GAME_H
