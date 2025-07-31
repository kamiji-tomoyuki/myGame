#pragma once
#include "externals/nlohmann/json.hpp"
#include "map"
#include "string"
#include "variant"
#include "unordered_map"
#include "vector"

#include "Vector2.h"
#include "Vector3.h"

// json
class GlobalVariables {
public:
	/// <summary>
	/// 毎フレーム処理
	/// </summary>
	void Update();

	/// <summary>
	/// インスタンスの取得
	/// </summary>
	/// <returns></returns>
	static GlobalVariables* GetInstance();

	/// <summary>
	/// グループの作成
	/// </summary>
	/// <param name="groupName"></param>
	void CreateGroup(const std::string& groupName);

	/// <summary>
	/// ファイルに書き出し
	/// </summary>
	/// <param name="groupname"></param>
	void SaveFile(const std::string& groupName);

	/// <summary>
	/// ディレクトリの全ファイル読み込み
	/// </summary>
	void LoadFiles();

	/// <summary>
	/// ファイルから読み込む
	/// </summary>
	/// <param name="groupName"></param>
	void LoadFile(const std::string& groupName);

	/// <summary>
	/// 項目の追加(int)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void AddItem(const std::string& groupName, const std::string& key, int32_t value);

	/// <summary>
	/// 項目の追加(float)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void AddItem(const std::string& groupName, const std::string& key, float value);

	/// <summary>
	/// 項目の追加(Vector2)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void AddItem(const std::string& groupName, const std::string& key, const Vector2& value);

	/// <summary>
	/// 項目の追加(Vector3)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void AddItem(const std::string& groupName, const std::string& key, const Vector3& value);

	/// <summary>
	/// 項目の追加(bool)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void AddItem(const std::string& groupName, const std::string& key, const bool& value);

	/// <summary>
	/// 値のセット(int)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void SetValue(const std::string& groupName, const std::string& key, int32_t value);

	/// <summary>
	/// 値のセット(float)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void SetValue(const std::string& groupName, const std::string& key, float value);

	/// <summary>
	/// 値のセット(Vector2)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void SetValue(const std::string& groupName, const std::string& key, const Vector2& value);

	/// <summary>
	/// 値のセット(Vector3)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void SetValue(const std::string& groupName, const std::string& key, const Vector3& value);

	/// <summary>
	/// 値のセット(bool)
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	void SetValue(const std::string& groupName, const std::string& key, const bool& value);

	int32_t GetIntValue(const std::string& groupName, const std::string& key) const;
	float GetFloatValue(const std::string& groupName, const std::string& key) const;
	Vector2 GetVector2Value(const std::string& groupName, const std::string& key) const;
	Vector3 GetVector3Value(const std::string& groupName, const std::string& key) const;
	bool GetBoolValue(const std::string& groupName, const std::string& key) const;

	bool GroupExists(const std::string& groupName) const;

	// 新機能：値の制限設定
	void SetIntRange(const std::string& groupName, const std::string& key, int32_t min, int32_t max);
	void SetFloatRange(const std::string& groupName, const std::string& key, float min, float max);
	void SetFloatSpeed(const std::string& groupName, const std::string& key, float speed);

	// 新機能：デフォルト値の保存・復元
	void SaveDefaultValues(const std::string& groupName);
	void ResetToDefault(const std::string& groupName);
	void ResetToDefault(const std::string& groupName, const std::string& key);

private:
	// 項目の制限設定
	struct ItemConstraints {
		// int用
		int32_t intMin = INT32_MIN;
		int32_t intMax = INT32_MAX;

		// float用
		float floatMin = -FLT_MAX;
		float floatMax = FLT_MAX;
		float floatSpeed = 0.1f;
	};

	// 項目
	struct Item {
		// 項目の値
		std::variant<int32_t, float, Vector2, Vector3, bool> value;
		// 制限設定
		ItemConstraints constraints;
	};

	// グループ
	struct Group {
		std::map<std::string, Item> items;
		bool collapsed = false; // 折りたたみ状態
	};

	// 全データ
	std::map<std::string, Group> datas_;

	// デフォルト値の保存
	std::map<std::string, Group> defaultValues_;

	using json = nlohmann::json;

	// グローバル変数の保存先ファイルパス
	const std::string kDirectoryPath = "resources/jsons/GlobalVariables/";

	// UI用の検索・ソート機能
	std::string searchFilter_;
	bool sortAlphabetically_ = false;

	// UI用のヘルパー関数
	bool PassesFilter(const std::string& itemName) const;
	std::vector<std::pair<std::string, Item&>> GetSortedItems(Group& group);
	void RenderItemControls(const std::string& groupName, const std::string& itemName, Item& item);

	GlobalVariables() = default;                                  // コンストラクタ
	~GlobalVariables() = default;                                 // デストラクタ
	GlobalVariables(const GlobalVariables&) = default;            // コピーコンストラクタ
	GlobalVariables& operator=(const GlobalVariables&) = default; // コピー代入演算子
};