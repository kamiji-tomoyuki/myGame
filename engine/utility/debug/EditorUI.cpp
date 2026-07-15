#include "EditorUI.h"
#ifdef _DEBUG
#include "imgui.h"
#include "imgui_internal.h" // DockBuilder API

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <json.hpp>

#include "SceneManager.h"
#include "DirectXCommon.h"
#include "WinApp.h"
#include "engine/Frame/Frame.h"

#include "Object3d.h"
#include "Object3dCommon.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "ParticleEmitter.h"
#include "ParticleCommon.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "SrvManager.h"
#include "PostEffect.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "Vector3.h"
#include "Vector2.h"

namespace Engine {

// エディタが所有するオブジェクトの実体
struct EditorObject {
	int type = 0;                 // 0:3D 1:Sprite 2:Particle 3:PostEffect
	std::string name;
	std::string resource;         // モデル/テクスチャ等のパス
	std::string savePath;
	bool visible = true;

	// 3D
	std::unique_ptr<Object3d> obj3d;
	WorldTransform transform;
	Vector3 position{ 0.0f, 0.0f, 0.0f };
	Vector3 rotation{ 0.0f, 0.0f, 0.0f };
	Vector3 scale{ 1.0f, 1.0f, 1.0f };

	// Sprite
	std::unique_ptr<Sprite> sprite;
	Vector2 spritePos{ 640.0f, 360.0f };
	float spriteRot = 0.0f;
	Vector2 spriteSize{ 128.0f, 128.0f };

	// Particle
	std::unique_ptr<ParticleEmitter> particle;
};

std::unique_ptr<EditorUI> EditorUI::instance_ = nullptr;

EditorUI* EditorUI::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = std::unique_ptr<EditorUI>(new EditorUI());
	}
	return instance_.get();
}

EditorUI::~EditorUI() = default;

void EditorUI::Initialize() {
	enabled_ = true;
	layoutBuilt_ = false;
}

void EditorUI::Finalize() {
	objects_.clear();
	instance_.reset();
}

void EditorUI::RegisterScene(const std::string& id, const std::string& label) {
	for (const auto& s : scenes_) {
		if (s.first == id) { return; }
	}
	scenes_.emplace_back(id, label);
}

bool EditorUI::PanelVisible(const char* name, const char* category) {
	auto it = panels_.find(name);
	if (it == panels_.end()) {
		PanelEntry entry;
		entry.category = category;
		entry.visible = true;
		panels_.emplace(name, entry);
		panelOrder_.emplace_back(name);
		return true;
	}
	return it->second.visible;
}

// =============================================================
// ドックスペース + メインメニューバー
// =============================================================
void EditorUI::BeginDockSpace() {
	if (!enabled_) { return; }

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground; // 中央はゲーム画面を透過表示

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("##EditorDockHost", nullptr, hostFlags);
	ImGui::PopStyleVar(3);

	// ドックスペース本体（中央ノードはパススルー）
	ImGuiID dockspaceID = ImGui::GetID("EditorDockSpace");

	// DockSpace() を呼ぶとノードが生成されるため、既定レイアウト構築は必ずその前に行う。
	// 保存済みレイアウト（imgui.ini）があればノードが復元済みなので構築しない。
	if (!layoutBuilt_) {
		if (ImGui::DockBuilderGetNode(dockspaceID) == nullptr) {
			BuildDefaultLayout(dockspaceID);
		}
		layoutBuilt_ = true;
	}

	ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// ---- メインメニューバー ----
	if (ImGui::BeginMenuBar()) {
		DrawSceneMenu();
		DrawDisplayMenu();
		DrawAddMenu();
		DrawFpsIndicator(); // 右寄せで常時FPS表示
		ImGui::EndMenuBar();
	}

	ImGui::End(); // ##EditorDockHost

	// ---- ゲーム画面 + 右パネル + パーティクル + 追加ダイアログ ----
	//   （シーン切り替えはメニューバーの「シーン」にあるため左のシーン設定ウィンドウは廃止）
	DrawGameWindow();
	DrawRightPanel();
	ParticleEmitter::DrawParticleWindow(); // 全エミッタを1つの「パーティクル」窓に集約
	DrawAddDialog();
}

