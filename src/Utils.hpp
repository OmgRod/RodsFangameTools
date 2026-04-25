#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;
namespace Utils {
    inline bool isCompressedLevel(const std::string& s) {
        size_t i = s.find_first_not_of(" \t\n\r");
        return i != std::string::npos && s.compare(i, 4, "H4sI") == 0;
    };
    inline std::string ltrim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        return (start == std::string::npos) ? "" : s.substr(start);
    };
   LevelSettingsObject* LevelSettingsObject(const std::string& s) {
        size_t pos = s.find(';');
        return (pos == std::string::npos) ? LevelSettingsObject::objectFromString(s) : LevelSettingsObject::objectFromString(s.substr(0, pos));
    };
}
