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

    inline std::string processLevelString(const std::string& input) {
        std::string working = input;
        bool wasCompressed = Utils::isCompressedLevel(input);

        if (wasCompressed) {
            working = ZipUtils::decompressString(working, false, 1);
        }

        std::vector<std::string> segments;
        std::stringstream ss(working);
        std::string segment;

        while (std::getline(ss, segment, ';')) {
            if (!segment.empty()) {
                segments.push_back(segment);
            }
        }

        std::vector<std::string> rebuiltSegments;

        for (auto& seg : segments) {
            std::vector<std::string> values;
            std::stringstream vs(seg);
            std::string val;

            while (std::getline(vs, val, ',')) {
                if (!val.empty()) {
                    values.push_back(val);
                }
            }

            if (values.size() % 2 != 0) {
                continue;
            }

            std::unordered_map<std::string, std::string> obj;

            for (size_t i = 0; i < values.size(); i += 2) {
                obj[values[i]] = values[i + 1];
            }

            auto it = obj.find("1");
            auto setting = Mod::get()->getSettingValue<std::string>("coin-type");
            if (setting == "User" && it != obj.end() && it->second == "142") {
                it->second = "1329";
            } else if (setting == "Secret" && it != obj.end() && it->second == "1329") {
                it->second = "142";
            }

            std::string rebuilt;
            for (const auto& [k, v] : obj) {
                if (!rebuilt.empty()) rebuilt += ",";
                rebuilt += k + "," + v;
            }

            rebuiltSegments.push_back(rebuilt);
        }

        std::string result;
        for (size_t i = 0; i < rebuiltSegments.size(); ++i) {
            if (i != 0) result += ";";
            result += rebuiltSegments[i];
        }

        if (wasCompressed) {
            result = ZipUtils::compressString(result, false, 1);
        }

        return result;
    }
}