// メニューバー右端に FPS を常時表示する
void EditorUI::DrawFpsIndicator() {
	const float fps = Frame::GetFPS();
	const float ms = Frame::DeltaTime() * 1000.0f;
	char buf[64];
	snprintf(buf, sizeof(buf), "FPS %.1f  (%.2f ms)", fps, ms);
	const float textW = ImGui::CalcTextSize(buf).x;
	ImGui::SameLine(ImGui::GetWindowWidth() - textW - 16.0f);
	ImVec4 color = (fps >= 59.0f) ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f)
		: (fps >= 30.0f) ? ImVec4(1.0f, 1.0f, 0.3f, 1.0f)
		: ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
	ImGui::TextColored(color, "%s", buf);
}

void EditorUI::EndDockSpace() {
	// 現状は追加処理なし（対称性のために用意）
}

void EditorUI::BuildDefaultLayout(unsigned int dockspaceID) {
	ImGui::DockBuilderRemoveNode(dockspaceID);
	ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->WorkSize);

	ImGuiID dockMain = dockspaceID;
	ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.22f, nullptr, &dockMain);
	ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.28f, nullptr, &dockMain);

	// 中央ノードにゲーム画面を配置（左右パネルに隠れない）
	ImGui::DockBuilderDockWindow("ゲーム画面", dockMain);
	ImGui::DockBuilderDockWindow("ヒエラルキー", dockRight);
	// 既存のデバッグウィンドウも初期状態でドッキングして画面を整理
	ImGui::DockBuilderDockWindow("Global Variables", dockRight);
	ImGui::DockBuilderDockWindow("OffScreen", dockRight);
	ImGui::DockBuilderDockWindow("パーティクル", dockLeft);
	ImGui::DockBuilderDockWindow("Debug", dockLeft);
	ImGui::DockBuilderDockWindow("GameScene:Debug", dockLeft);
	ImGui::DockBuilderFinish(dockspaceID);
}

// =============================================================
// シーンメニュー
// =============================================================
void EditorUI::DrawSceneMenu() {
	if (!ImGui::BeginMenu("◆ シーン")) { return; }

	SceneManager* sm = SceneManager::GetInstance();
	bool canChange = sm->CanChangeScene();

	if (scenes_.empty()) {
		ImGui::TextDisabled("(登録シーンなし)");
	}
	for (const auto& s : scenes_) {
		std::string label = "● " + s.second;
		if (ImGui::MenuItem(label.c_str(), nullptr, false, canChange)) {
			sm->NextSceneReservation(s.first);
		}
	}
	ImGui::EndMenu();
}

// =============================================================
// 表示メニュー（パネルのトグル）
// =============================================================
void EditorUI::DrawDisplayMenu() {
	if (!ImGui::BeginMenu("◎ 表示")) { return; }

	// 一括切り替え
	if (ImGui::MenuItem("● すべて表示")) {
		for (auto& p : panels_) { p.second.visible = true; }
	}
	if (ImGui::MenuItem("○ すべて非表示")) {
		for (auto& p : panels_) { p.second.visible = false; }
	}
	ImGui::Separator();

	if (panels_.empty()) {
		ImGui::TextDisabled("(登録パネルなし)");
	}

	// カテゴリ収集（登録順を維持）
	std::vector<std::string> categories;
	for (const auto& name : panelOrder_) {
		const std::string& cat = panels_[name].category;
		if (std::find(categories.begin(), categories.end(), cat) == categories.end()) {
			categories.push_back(cat);
		}
	}

	for (const auto& cat : categories) {
		if (!ImGui::BeginMenu(cat.c_str())) { continue; }

		// カテゴリ単位の一括
		if (ImGui::MenuItem("このカテゴリを表示")) {
			for (auto& p : panels_) { if (p.second.category == cat) { p.second.visible = true; } }
		}
		if (ImGui::MenuItem("このカテゴリを非表示")) {
			for (auto& p : panels_) { if (p.second.category == cat) { p.second.visible = false; } }
		}
		ImGui::Separator();

		for (const auto& name : panelOrder_) {
			PanelEntry& entry = panels_[name];
			if (entry.category != cat) { continue; }
			ImGui::MenuItem(name.c_str(), nullptr, &entry.visible);
		}
		ImGui::EndMenu();
	}
	ImGui::EndMenu();
}

