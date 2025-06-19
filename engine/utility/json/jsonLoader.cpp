#include "JsonLoader.h"
#include <fstream>
#include <iostream>
#include <numbers>
#include <ModelManager.h>

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

void JsonLoader::LoadSceneFile(const std::string& filePath)
{
	// --- ファイル読み込み ---
	const std::string fullPath = "resources/jsons/scene/" + filePath;

	std::ifstream file(fullPath);

	if (file.fail()) {
		assert(0);
	}

	// --- ファイルチェック ---
	nlohmann::json deserialized;

	file >> deserialized;

	assert(deserialized.is_object());
	assert(deserialized.contains("name"));
	assert(deserialized["name"].is_string());

	// "name"を文字列として取得
	std::string name = deserialized["name"].get<std::string>();

	assert(name.compare("scene") == 0);

	// levelData_を一度だけ初期化
	levelData_ = std::make_unique<LevelData>();

	// --- オブジェクトの走査 ---
	for (nlohmann::json& object : deserialized["objects"]) {
		assert(object.contains("type"));

		// 種別を取得
		std::string type = object["type"].get<std::string>();

		// 種類ごとの処理
		// MESH
		if (type.compare("MESH") == 0) {
			levelData_->objects.push_back(ObjectData{});
			ObjectData& objectData = levelData_->objects.back();

			if (object.contains("file_name")) {
				objectData.fileName = object["file_name"];
			}

			// transform
			nlohmann::json& transform = object["transform"];

			objectData.translation.x = (float)transform["translation"][0];
			objectData.translation.y = (float)transform["translation"][2];
			objectData.translation.z = (float)transform["translation"][1];

			const float degToRad = (float)std::numbers::pi / 180.0f;
			objectData.rotation.x = -(float)transform["rotation"][0] * degToRad;
			objectData.rotation.y = -(float)transform["rotation"][2] * degToRad;
			objectData.rotation.z = -(float)transform["rotation"][1] * degToRad;

			objectData.scale.x = (float)transform["scaling"][0];
			objectData.scale.y = (float)transform["scaling"][2];
			objectData.scale.z = (float)transform["scaling"][1];

			// 再帰処理
			if (object.contains("children")) {
				for (const auto& child : object["children"]) {
					ObjectData childData;
					Recursive(child, childData);
					objectData.children.push_back(std::move(childData));
				}
			}
		}
	}

	SetScene();
}

void JsonLoader::SetScene()
{
	for (auto& objectData : levelData_->objects) {
		// ファイル名から登録済みモデルを検索
		Model* model = nullptr;
		decltype(models_)::iterator it = models_.find(objectData.fileName);
		
		// モデルを指定して3Dオブジェクトを生成
		std::unique_ptr<Object3d> newObject = std::make_unique<Object3d>();
		newObject->Initialize("scene/" + objectData.fileName);

		// ワールド座標生成
		std::unique_ptr<WorldTransform> worldTransform = std::make_unique<WorldTransform>();
		worldTransform->Initialize();

		// 座標
		worldTransform.get()->translation_ = objectData.translation;
		// 回転角
		worldTransform.get()->rotation_ = objectData.rotation;
		// 拡縮
		worldTransform.get()->scale_ = objectData.scale;

		// 配列に登録
		objects_.push_back(std::move(newObject));
		worldTransforms_.push_back(std::move(worldTransform));
	}
}

void JsonLoader::UpdateScene()
{
	for (size_t i = 0; i < worldTransforms_.size(); ++i) {
		worldTransforms_[i]->UpdateMatrix();
	}
}

void JsonLoader::DrawScene(const ViewProjection& viewProjection)
{
	for (size_t i = 0; i < objects_.size(); ++i) {
		objects_[i]->Draw(*worldTransforms_[i], viewProjection);
	}
}

bool JsonLoader::GetName(const std::string& filePath, const std::string& targetName) const {
	const std::string fullPath = "resources/jsons/" + filePath;

	std::ifstream file(fullPath);

	file.open(fullPath);
	if (file.fail()) {
		assert(0);
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

void JsonLoader::Recursive(const nlohmann::json& jsonObject, ObjectData& parent)
{
	if (jsonObject.contains("file_name")) {
		parent.fileName = jsonObject["file_name"];
	}

	if (jsonObject.contains("transform")) {
		auto& t = jsonObject["transform"];

		if (t.contains("translation") && t["translation"].is_array() && t["translation"].size() == 3) {
			parent.translation.x = t["translation"][0];
			parent.translation.y = t["translation"][1];
			parent.translation.z = t["translation"][2];
		}

		if (t.contains("rotation") && t["rotation"].is_array() && t["rotation"].size() == 3) {
			const float degToRad = (float)std::numbers::pi / 180.0f;
			parent.rotation.x = t["rotation"][0] * degToRad;
			parent.rotation.y = t["rotation"][1] * degToRad;
			parent.rotation.z = t["rotation"][2] * degToRad;
		}

		if (t.contains("scaling") && t["scaling"].is_array() && t["scaling"].size() == 3) {
			parent.scale.x = t["scaling"][0];
			parent.scale.y = t["scaling"][1];
			parent.scale.z = t["scaling"][2];
		}
	}

	// 再帰処理
	if (jsonObject.contains("children") && jsonObject["children"].is_array()) {
		for (const auto& childJson : jsonObject["children"]) {
			ObjectData child;
			Recursive(childJson, child);
			parent.children.push_back(std::move(child));
		}
	}
}
