#include "Windows.h"
#include "fstream"
#include "GlobalVariables.h"
#include "algorithm"
#include "cctype"

#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG

void GlobalVariables::Update() {
#ifdef _DEBUG
	if (!ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}

	// メニューバー
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("オプション")) {
			ImGui::MenuItem("アルファベット順にソート", nullptr, &sortAlphabetically_);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	// 検索フィルター
	std::vector<char> buffer(256); // 最大文字数に応じて調整

	// strncpy_s を使用してセキュリティを向上
	if (!searchFilter_.empty()) {
		strncpy_s(buffer.data(), buffer.size(), searchFilter_.c_str(), _TRUNCATE);
	}
	else {
		buffer[0] = '\0';
	}

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
	if (ImGui::InputTextWithHint("##SearchFilter", "グループを検索...", buffer.data(), buffer.size())) {
		searchFilter_ = std::string(buffer.data());
	}
	ImGui::SameLine();
	if (ImGui::Button("クリア")) {
		searchFilter_.clear();
	}

	ImGui::Separator();

	// タブバーの開始
	if (ImGui::BeginTabBar("GlobalVariablesTabBar")) {
		// 各グループについてタブを作成
		for (auto& [groupName, group] : datas_) {
			// グループ名が検索フィルターにマッチしない場合はスキップ
			if (!PassesFilter(groupName)) {
				continue;
			}

			// タブの開始
			if (ImGui::BeginTabItem(groupName.c_str())) {
				// 検索結果の項目数を表示
				auto sortedItems = GetSortedItems(group);

				ImGui::Text("Items: %d", (int)group.items.size());

				// グループ制御ボタン
				if (ImGui::Button("Save")) {
					SaveFile(groupName);
					std::string message = std::format("{}.json saved.", groupName);
					MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
				}
				ImGui::SameLine();
				if (ImGui::Button("デフォルトとして保存")) {
					SaveDefaultValues(groupName);
				}
				ImGui::SameLine();
				if (ImGui::Button("デフォルトにリセット")) {
					ResetToDefault(groupName);
				}

				ImGui::Separator();

				// スクロール可能な領域
				if (ImGui::BeginChild("ItemsScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
					// 各項目について
					for (const auto& [itemName, item] : sortedItems) {
						// アイテムをレンダリング
						RenderItemControls(groupName, itemName, const_cast<Item&>(item));
					}
				}
				ImGui::EndChild();

				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
#endif // _DEBUG
}

bool GlobalVariables::PassesFilter(const std::string& itemName) const {
	if (searchFilter_.empty()) {
		return true;
	}

	std::string lowerItemName = itemName;
	std::string lowerFilter = searchFilter_;

	std::transform(lowerItemName.begin(), lowerItemName.end(), lowerItemName.begin(), ::tolower);
	std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

	return lowerItemName.find(lowerFilter) != std::string::npos;
}

std::vector<std::pair<std::string, GlobalVariables::Item&>> GlobalVariables::GetSortedItems(Group& group) {
	std::vector<std::pair<std::string, Item&>> items;

	for (auto& [itemName, item] : group.items) {
		items.emplace_back(itemName, item);
	}

	if (sortAlphabetically_) {
		std::sort(items.begin(), items.end(),
			[](const auto& a, const auto& b) {
				return a.first < b.first;
			});
	}

	return items;
}

void GlobalVariables::RenderItemControls(const std::string& groupName, const std::string& itemName, Item& item) {
	ImGui::PushID(itemName.c_str());

	// 項目名の表示（右クリックメニュー付き）
	if (ImGui::BeginPopupContextItem(("ItemContext" + itemName).c_str())) {
		if (ImGui::MenuItem("デフォルトにリセット")) {
			ResetToDefault(groupName, itemName);
		}
		ImGui::EndPopup();
	}

	// 型に応じた制御の表示
	if (std::holds_alternative<int32_t>(item.value)) {
		int32_t* ptr = std::get_if<int32_t>(&item.value);
		if (item.constraints.intMin != INT32_MIN || item.constraints.intMax != INT32_MAX) {
			ImGui::SliderInt(itemName.c_str(), ptr, item.constraints.intMin, item.constraints.intMax);
		}
		else {
			ImGui::DragInt(itemName.c_str(), ptr, 1);
		}
	}
	else if (std::holds_alternative<float>(item.value)) {
		float* ptr = std::get_if<float>(&item.value);
		if (item.constraints.floatMin != -FLT_MAX || item.constraints.floatMax != FLT_MAX) {
			ImGui::SliderFloat(itemName.c_str(), ptr, item.constraints.floatMin, item.constraints.floatMax);
		}
		else {
			ImGui::DragFloat(itemName.c_str(), ptr, item.constraints.floatSpeed);
		}
	}
	else if (std::holds_alternative<Vector2>(item.value)) {
		Vector2* ptr = std::get_if<Vector2>(&item.value);
		ImGui::DragFloat2(itemName.c_str(), reinterpret_cast<float*>(ptr), item.constraints.floatSpeed);
	}
	else if (std::holds_alternative<Vector3>(item.value)) {
		Vector3* ptr = std::get_if<Vector3>(&item.value);
		ImGui::DragFloat3(itemName.c_str(), reinterpret_cast<float*>(ptr), item.constraints.floatSpeed);
	}
	else if (std::holds_alternative<bool>(item.value)) {
		bool* ptr = std::get_if<bool>(&item.value);
		ImGui::Checkbox(itemName.c_str(), ptr);
	}

	ImGui::PopID();
}

GlobalVariables* GlobalVariables::GetInstance() {
	static GlobalVariables instance;
	return &instance;
}

void GlobalVariables::CreateGroup(const std::string& groupName) {
	datas_[groupName];
}

void GlobalVariables::SaveFile(const std::string& groupName) {
	auto itGroup = datas_.find(groupName);
	assert(itGroup != datas_.end());

	json root = json::object();
	root[groupName] = json::object();

	for (const auto& [itemName, item] : itGroup->second.items) {
		if (std::holds_alternative<int32_t>(item.value)) {
			root[groupName][itemName] = std::get<int32_t>(item.value);
		}
		else if (std::holds_alternative<float>(item.value)) {
			root[groupName][itemName] = std::get<float>(item.value);
		}
		else if (std::holds_alternative<Vector2>(item.value)) {
			Vector2 value = std::get<Vector2>(item.value);
			root[groupName][itemName] = json::array({ value.x, value.y });
		}
		else if (std::holds_alternative<Vector3>(item.value)) {
			Vector3 value = std::get<Vector3>(item.value);
			root[groupName][itemName] = json::array({ value.x, value.y, value.z });
		}
		else if (std::holds_alternative<bool>(item.value)) {
			root[groupName][itemName] = std::get<bool>(item.value);
		}
	}

	std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(kDirectoryPath)) {
		std::filesystem::create_directory(kDirectoryPath);
	}

	std::string filePath = kDirectoryPath + groupName + ".json";
	std::ofstream ofs(filePath);

	if (ofs.fail()) {
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		assert(0);
		return;
	}

	ofs << std::setw(4) << root << std::endl;
	ofs.close();
}

void GlobalVariables::LoadFiles() {
	if (!std::filesystem::exists(kDirectoryPath)) {
		return;
	}

	std::filesystem::directory_iterator dir_it(kDirectoryPath);
	for (const std::filesystem::directory_entry& entry : dir_it) {
		const std::filesystem::path& filePath = entry.path();
		std::string extension = filePath.extension().string();

		if (extension.compare(".json") != 0) {
			continue;
		}

		LoadFile(filePath.stem().string());
	}
}

void GlobalVariables::LoadFile(const std::string& groupName) {
	std::string filePath = kDirectoryPath + groupName + ".json";
	std::ifstream ifs(filePath);

	if (ifs.fail()) {
		std::string message = "Failed open data file for read.";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		assert(0);
		return;
	}

	json root;
	ifs >> root;
	ifs.close();

	json::iterator itGroup = root.find(groupName);
	assert(itGroup != root.end());

	for (json::iterator itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem) {
		const std::string& itemName = itItem.key();

		if (itItem->is_number_integer()) {
			int32_t value = itItem->get<int32_t>();
			SetValue(groupName, itemName, value);
		}
		else if (itItem->is_number_float()) {
			double value = itItem->get<double>();
			SetValue(groupName, itemName, static_cast<float>(value));
		}
		else if (itItem->is_array() && itItem->size() == 2) {
			Vector2 value = { itItem->at(0), itItem->at(1) };
			SetValue(groupName, itemName, value);
		}
		else if (itItem->is_array() && itItem->size() == 3) {
			Vector3 value = { itItem->at(0), itItem->at(1), itItem->at(2) };
			SetValue(groupName, itemName, value);
		}
		else if (itItem->is_boolean()) {
			bool value = itItem->get<bool>();
			SetValue(groupName, itemName, value);
		}
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, int32_t value) {
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, float value) {
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const Vector2& value) {
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const Vector3& value) {
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const bool& value) {
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, int32_t value) {
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, float value) {
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const Vector2& value) {
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const Vector3& value) {
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const bool& value) {
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

// 制限設定の新機能
void GlobalVariables::SetIntRange(const std::string& groupName, const std::string& key, int32_t min, int32_t max) {
	Group& group = datas_[groupName];
	if (group.items.find(key) != group.items.end()) {
		group.items[key].constraints.intMin = min;
		group.items[key].constraints.intMax = max;
	}
}

void GlobalVariables::SetFloatRange(const std::string& groupName, const std::string& key, float min, float max) {
	Group& group = datas_[groupName];
	if (group.items.find(key) != group.items.end()) {
		group.items[key].constraints.floatMin = min;
		group.items[key].constraints.floatMax = max;
	}
}

void GlobalVariables::SetFloatSpeed(const std::string& groupName, const std::string& key, float speed) {
	Group& group = datas_[groupName];
	if (group.items.find(key) != group.items.end()) {
		group.items[key].constraints.floatSpeed = speed;
	}
}

// デフォルト値の保存・復元
void GlobalVariables::SaveDefaultValues(const std::string& groupName) {
	if (datas_.find(groupName) != datas_.end()) {
		defaultValues_[groupName] = datas_[groupName];
	}
}

void GlobalVariables::ResetToDefault(const std::string& groupName) {
	if (defaultValues_.find(groupName) != defaultValues_.end()) {
		datas_[groupName] = defaultValues_[groupName];
	}
}

void GlobalVariables::ResetToDefault(const std::string& groupName, const std::string& key) {
	if (defaultValues_.find(groupName) != defaultValues_.end()) {
		const auto& defaultGroup = defaultValues_[groupName];
		if (defaultGroup.items.find(key) != defaultGroup.items.end()) {
			datas_[groupName].items[key] = defaultGroup.items.at(key);
		}
	}
}

// 既存のget関数群（変更なし）
int32_t GlobalVariables::GetIntValue(const std::string& groupName, const std::string& key) const {
	if (datas_.find(groupName) == datas_.end()) {
		return 0;
	}
	const Group& group = datas_.at(groupName);
	assert(group.items.find(key) != group.items.end());
	const Item& item = group.items.at(key);
	assert(std::holds_alternative<int32_t>(item.value));
	return std::get<int32_t>(item.value);
}

float GlobalVariables::GetFloatValue(const std::string& groupName, const std::string& key) const {
	if (datas_.find(groupName) == datas_.end()) {
		return 0.0f;
	}
	const Group& group = datas_.at(groupName);
	assert(group.items.find(key) != group.items.end());
	const Item& item = group.items.at(key);
	assert(std::holds_alternative<float>(item.value));
	return std::get<float>(item.value);
}

Vector2 GlobalVariables::GetVector2Value(const std::string& groupName, const std::string& key) const {
	if (datas_.find(groupName) == datas_.end()) {
		return Vector2(0);
	}
	const Group& group = datas_.at(groupName);
	assert(group.items.find(key) != group.items.end());
	const Item& item = group.items.at(key);
	assert(std::holds_alternative<Vector2>(item.value));
	return std::get<Vector2>(item.value);
}

Vector3 GlobalVariables::GetVector3Value(const std::string& groupName, const std::string& key) const {
	if (datas_.find(groupName) == datas_.end()) {
		return Vector3(0);
	}
	const Group& group = datas_.at(groupName);
	assert(group.items.find(key) != group.items.end());
	const Item& item = group.items.at(key);
	assert(std::holds_alternative<Vector3>(item.value));
	return std::get<Vector3>(item.value);
}

bool GlobalVariables::GetBoolValue(const std::string& groupName, const std::string& key) const {
	if (datas_.find(groupName) == datas_.end()) {
		return false;
	}
	const Group& group = datas_.at(groupName);
	assert(group.items.find(key) != group.items.end());
	const Item& item = group.items.at(key);
	assert(std::holds_alternative<bool>(item.value));
	return std::get<bool>(item.value);
}

bool GlobalVariables::GroupExists(const std::string& groupName) const {
	return datas_.find(groupName) != datas_.end();
}