// =============================================================
// 追加メニュー
// =============================================================
void EditorUI::DrawAddMenu() {
	if (!ImGui::BeginMenu("＋ 追加")) { return; }

	if (ImGui::MenuItem("■ 3Dオブジェクト")) { addType_ = 0; showAddDialog_ = true; }
	if (ImGui::MenuItem("◆ 2Dスプライト")) { addType_ = 1; showAddDialog_ = true; }
	if (ImGui::MenuItem("★ パーティクル")) { addType_ = 2; showAddDialog_ = true; }
	if (ImGui::MenuItem("◎ ポストエフェクト")) { addType_ = 3; showAddDialog_ = true; }

	ImGui::EndMenu();
}

// =============================================================
// リソースのスキャン（モデル/画像の選択候補を用意）
// =============================================================
void EditorUI::ScanResources() {
	namespace fs = std::filesystem;
	modelFiles_.clear();
	imageFiles_.clear();

	auto toLower = [](std::string s) {
		for (char& c : s) { c = static_cast<char>(::tolower(static_cast<unsigned char>(c))); }
		return s;
	};

	// --- モデル（resources/models 以下の .obj / .gltf、相対パスで保持） ---
	try {
		fs::path root = "resources/models";
		if (fs::exists(root)) {
			for (const auto& e : fs::recursive_directory_iterator(root)) {
				if (!e.is_regular_file()) { continue; }
				std::string ext = toLower(e.path().extension().string());
				if (ext == ".obj" || ext == ".gltf") {
					modelFiles_.push_back(fs::relative(e.path(), root).generic_string());
				}
			}
		}
	}
	catch (...) {}

	// --- 画像（resources/images 以下の .png、ファイル名で保持し重複除去） ---
	try {
		fs::path root = "resources/images";
		if (fs::exists(root)) {
			for (const auto& e : fs::recursive_directory_iterator(root)) {
				if (!e.is_regular_file()) { continue; }
				if (toLower(e.path().extension().string()) != ".png") { continue; }
				std::string name = e.path().filename().string();
				if (std::find(imageFiles_.begin(), imageFiles_.end(), name) == imageFiles_.end()) {
					imageFiles_.push_back(name);
				}
			}
		}
	}
	catch (...) {}

	std::sort(modelFiles_.begin(), modelFiles_.end());
	std::sort(imageFiles_.begin(), imageFiles_.end());
	resourceScanned_ = true;
}

