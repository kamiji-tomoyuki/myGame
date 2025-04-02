#include "JsonLoader.h"
#include <fstream>
#include <iostream>

bool JsonLoader::LoadFromFile(const std::string& filePath) {
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }
    try {
        file >> jsonData_;
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool JsonLoader::SaveToFile(const std::string& filePath) const {
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    try {
        file << jsonData_.dump(4);
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

std::optional<nlohmann::json> JsonLoader::GetValue(const std::string& key) const {
    if (jsonData_.contains(key)) {
        return jsonData_.at(key);
    }
    return std::nullopt;
}

void JsonLoader::SetValue(const std::string& key, const nlohmann::json& value) {
    jsonData_[key] = value;
}

void JsonLoader::RemoveValue(const std::string& key) {
    jsonData_.erase(key);
}

void JsonLoader::Clear() {
    jsonData_.clear();
}

bool JsonLoader::GetName(const std::string& filePath, const std::string& targetName) const {
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }
    nlohmann::json jsonData;
    try {
        file >> jsonData;
    }
    catch (const std::exception&) {
        return false;
    }
    if (!jsonData.contains("objects") || !jsonData["objects"].is_array()) {
        return false;
    }
    for (const auto& obj : jsonData["objects"]) {
        if (obj.contains("name") && obj["name"] == targetName) {
            return true;
        }
    }
    return false;
}

Vector3 JsonLoader::GetWorldTransform(const std::string& filePath, const std::string& targetName) const {
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    nlohmann::json jsonData;
    try {
        file >> jsonData;
    }
    catch (const std::exception&) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    if (!jsonData.contains("objects") || !jsonData["objects"].is_array()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    for (const auto& obj : jsonData["objects"]) {
        if (obj.contains("name") && obj["name"] == targetName) {
            if (obj.contains("transform") && obj["transform"].contains("translation")) {
                std::vector<float> position = obj["transform"]["translation"];
                if (position.size() == 3) {
                    return Vector3(position[0], position[1], position[2]);
                }
            }
        }
    }
    return Vector3(0.0f, 0.0f, 0.0f);
}

Vector3 JsonLoader::GetWorldTransformRandom(const std::string& filePath, const std::string& targetName) const
{
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    nlohmann::json jsonData;
    try {
        file >> jsonData;
    }
    catch (const std::exception&) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    if (!jsonData.contains("objects") || !jsonData["objects"].is_array()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    std::vector<Vector3> positions;
    for (const auto& obj : jsonData["objects"]) {
        if (obj.contains("name") && obj["name"].get<std::string>().find(targetName) != std::string::npos) {
            if (obj.contains("transform") && obj["transform"].contains("translation")) {
                std::vector<float> pos = obj["transform"]["translation"];
                if (pos.size() == 3) {
                    positions.emplace_back(pos[0], pos[1], pos[2]);
                }
            }
        }
    }
    if (positions.empty()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, positions.size() - 1);
    return positions[dist(gen)];
}
