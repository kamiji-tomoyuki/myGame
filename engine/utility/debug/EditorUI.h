#pragma once
#ifdef _DEBUG
#include <memory>
#include <string>
#include <vector>
#include <map>

/// <summary>
/// Unityライクなエディタ画面（Debug実行時専用）
/// ・画面上端のメインメニューバー（シーン / 表示 / 追加）
/// ・全画面ドックスペース（中央はゲーム画面をパススルー、左右にパネルをドッキング）
/// </summary>
namespace Engine {

class ViewProjection;
struct EditorObject; // 実体は EditorUI.cpp 内で定義

class EditorUI {
public:
	~EditorUI();

	static EditorUI* GetInstance();

	/// <summary>初期化</summary>
	void Initialize();
	/// <summary>終了（生成したエディタオブジェクトの解放）</summary>
	void Finalize();

	// ---- フレーム処理（ImGui::NewFrame後～Render前に呼ぶ） ----
	/// <summary>ドックスペース＋メインメニューバーの構築を開始</summary>
	void BeginDockSpace();
	/// <summary>ドックスペースの構築を終了</summary>
	void EndDockSpace();

	// ---- シーンメニュー用 ----
	/// <summary>シーンメニューに切り替え候補を登録（id=生成キー, label=表示名）</summary>
	void RegisterScene(const std::string& id, const std::string& label);

	// ---- 表示メニュー（ImGuiパネルの表示トグル）用 ----
	/// <summary>
	/// パネルを表示レジストリに登録し、現在の表示状態を返す。
	/// 使い方: if (EditorUI::GetInstance()->PanelVisible("Player","Character")) { ImGui::Begin(...); ... ImGui::End(); }
	/// </summary>
	bool PanelVisible(const char* name, const char* category = "General");

	// ---- 追加したエディタオブジェクトの更新・描画（ゲーム描画パスから呼ぶ） ----
	/// <summary>追加オブジェクトを描画（内部で各Commonの描画設定も行う）</summary>
	void DrawEditorObjects(const ViewProjection& vp);

	/// <summary>エディタが有効か（メニュー等が使えるか）</summary>
	bool IsEnabled() const { return enabled_; }

private:
	EditorUI() = default;
	EditorUI(const EditorUI&) = delete;
	EditorUI& operator=(const EditorUI&) = delete;

	// メニュー描画
	void DrawSceneMenu();
	void DrawDisplayMenu();
	void DrawAddMenu();

	// パネル描画
	void DrawGameWindow();  // ゲーム画面（実行画面をImGuiウィンドウ内に表示）
	void DrawLeftPanel();   // シーン設定（左）
	void DrawRightPanel();  // ヒエラルキー＋インスペクター（右）
	void DrawAddDialog();   // 追加ウィンドウ

	// 追加ダイアログからエディタオブジェクトを生成
	void CreateEditorObjectFromDialog();
	// 追加内容をJSONへ保存
	void SaveObjectRecord(const EditorObject& obj);
	// リソース(モデル/画像)をスキャンして選択候補を用意
	void ScanResources();

	// 初回のみドックレイアウトを構築
	void BuildDefaultLayout(unsigned int dockspaceID);

private:
	static std::unique_ptr<EditorUI> instance_;

	bool enabled_ = true;
	bool layoutBuilt_ = false;

	// シーン一覧（id, label）
	std::vector<std::pair<std::string, std::string>> scenes_;

	// 表示レジストリ
	struct PanelEntry {
		std::string category;
		bool visible = true;
	};
	std::map<std::string, PanelEntry> panels_; // key = パネル名
	std::vector<std::string> panelOrder_;      // 登録順の維持

	// 追加ダイアログ状態
	bool showAddDialog_ = false;
	int addType_ = 0;              // 0:3D 1:Sprite 2:Particle 3:PostEffect
	char addName_[128] = "";
	char addResource_[256] = "";
	char addSavePath_[256] = "resources/jsons/editor";

	// リソース選択候補（スキャン結果）
	bool resourceScanned_ = false;
	std::vector<std::string> modelFiles_; // resources/models からの相対パス（.obj/.gltf）
	std::vector<std::string> imageFiles_; // resources/images 内のファイル名（.png）

	// 選択中のオブジェクト（インスペクター用）
	int selectedObject_ = -1;

	// ゲーム画面ウィンドウ内の画像スクリーン矩形（ギズモ用）
	float gameImageMin_[2] = { 0.0f, 0.0f };
	float gameImageMax_[2] = { 0.0f, 0.0f };

	// エディタが所有するオブジェクト
	std::vector<std::unique_ptr<EditorObject>> objects_;
	int object3dCount_ = 0;
	int spriteCount_ = 0;
	int particleCount_ = 0;
	int postEffectCount_ = 0;
};

} // namespace Engine
#endif // _DEBUG