// =============================================================
// 追加ダイアログ
// =============================================================
void EditorUI::DrawAddDialog() {
	if (!showAddDialog_) { return; }

	if (!resourceScanned_) { ScanResources(); }

	ImGui::SetNextWindowSize(ImVec2(560.0f, 560.0f), ImGuiCond_Appearing);
	if (ImGui::Begin("オブジェクトの追加", &showAddDialog_, ImGuiWindowFlags_NoDocking)) {

		const char* types[] = { "■ 3Dオブジェクト", "◆ 2Dスプライト", "★ パーティクル", "◎ ポストエフェクト" };
		ImGui::Combo("種類", &addType_, types, IM_ARRAYSIZE(types));

		ImGui::InputText("名前", addName_, IM_ARRAYSIZE(addName_));

		ImGui::SameLine();
		if (ImGui::SmallButton("再スキャン")) { ScanResources(); }

		ImGui::Text("選択中: %s", addResource_[0] ? addResource_ : "(未選択)");
		ImGui::Separator();

		// リソース選択領域
		ImGui::BeginChild("ResourceSelect", ImVec2(0.0f, 340.0f), true);
		switch (addType_) {
		case 0: // 3Dオブジェクト（モデル一覧）
		case 2: // パーティクル（マテリアル=モデル一覧）
		{
			ImGui::TextDisabled("モデルを選択");
			for (const auto& m : modelFiles_) {
				bool selected = (m == addResource_);
				if (ImGui::Selectable(m.c_str(), selected)) {
					strncpy_s(addResource_, m.c_str(), _TRUNCATE);
				}
			}
			break;
		}
		case 1: // 2Dスプライト（画像サムネイル一覧）
		{
			ImGui::TextDisabled("テクスチャを選択（クリックで確定）");
			TextureManager* tm = TextureManager::GetInstance();
			SrvManager* srv = SrvManager::GetInstance();
			const float thumb = 72.0f;
			float avail = ImGui::GetContentRegionAvail().x;
			int perRow = static_cast<int>(avail / (thumb + 12.0f));
			if (perRow < 1) { perRow = 1; }
			int col = 0;
			for (const auto& img : imageFiles_) {
				tm->LoadTexture(img); // 読み込み（キャッシュ済みなら何もしない）
				uint32_t idx = tm->GetTextureIndexByFilePath(img);
				ImTextureID tex = static_cast<ImTextureID>(srv->GetGPUDescriptorHandle(idx).ptr);

				ImGui::BeginGroup();
				ImGui::PushID(img.c_str());
				bool selected = (img == addResource_);
				if (selected) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 0.6f));
				}
				if (ImGui::ImageButton("thumb", tex, ImVec2(thumb, thumb))) {
					strncpy_s(addResource_, img.c_str(), _TRUNCATE);
				}
				if (selected) { ImGui::PopStyleColor(); }
				// ファイル名（長い場合は省略表示）
				std::string shortName = img.size() > 10 ? img.substr(0, 9) + "…" : img;
				ImGui::TextUnformatted(shortName.c_str());
				ImGui::PopID();
				ImGui::EndGroup();

				if (++col % perRow != 0) { ImGui::SameLine(); }
			}
			break;
		}
		case 3: // ポストエフェクト（種類一覧）
		{
			ImGui::TextDisabled("エフェクトの種類を選択");
			const std::vector<std::string>& fxTypes = GetPostEffectTypeNames();
			for (const auto& t : fxTypes) {
				bool selected = (t == addResource_);
				if (ImGui::Selectable(t.c_str(), selected)) {
					strncpy_s(addResource_, t.c_str(), _TRUNCATE);
				}
			}
			break;
		}
		}
		ImGui::EndChild();

		ImGui::InputText("保存先", addSavePath_, IM_ARRAYSIZE(addSavePath_));

		ImGui::Separator();
		if (ImGui::Button("追加", ImVec2(120.0f, 0.0f))) {
			CreateEditorObjectFromDialog();
			showAddDialog_ = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("キャンセル", ImVec2(120.0f, 0.0f))) {
			showAddDialog_ = false;
		}
	}
	ImGui::End();
}

