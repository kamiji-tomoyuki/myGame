#pragma once
#include <random>
#include <string>
#include <vector>
#include <json.hpp>

#include <Object3d.h>
#include <ViewProjection.h>
#include <WorldTransform.h>

#include <Vector3.h>

// JSONローダー
class JsonLoader {
private:

	struct ObjectData {
		std::string fileName;

		Vector3 translation;
		Vector3 rotation;
		Vector3 scale;

		std::vector<ObjectData> children;
	};

	struct LevelData
	{
		std::vector<ObjectData> objects;
	};

public:

	/// --- 汎用関数 ---

	/// <summary>
	/// JSONファイルを読み込む
	/// </summary>
	/// <param name="filePath">: 読み込むJSONファイルのパス</param>
	/// <returns>読み込み成功ならtrue</returns>
	bool LoadFromFile(const std::string& filePath);

	/// <summary>
	/// JSONをファイルに保存する
	/// </summary>
	/// <param name="filePath">: 保存するJSONファイルのパス</param>
	/// <returns>保存成功ならtrue</returns>
	bool SaveToFile(const std::string& filePath) const;

	/// <summary>
	/// 指定キーの値を取得
	/// </summary>
	/// <param name="key">: 取得するキー</param>
	/// <returns>取得したJSON値</returns>
	std::optional<nlohmann::json> GetValue(const std::string& key) const;

	/// <summary>
	/// 指定キーに値を設定
	/// </summary>
	/// <param name="key">: 設定するキー</param>
	/// <param name="value">: 設定する値</param>
	void SetValue(const std::string& key, const nlohmann::json& value);

	/// <summary>
	/// 指定キーのデータを削除
	/// </summary>
	/// <param name="key">: 削除するキー</param>
	void RemoveValue(const std::string& key);

	/// <summary>
	/// JSONデータをクリア
	/// </summary>
	void Clear();

public:

	/// --- Scene関数 ---

	/// <summary>
	/// SCENE出力JSONファイルを読み込む
	/// </summary>
	/// <param name="filePath">: 読み込むJSONファイルのパス</param>
	/// <returns>読み込み成功ならtrue</returns>
	void LoadSceneFile(const std::string& filePath);

	/// <summary>
	/// 配置
	/// </summary>
	void SetScene();

	/// <summary>
	/// SCENE出力JSONファイルを更新
	/// </summary>
	void UpdateScene();

	/// <summary>
	/// SCENE出力JSONファイルを描画
	/// </summary>
	void DrawScene(const ViewProjection& viewProjection);

	/// <summary>
	/// 指定した名前のオブジェクトが存在するかを確認
	/// </summary>
	/// <param name="filePath">: JSONファイルのパス</param>
	/// <param name="targetName">: 検索するオブジェクト名</param>
	/// <returns>オブジェクトが存在すればtrue</returns>
	bool GetName(const std::string& filePath, const std::string& targetName) const;

	/// <summary>
	/// 指定したオブジェクトのワールド座標を取得
	/// </summary>
	/// <param name="filePath">: JSONファイルのパス</param>
	/// <param name="targetName">: 取得するオブジェクトの名前</param>
	/// <returns>ワールド座標 (x, y, z) のリスト</returns>
	Vector3 GetWorldTransform(const std::string& filePath, const std::string& targetName) const;

	/// <summary>
	/// 指定したオブジェクト名からランダムにワールド座標を取得
	/// </summary>
	/// <param name="filePath">: JSONファイルのパス</param>
	/// <param name="targetName">: 取得するオブジェクトの名前</param>
	/// <returns>ワールド座標 (x, y, z) のリスト</returns>
	Vector3 GetWorldTransformRandom(const std::string& filePath, const std::string& targetName) const;

private:

	/// <summary>
    /// 再帰処理
    /// </summary>
	void Recursive(const nlohmann::json& jsonObject, ObjectData& parent);



private:

	nlohmann::json jsonData_;

	std::unique_ptr<LevelData> levelData_;

	std::vector<std::unique_ptr<Object3d>> objects_;
	std::map<std::string, Model*> models_;
	std::vector<std::unique_ptr<WorldTransform>> worldTransforms_;
};