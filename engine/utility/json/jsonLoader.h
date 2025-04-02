#pragma once
#include <random>
#include <string>
#include <vector>
#include <json.hpp>

#include <Vector3.h>

// JSONローダー
class JsonLoader {
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

    /// --- レベルデザイン関数 ---

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

    nlohmann::json jsonData_;

};