void EditorUI::CreateEditorObjectFromDialog() {
	auto obj = std::make_unique<EditorObject>();
	obj->type = addType_;
	obj->resource = addResource_;
	obj->savePath = addSavePath_;

	std::string name = addName_;

	switch (addType_) {
	case 0: { // 3Dオブジェクト
		if (name.empty()) { name = "Object3d_" + std::to_string(object3dCount_); }
		if (obj->resource.empty()) { obj->resource = "debug/Cube.obj"; }
		ModelManager::GetInstance()->LoadModel(obj->resource);
		obj->obj3d = std::make_unique<Object3d>();
		obj->obj3d->Initialize(obj->resource);
		obj->transform.Initialize();
		++object3dCount_;
		break;
	}
	case 1: { // 2Dスプライト
		if (name.empty()) { name = "Sprite_" + std::to_string(spriteCount_); }
		if (obj->resource.empty()) { obj->resource = "uvChecker.png"; }
		obj->sprite = std::make_unique<Sprite>();
		obj->sprite->Initialize(obj->resource, obj->spritePos);
		obj->spriteSize = obj->sprite->GetSize(); // テクスチャ実サイズを初期値に
		++spriteCount_;
		break;
	}
	case 2: { // パーティクル
		if (name.empty()) { name = "Particle_" + std::to_string(particleCount_); }
		if (obj->resource.empty()) { obj->resource = "debug/plane.obj"; }
		obj->particle = std::make_unique<ParticleEmitter>();
		obj->particle->Initialize(name, obj->resource);
		obj->particle->SetActive(true);
		++particleCount_;
		break;
	}
	case 3: { // ポストエフェクト（記録のみ）
		if (name.empty()) { name = "PostEffect_" + std::to_string(postEffectCount_); }
		++postEffectCount_;
		break;
	}
	}

	obj->name = name;
	SaveObjectRecord(*obj);
	objects_.push_back(std::move(obj));
	selectedObject_ = static_cast<int>(objects_.size()) - 1;
}

void EditorUI::SaveObjectRecord(const EditorObject& obj) {
	using json = nlohmann::json;
	try {
		std::filesystem::path dir(obj.savePath);
		if (!dir.empty() && !std::filesystem::exists(dir)) {
			std::filesystem::create_directories(dir);
		}
		const char* typeNames[] = { "Object3D", "Sprite", "Particle", "PostEffect" };
		json j;
		j["name"] = obj.name;
		j["type"] = typeNames[obj.type];
		j["resource"] = obj.resource;

		std::filesystem::path file = dir / (obj.name + ".json");
		std::ofstream ofs(file);
		if (ofs) { ofs << j.dump(2); }
	}
	catch (...) {
		// 保存失敗は致命的ではないので無視
	}
}

