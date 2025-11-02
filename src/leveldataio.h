#pragma once
#include "leveldata.h"
#include <fstream>
#include "json.hpp"
using json = nlohmann::json;

// Serialize a Vector2
inline void to_json(json& j, const Vector2& v) {
    j = json{{"x", v.x}, {"y", v.y}};
}

inline void from_json(const json& j, Vector2& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
}

// Serialize a LevelData
inline void to_json(json& j, const LevelData& data) {
    j = json{
        {"wallPositions", data.wallPositions},
        {"spawnerPositions", data.spawnerPositions},
        {"targetTile", data.targetTile}
    };
}

inline void from_json(const json& j, LevelData& data) {
    j.at("wallPositions").get_to(data.wallPositions);
    j.at("spawnerPositions").get_to(data.spawnerPositions);
    j.at("targetTile").get_to(data.targetTile);
}

// Save multiple levels
inline void saveAllLevelsToFile(const vector<LevelData>& levels, const std::string& filename) {
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }
    json j = levels;
    out << j.dump(4);
}

// Load multiple levels
inline vector<LevelData> loadAllLevelsFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return {};
    }
    json j;
    in >> j;
    return j.get<vector<LevelData>>();
}