// =============================================================
// ゲーム画面（実行画面をImGuiウィンドウ内に表示）
// =============================================================
void EditorUI::DrawGameWindow() {
	DirectXCommon* dx = DirectXCommon::GetInstance();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (ImGui::Begin("ゲーム画面", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (avail.x > 1.0f && avail.y > 1.0f) {
			// テクスチャのアスペクト比を維持してレターボックス配置
			const float texAspect =
				static_cast<float>(WinApp::kClientWidth) / static_cast<float>(WinApp::kClientHeight);
			float w = avail.x;
			float h = w / texAspect;
			if (h > avail.y) {
				h = avail.y;
				w = h * texAspect;
			}
			ImVec2 cur = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cur.x + (avail.x - w) * 0.5f, cur.y + (avail.y - h) * 0.5f));

			// シーンのレンダーテクスチャを表示。アルファ強制1のSRVを使い、
			// 透明オブジェクト部分がRTのアルファでImGui背景と混ざって暗くなるのを防ぐ。
			ImTextureID tex = static_cast<ImTextureID>(dx->GetOffScreenDisplayGPUHandle().ptr);
			ImGui::Image(tex, ImVec2(w, h));

			// 画像のスクリーン矩形を記録(ギズモ等の座標変換に使用)
			ImVec2 rmin = ImGui::GetItemRectMin();
			ImVec2 rmax = ImGui::GetItemRectMax();
			gameImageMin_[0] = rmin.x; gameImageMin_[1] = rmin.y;
			gameImageMax_[0] = rmax.x; gameImageMax_[1] = rmax.y;
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

// =============================================================
// 左パネル：シーン設定
// =============================================================
// =============================================================
// 右パネル：ヒエラルキー + インスペクター
// =============================================================
void EditorUI::DrawRightPanel() {
	if (!ImGui::Begin("ヒエラルキー")) { ImGui::End(); return; }

	// ---- ヒエラルキー ----
	ImGui::TextUnformatted("追加オブジェクト");
	ImGui::Separator();
	const char* typeIcon[] = { "[3D]", "[2D]", "[P]", "[FX]" };
	for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
		EditorObject* o = objects_[i].get();
		ImGui::PushID(i);
		std::string label = std::string(typeIcon[o->type]) + " " + o->name;
		if (ImGui::Selectable(label.c_str(), selectedObject_ == i)) {
			selectedObject_ = i;
		}
		ImGui::PopID();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextUnformatted("インスペクター");
	ImGui::Separator();

	if (selectedObject_ >= 0 && selectedObject_ < static_cast<int>(objects_.size())) {
		EditorObject* o = objects_[selectedObject_].get();
		ImGui::Text("名前: %s", o->name.c_str());
		ImGui::Text("リソース: %s", o->resource.c_str());
		ImGui::Checkbox("表示", &o->visible);

		switch (o->type) {
		case 0: // 3D
			ImGui::DragFloat3("位置", &o->position.x, 0.05f);
			ImGui::DragFloat3("回転", &o->rotation.x, 0.01f);
			ImGui::DragFloat3("大きさ", &o->scale.x, 0.05f);
			break;
		case 1: // Sprite
			ImGui::DragFloat2("位置", &o->spritePos.x, 1.0f);
			ImGui::DragFloat("回転", &o->spriteRot, 0.01f);
			ImGui::DragFloat2("大きさ", &o->spriteSize.x, 1.0f);
			break;
		case 2: // Particle
			ImGui::DragFloat3("位置", &o->position.x, 0.05f);
			ImGui::DragFloat3("大きさ", &o->scale.x, 0.05f);
			break;
		default:
			ImGui::TextDisabled("(調整項目なし)");
			break;
		}

		if (ImGui::Button("削除", ImVec2(-1.0f, 0.0f))) {
			objects_.erase(objects_.begin() + selectedObject_);
			selectedObject_ = -1;
		}
	}
	else {
		ImGui::TextDisabled("オブジェクト未選択");
	}

	ImGui::End();
}

// =============================================================
// 追加オブジェクトの描画（ゲーム描画パスから呼ぶ）
// =============================================================
void EditorUI::DrawEditorObjects(const ViewProjection& vp) {
	if (objects_.empty()) { return; }

	// --- 3Dオブジェクト ---
	Object3dCommon::GetInstance()->DrawCommonSetting();
	for (auto& o : objects_) {
		if (o->type != 0 || !o->visible || !o->obj3d) { continue; }
		o->transform.scale_ = o->scale;
		o->transform.rotation_ = o->rotation;
		o->transform.translation_ = o->position;
		o->transform.UpdateMatrix();
		o->obj3d->Update(o->transform, vp);
		o->obj3d->Draw(o->transform, vp);
	}

	// --- パーティクル ---
	ParticleCommon::GetInstance()->DrawCommonSetting();
	for (auto& o : objects_) {
		if (o->type != 2 || !o->visible || !o->particle) { continue; }
		o->particle->SetPosition(o->position);
		o->particle->SetScale(o->scale);
		o->particle->Update(vp);
		o->particle->Draw(Normal);
	}

	// --- スプライト ---
	SpriteCommon::GetInstance()->DrawCommonSetting();
	for (auto& o : objects_) {
		if (o->type != 1 || !o->visible || !o->sprite) { continue; }
		o->sprite->SetPosition(o->spritePos);
		o->sprite->SetRotation(o->spriteRot);
		o->sprite->SetSize(o->spriteSize);
		o->sprite->Draw();
	}
}

} // namespace Engine
#endif // _DEBUG
