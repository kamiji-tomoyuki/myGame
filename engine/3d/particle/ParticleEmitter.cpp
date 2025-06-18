#include "ParticleEmitter.h"
#include "line/DrawLine3D.h"

ParticleEmitter::ParticleEmitter() {}

void ParticleEmitter::Initialize(const std::string& name, const std::string& fileName)
{
	// --- 引数で受け取りメンバ変数に記録 ---
	name_ = name;

	transform_.Initialize();

	manager_ = std::make_unique<ParticleManager>();
	manager_->Initialize(SrvManager::GetInstance());
	manager_->CreateParticleGroup(name_, fileName);

	// --- 各ステータスにデフォルト値を設定 ---
	emitFrequency_ = 0.1f;
	velocityMin_ = { -1.0f, -1.0f, -1.0f };
	velocityMax_ = { 1.0f, 1.0f, 1.0f };
	lifeTimeMin_ = { 1.0f };
	lifeTimeMax_ = { 3.0f };
	isVisible = true;
	startAcce_ = { 1.0f,1.0f,1.0f };
	endAcce_ = { 1.0f,1.0f,1.0f };
	startScale_ = { 1.0f,1.0f,1.0f };
	endScale_ = { 1.0f,1.0f,1.0f };
	rotateVelocityMin = { -0.07f,-0.07f,-0.07f };
	rotateVelocityMax = { 0.07f,0.07f,0.07f };
	count_ = 3;
	alphaMin_ = 1.0f;
	alphaMax_ = 1.0f;
	AddItem();
	isBillBoard = false;
	isActive_ = true;
	isAcceMultiply = false;
	allScaleMin = { 1.0f,1.0f,1.0f };
	allScaleMax = { 1.0f,1.0f,1.0f };

	ApplyGlobalVariables();
}

void ParticleEmitter::Update(const ViewProjection& vp_) {
	SetValue();
	elapsedTime_ += deltaTime;

	while (elapsedTime_ >= emitFrequency_) {
		Emit();
		elapsedTime_ -= emitFrequency_;
	}

	manager_->Update(vp_);
	transform_.UpdateMatrix();
}

void ParticleEmitter::UpdateOnce(const ViewProjection& vp_)
{
	SetValue();
	if (!isActive_) {
		Emit();
		isActive_ = true;
	}
	manager_->Update(vp_);
	transform_.UpdateMatrix();
}

void ParticleEmitter::Draw(PrimitiveType primitiveType)
{
	manager_->SetRandomRotate(isRandomRotate);
	manager_->SetRandomRotateY(isRandomRotateY);
	manager_->SetAcceMultipy(isAcceMultiply);
	manager_->SetBillBorad(isBillBoard);
	manager_->SetRandomSize(isRandomScale);
	manager_->SetAllRandomSize(isAllRamdomScale);
	manager_->SetSinMove(isSinMove);
	manager_->Draw(primitiveType);
}

void ParticleEmitter::DrawEmitter()
{
	if (!isVisible) return;

	std::array<Vector3, 8> localVertices = {
		Vector3{-1.0f, -1.0f, -1.0f}, // 左下前
		Vector3{ 1.0f, -1.0f, -1.0f}, // 右下前
		Vector3{-1.0f,  1.0f, -1.0f}, // 左上前
		Vector3{ 1.0f,  1.0f, -1.0f}, // 右上前
		Vector3{-1.0f, -1.0f,  1.0f}, // 左下奥
		Vector3{ 1.0f, -1.0f,  1.0f}, // 右下奥
		Vector3{-1.0f,  1.0f,  1.0f}, // 左上奥
		Vector3{ 1.0f,  1.0f,  1.0f}  // 右上奥
	};

	std::array<Vector3, 8> worldVertices;

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale_, transform_.rotation_, transform_.translation_);

	for (size_t i = 0; i < localVertices.size(); i++) {
		worldVertices[i] = Transformation(localVertices[i], worldMatrix);
	}

	constexpr std::array<std::pair<int, int>, 12> edges = {
		std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0), // 前面
		std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4), // 背面
		std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)  // 側面
	};

	for (const auto& edge : edges) {
		DrawLine3D::GetInstance()->SetPoints(worldVertices[edge.first], worldVertices[edge.second]);
	}
}


void ParticleEmitter::Emit() {
	manager_->Emit(
		name_,
		transform_.translation_,
		count_,
		transform_.scale_,
		velocityMin_,
		velocityMax_,
		lifeTimeMin_,
		lifeTimeMax_,
		startScale_,
		endScale_,
		startAcce_,
		endAcce_,
		startRote_,
		endRote_,
		isRandomColor,
		alphaMin_,
		alphaMax_,
		rotateVelocityMin,
		rotateVelocityMax,
		allScaleMax,
		allScaleMin,
		scaleMin,
		scaleMax,
		transform_.rotation_
	);
}

void ParticleEmitter::ApplyGlobalVariables()
{
	emitFrequency_ = globalVariables->GetFloatValue(groupName, "emitFrequency");
	count_ = globalVariables->GetIntValue(groupName, "count");
	transform_.translation_ = globalVariables->GetVector3Value(groupName, "Emit translation");
	transform_.scale_ = globalVariables->GetVector3Value(groupName, "Emit scale");
	transform_.rotation_ = globalVariables->GetVector3Value(groupName, "Emit rotation");
	startScale_ = globalVariables->GetVector3Value(groupName, "Particle StartScale");
	endScale_ = globalVariables->GetVector3Value(groupName, "Particle EndScale");
	startRote_ = globalVariables->GetVector3Value(groupName, "Particle StartRote");
	endRote_ = globalVariables->GetVector3Value(groupName, "Particle EndRote");
	startAcce_ = globalVariables->GetVector3Value(groupName, "Particle StartAcce");
	endAcce_ = globalVariables->GetVector3Value(groupName, "Particle EndAcce");
	velocityMin_ = globalVariables->GetVector3Value(groupName, "minVelocity");
	velocityMax_ = globalVariables->GetVector3Value(groupName, "maxVelocity");
	lifeTimeMax_ = globalVariables->GetFloatValue(groupName, "lifeTimeMax");
	lifeTimeMin_ = globalVariables->GetFloatValue(groupName, "lifeTimeMin");
	isVisible = globalVariables->GetBoolValue(groupName, "isVisible");
	isBillBoard = globalVariables->GetBoolValue(groupName, "isBillBoard");
	isRandomColor = globalVariables->GetBoolValue(groupName, "isRamdomColor");
	alphaMin_ = globalVariables->GetFloatValue(groupName, "alphaMin");
	alphaMax_ = globalVariables->GetFloatValue(groupName, "alphaMax");
	isRandomRotate = globalVariables->GetBoolValue(groupName, "isRandomRotate");
	isRandomRotateY = globalVariables->GetBoolValue(groupName, "isRandomRotateY");
	isAcceMultiply = globalVariables->GetBoolValue(groupName, "isAcceMultiply");
	rotateVelocityMin = globalVariables->GetVector3Value(groupName, "RotationVelo Min");
	rotateVelocityMax = globalVariables->GetVector3Value(groupName, "RotationVelo Max");
	allScaleMax = globalVariables->GetVector3Value(groupName, "AllScale Max");
	allScaleMin = globalVariables->GetVector3Value(groupName, "AllScale Min");
	scaleMin = globalVariables->GetFloatValue(groupName, "Scale Min");
	scaleMax = globalVariables->GetFloatValue(groupName, "Scale Max");
	isRandomScale = globalVariables->GetBoolValue(groupName, "isRandomScale");
	isAllRamdomScale = globalVariables->GetBoolValue(groupName, "isAllRamdomScale");
	isSinMove = globalVariables->GetBoolValue(groupName, "isSinMove");
}

void ParticleEmitter::SetValue()
{
	globalVariables->SetValue(groupName, "emitFrequency", emitFrequency_);
	globalVariables->SetValue(groupName, "count", count_);
	globalVariables->SetValue(groupName, "Emit translation", transform_.translation_);
	globalVariables->SetValue(groupName, "Emit scale", transform_.scale_);
	globalVariables->SetValue(groupName, "Emit rotation", transform_.rotation_);
	globalVariables->SetValue(groupName, "Particle StartScale", startScale_);
	globalVariables->SetValue(groupName, "Particle StartRote", startRote_);
	globalVariables->SetValue(groupName, "Particle EndRote", endRote_);
	globalVariables->SetValue(groupName, "Particle EndScale", endScale_);
	globalVariables->SetValue(groupName, "Particle StartAcce", startAcce_);
	globalVariables->SetValue(groupName, "Particle EndAcce", endAcce_);
	globalVariables->SetValue(groupName, "minVelocity", velocityMin_);
	globalVariables->SetValue(groupName, "maxVelocity", velocityMax_);
	globalVariables->SetValue(groupName, "lifeTimeMax", lifeTimeMax_);
	globalVariables->SetValue(groupName, "lifeTimeMin", lifeTimeMin_);
	globalVariables->SetValue(groupName, "isVisible", isVisible);
	globalVariables->SetValue(groupName, "isBillBoard", isBillBoard);
	globalVariables->SetValue(groupName, "isRamdomColor", isRandomColor);
	globalVariables->SetValue(groupName, "alphaMin", alphaMin_);
	globalVariables->SetValue(groupName, "alphaMax", alphaMax_);
	globalVariables->SetValue(groupName, "isRandomRotate", isRandomRotate);
	globalVariables->SetValue(groupName, "isRandomRotateY", isRandomRotateY);
	globalVariables->SetValue(groupName, "RotationVelo Min", rotateVelocityMin);
	globalVariables->SetValue(groupName, "isAcceMultiply", isAcceMultiply);
	globalVariables->SetValue(groupName, "RotationVelo Max", rotateVelocityMax);
	globalVariables->SetValue(groupName, "AllScale Max", allScaleMax);
	globalVariables->SetValue(groupName, "AllScale Min", allScaleMin);
	globalVariables->SetValue(groupName, "Scale Min", scaleMin);
	globalVariables->SetValue(groupName, "Scale Max", scaleMax);
	globalVariables->SetValue(groupName, "isRandomScale", isRandomScale);
	globalVariables->SetValue(groupName, "isAllRamdomScale", isAllRamdomScale);
	globalVariables->SetValue(groupName, "isSinMove", isSinMove);
}

void ParticleEmitter::AddItem()
{
	groupName = name_.c_str();
	globalVariables = GlobalVariables::GetInstance();
	globalVariables->CreateGroup(groupName);
	globalVariables->AddItem(groupName, "emitFrequency", emitFrequency_);
	globalVariables->AddItem(groupName, "count", count_);
	globalVariables->AddItem(groupName, "Emit translation", transform_.translation_);
	globalVariables->AddItem(groupName, "Emit scale", transform_.scale_);
	globalVariables->AddItem(groupName, "Emit rotation", transform_.rotation_);
	globalVariables->AddItem(groupName, "Particle StartScale", startScale_);
	globalVariables->AddItem(groupName, "Particle EndScale", endScale_);
	globalVariables->AddItem(groupName, "Particle StartRote", startRote_);
	globalVariables->AddItem(groupName, "Particle EndRote", endRote_);
	globalVariables->AddItem(groupName, "Particle StartAcce", startAcce_);
	globalVariables->AddItem(groupName, "Particle EndAcce", endAcce_);
	globalVariables->AddItem(groupName, "minVelocity", velocityMin_);
	globalVariables->AddItem(groupName, "maxVelocity", velocityMax_);
	globalVariables->AddItem(groupName, "lifeTimeMax", lifeTimeMax_);
	globalVariables->AddItem(groupName, "lifeTimeMin", lifeTimeMin_);
	globalVariables->AddItem(groupName, "isRamdomColor", isRandomColor);
	globalVariables->AddItem(groupName, "alphaMin", alphaMin_);
	globalVariables->AddItem(groupName, "alphaMax", alphaMax_);
	globalVariables->AddItem(groupName, "AllScale Max", allScaleMax);
	globalVariables->AddItem(groupName, "AllScale Min", allScaleMin);
	globalVariables->AddItem(groupName, "Scale Min", scaleMin);
	globalVariables->AddItem(groupName, "Scale Max", scaleMax);
	globalVariables->AddItem(groupName, "isVisible", isVisible);
	globalVariables->AddItem(groupName, "isBillBoard", isBillBoard);
	globalVariables->AddItem(groupName, "isRandomRotate", isRandomRotate);
	globalVariables->AddItem(groupName, "isRandomRotateY", isRandomRotateY);
	globalVariables->AddItem(groupName, "isAcceMultiply", isAcceMultiply);
	globalVariables->AddItem(groupName, "RotationVelo Min", rotateVelocityMin);
	globalVariables->AddItem(groupName, "RotationVelo Max", rotateVelocityMax);
	globalVariables->AddItem(groupName, "isRandomScale", isRandomScale);
	globalVariables->AddItem(groupName, "isAllRamdomScale", isAllRamdomScale);
	globalVariables->AddItem(groupName, "isSinMove", isSinMove);
}

void ParticleEmitter::imgui() {
#ifdef _DEBUG
    // カスタムスタイル設定
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 元のスタイルを保持
    ImGuiStyle oldStyle = style;

    // パーティクルエディタ用のカスタムスタイル
    style.FrameRounding = 4.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 6);

    // ヘッダーカラー
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.4f, 0.8f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.2f, 0.4f, 0.8f, 0.65f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.2f, 0.4f, 0.8f, 0.85f);

    // カラフルなセクション向けのカラーパレット定義
    ImVec4 sectionColors[] = {
        ImVec4(0.2f, 0.6f, 0.9f, 0.95f),  // 青系
        ImVec4(0.9f, 0.5f, 0.2f, 0.95f),  // オレンジ系
        ImVec4(0.2f, 0.7f, 0.4f, 0.95f),  // 緑系
        ImVec4(0.7f, 0.3f, 0.7f, 0.95f),  // 紫系
        ImVec4(0.8f, 0.3f, 0.4f, 0.95f)   // 赤系
    };

    ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);

    // メインウィンドウ開始
    ImGui::Begin(name_.c_str(), nullptr);

    // ヘッダーセクション
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, sectionColors[0]);
    ImGui::Button("パーティクルエディタ", ImVec2(ImGui::GetContentRegionAvail().x, 0));
    ImGui::PopStyleColor(2);
    ImGui::Spacing();

    // 操作ガイド
    if (ImGui::Button("ヘルプ"))
        ImGui::OpenPopup("HelpPopup");

    if (ImGui::BeginPopup("HelpPopup")) {
        ImGui::Text("パーティクルエディタの使い方");
        ImGui::Separator();
        ImGui::BulletText("各セクションを開くには、ヘッダーをクリックします");
        ImGui::BulletText("Ctrl+クリックで微調整、Shift+クリックで大きく調整");
        ImGui::BulletText("パラメータにカーソルを合わせると詳細が表示されます");
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("プリセット保存"))
        ImGui::OpenPopup("SavePresetPopup");

    ImGui::SameLine();
    if (ImGui::Button("プリセット読込"))
        ImGui::OpenPopup("LoadPresetPopup");

    // プリセット保存ポップアップ
    static char presetName[64] = "";
    if (ImGui::BeginPopup("SavePresetPopup")) {
        ImGui::Text("プリセット名:");
        ImGui::InputText("##PresetName", presetName, IM_ARRAYSIZE(presetName));
        if (ImGui::Button("保存", ImVec2(120, 0))) {
            // プリセット保存処理をここに実装
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // プリセット読込ポップアップ
    if (ImGui::BeginPopup("LoadPresetPopup")) {
        ImGui::Text("プリセットを選択:");
        // ダミープリセットリスト
        static const char* presets[] = { "Default", "Fire", "Smoke", "Explosion", "Magic" };
        static int selectedPreset = 0;
        ImGui::ListBox("##PresetList", &selectedPreset, presets, IM_ARRAYSIZE(presets), 5);

        if (ImGui::Button("読込", ImVec2(120, 0))) {
            // プリセット読込処理をここに実装
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::Spacing();

    // エミッターデータ
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, sectionColors[0]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(sectionColors[0].x, sectionColors[0].y, sectionColors[0].z, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(sectionColors[0].x, sectionColors[0].y, sectionColors[0].z, 0.9f));

    if (ImGui::CollapsingHeader("エミッターデータ", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PopStyleColor(4); // Header styleを戻す前に先に色設定をポップ

        ImGui::BeginChild("EmitterData", ImVec2(0, 180), true);

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "Transformデータ:");
        ImGui::Separator();

        // テーブルレイアウトの使用（新版ImGuiの機能）
        if (ImGui::BeginTable("TransformTable", 2, ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch);

            // 位置
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "位置");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragFloat3("##位置", &transform_.translation_.x, 0.1f)) {
                // 変更時の処理
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("エミッターの位置を設定します (X, Y, Z)");

            // 回転
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "回転");
            ImGui::TableNextColumn();
            float rotationDegrees[3] = {
                radiansToDegrees(transform_.rotation_.x),
                radiansToDegrees(transform_.rotation_.y),
                radiansToDegrees(transform_.rotation_.z)
            };

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragFloat3("##回転 (度)", rotationDegrees, 0.1f, -360.0f, 360.0f)) {
                transform_.rotation_.x = degreesToRadians(rotationDegrees[0]);
                transform_.rotation_.y = degreesToRadians(rotationDegrees[1]);
                transform_.rotation_.z = degreesToRadians(rotationDegrees[2]);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("エミッターの回転を設定します (度)");

            // 大きさ
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "大きさ");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragFloat3("##大きさ", &transform_.scale_.x, 0.1f, 0.0f)) {
                // 変更時の処理
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("エミッターの大きさを設定します (X, Y, Z)");

            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::Checkbox("表示", &isVisible);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("エミッターを表示するかどうかを設定します");

        ImGui::EndChild();
    }
    else {
        ImGui::PopStyleColor(4);
    }

    // パーティクルデータ
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, sectionColors[1]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(sectionColors[1].x, sectionColors[1].y, sectionColors[1].z, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(sectionColors[1].x, sectionColors[1].y, sectionColors[1].z, 0.9f));

    if (ImGui::CollapsingHeader("パーティクルデータ")) {
        ImGui::PopStyleColor(4);

        // 寿命
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.2f, 0.1f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.3f, 0.2f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.5f, 0.4f, 0.3f, 0.8f));

        if (ImGui::TreeNodeEx("寿命", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("LifetimeSection", ImVec2(0, 80), true);

            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "寿命設定:");
            ImGui::Separator();

            float prevLifeTimeMax = lifeTimeMax_;
            float prevLifeTimeMin = lifeTimeMin_;

            // スライダーとドラッグを組み合わせた入力
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
            if (ImGui::SliderFloat("最大値", &lifeTimeMax_, 0.1f, 10.0f, "%.2f秒")) {
                if (lifeTimeMax_ < lifeTimeMin_)
                    lifeTimeMin_ = lifeTimeMax_;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##MaxLifeTimeFine", &lifeTimeMax_, 0.01f);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("パーティクルの最大寿命 (秒)");

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
            if (ImGui::SliderFloat("最小値", &lifeTimeMin_, 0.0f, lifeTimeMax_, "%.2f秒")) {
                lifeTimeMin_ = std::clamp(lifeTimeMin_, 0.0f, lifeTimeMax_);
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##MinLifeTimeFine", &lifeTimeMin_, 0.01f);
            lifeTimeMin_ = std::clamp(lifeTimeMin_, 0.0f, lifeTimeMax_);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("パーティクルの最小寿命 (秒)");

            // 変更を視覚的に表示
            if (prevLifeTimeMax != lifeTimeMax_ || prevLifeTimeMin != lifeTimeMin_) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.3f, 1.0f), "設定が変更されました");
            }

            ImGui::EndChild();
            ImGui::TreePop();
        }
        ImGui::PopStyleColor(4);

        ImGui::Separator();

        // 速度、加速度
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 1.0f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.3f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.4f, 0.3f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.5f, 0.4f, 0.8f));

        if (ImGui::TreeNodeEx("速度、加速度", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("VelocitySection", ImVec2(0, 200), true);

            if (ImGui::BeginTabBar("VelocityTabs")) {
                if (ImGui::BeginTabItem("速度")) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "初期速度範囲設定:");

                    if (ImGui::BeginTable("VelocityTable", 4, ImGuiTableFlags_Borders)) {
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("最大値");

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##VelMaxX", &velocityMax_.x, 0.1f);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##VelMaxY", &velocityMax_.y, 0.1f);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##VelMaxZ", &velocityMax_.z, 0.1f);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("最小値");

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##VelMinX", &velocityMin_.x, 0.1f);
                        velocityMin_.x = std::clamp(velocityMin_.x, -FLT_MAX, velocityMax_.x);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##VelMinY", &velocityMin_.y, 0.1f);
                        velocityMin_.y = std::clamp(velocityMin_.y, -FLT_MAX, velocityMax_.y);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##VelMinZ", &velocityMin_.z, 0.1f);
                        velocityMin_.z = std::clamp(velocityMin_.z, -FLT_MAX, velocityMax_.z);

                        ImGui::EndTable();
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("加速度")) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "加速度設定:");

                    if (ImGui::BeginTable("AccelerationTable", 4, ImGuiTableFlags_Borders)) {
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("最初");

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##StartAcceX", &startAcce_.x, 0.001f);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##StartAcceY", &startAcce_.y, 0.001f);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##StartAcceZ", &startAcce_.z, 0.001f);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("最後");

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##EndAcceX", &endAcce_.x, 0.001f);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##EndAcceY", &endAcce_.y, 0.001f);

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##EndAcceZ", &endAcce_.z, 0.001f);

                        ImGui::EndTable();
                    }

                    ImGui::Checkbox("乗算", &isAcceMultiply);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("加速度を速度に乗算するかどうか設定します");

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::EndChild();
            ImGui::TreePop();
        }
        ImGui::PopStyleColor(4);

        ImGui::Separator();

        // 大きさ
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.1f, 0.1f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.2f, 0.2f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.5f, 0.3f, 0.3f, 0.8f));

        if (ImGui::TreeNodeEx("大きさ", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("ScaleSection", ImVec2(0, 220), true);

            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "大きさの設定:");

            // チェックボックスをグループ化
            ImGui::BeginGroup();
            ImGui::Checkbox("均等にランダムな大きさ", &isRandomScale);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("XYZが同じ値のランダムな大きさにします");

            ImGui::SameLine(ImGui::GetWindowWidth() * 0.5f);
            ImGui::Checkbox("ばらばらにランダムな大きさ", &isAllRamdomScale);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("XYZがそれぞれ独立したランダムな大きさにします");

            ImGui::Checkbox("sin波の動き", &isSinMove);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("大きさをsin波で変化させます");
            ImGui::EndGroup();

            ImGui::Separator();

            // 大きさの設定
            if (isAllRamdomScale) {
                // ばらばらランダム
                if (ImGui::BeginTable("AllRandomScaleTable", 4, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("最初");

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##StartScaleX", &startScale_.x, 0.1f, 0.0f);

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##StartScaleY", &startScale_.y, 0.1f, 0.0f);

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##StartScaleZ", &startScale_.z, 0.1f, 0.0f);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("最後");

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##EndScaleX", &endScale_.x, 0.1f, 0.0f);

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##EndScaleY", &endScale_.y, 0.1f, 0.0f);

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##EndScaleZ", &endScale_.z, 0.1f, 0.0f);

                    ImGui::EndTable();
                }
            }

            ImGui::EndChild();
            ImGui::TreePop();
        }
        ImGui::PopStyleColor(4);

        ImGui::Separator();

        // 回転
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.3f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.4f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.5f, 0.8f));

        if (ImGui::TreeNodeEx("回転", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("RotationSection", ImVec2(0, 180), true);

            ImGui::Checkbox("ランダムな回転", &isRandomRotate);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("回転をランダムにするかどうかを設定します");

            ImGui::Checkbox("Y軸を中心にランダムな回転", &isRandomRotateY);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Y軸を中心に回転をランダムにするかどうかを設定します");

            ImGui::Separator();

            if (isRandomRotate) {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 1.0f, 1.0f), "回転速度範囲:");

                if (ImGui::BeginTable("RotateVelocityTable", 4, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("最大値");

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##RotVelMaxX", &rotateVelocityMax.x, 0.01f);

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##RotVelMaxY", &rotateVelocityMax.y, 0.01f);

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##RotVelMaxZ", &rotateVelocityMax.z, 0.01f);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("最小値");

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##RotVelMinX", &rotateVelocityMin.x, 0.01f);
                    rotateVelocityMin.x = std::clamp(rotateVelocityMin.x, -FLT_MAX, rotateVelocityMax.x);

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##RotVelMinY", &rotateVelocityMin.y, 0.01f);
                    rotateVelocityMin.y = std::clamp(rotateVelocityMin.y, -FLT_MAX, rotateVelocityMax.y);

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##RotVelMinZ", &rotateVelocityMin.z, 0.01f);
                    rotateVelocityMin.z = std::clamp(rotateVelocityMin.z, -FLT_MAX, rotateVelocityMax.z);

                    ImGui::EndTable();
                }
            }
            else if (isRandomRotateY) {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 1.0f, 1.0f), "回転速度範囲:");

                if (ImGui::BeginTable("RotateVelocityTable", 2, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("最大値");

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##RotVelMaxY", &rotateVelocityMax.y, 0.01f);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("最小値");

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::DragFloat("##RotVelMinY", &rotateVelocityMin.y, 0.01f);
                    rotateVelocityMin.y = std::clamp(rotateVelocityMin.y, -FLT_MAX, rotateVelocityMax.y);

                    ImGui::EndTable();
                }
            }
            else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 1.0f, 1.0f), "開始/終了の回転 (度):");

                if (ImGui::BeginTable("RotationTable", 4, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);

                    float startRotationDegrees[3] = {
                        radiansToDegrees(startRote_.x),
                        radiansToDegrees(startRote_.y),
                        radiansToDegrees(startRote_.z)
                    };

                    float endRotationDegrees[3] = {
                        radiansToDegrees(endRote_.x),
                        radiansToDegrees(endRote_.y),
                        radiansToDegrees(endRote_.z)
                    };

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("最初");

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::DragFloat("##StartRotX", &startRotationDegrees[0], 0.1f)) {
                        startRote_.x = degreesToRadians(startRotationDegrees[0]);
                    }

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::DragFloat("##StartRotY", &startRotationDegrees[1], 0.1f)) {
                        startRote_.y = degreesToRadians(startRotationDegrees[1]);
                    }

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::DragFloat("##StartRotZ", &startRotationDegrees[2], 0.1f)) {
                        startRote_.z = degreesToRadians(startRotationDegrees[2]);
                    }

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("最後");

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::DragFloat("##EndRotX", &endRotationDegrees[0], 0.1f)) {
                        endRote_.x = degreesToRadians(endRotationDegrees[0]);
                    }

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::DragFloat("##EndRotY", &endRotationDegrees[1], 0.1f)) {
                        endRote_.y = degreesToRadians(endRotationDegrees[1]);
                    }

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::DragFloat("##EndRotZ", &endRotationDegrees[2], 0.1f)) {
                        endRote_.z = degreesToRadians(endRotationDegrees[2]);
                    }

                    ImGui::EndTable();
                }
            }


            ImGui::EndChild();
            ImGui::TreePop();
        }




        ImGui::PopStyleColor(4);

        ImGui::Separator();

        // 透明度
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));

        if (ImGui::TreeNodeEx("透明度", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("AlphaSection", ImVec2(0, 110), true);

            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "透明度の設定:");

            // 透明度プレビュー
            ImVec2 previewSize = ImVec2(ImGui::GetContentRegionAvail().x, 20);
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 p1 = ImVec2(p0.x + previewSize.x, p0.y + previewSize.y);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // グラデーションの描画
            ImU32 colorStart = IM_COL32(255, 255, 255, (int)(alphaMax_ * 255));
            ImU32 colorEnd = IM_COL32(255, 255, 255, (int)(alphaMin_ * 255));
            draw_list->AddRectFilledMultiColor(p0, p1, colorStart, colorStart, colorEnd, colorEnd);
            draw_list->AddRect(p0, p1, IM_COL32(100, 100, 100, 255));

            ImGui::Dummy(previewSize);
            ImGui::Spacing();

            // 透明度スライダー
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
            ImGui::SliderFloat("最大値", &alphaMax_, 0.0f, 1.0f, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##AlphaMaxFine", &alphaMax_, 0.01f, 0.0f, 1.0f);

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
            ImGui::SliderFloat("最小値", &alphaMin_, 0.0f, alphaMax_, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##AlphaMinFine", &alphaMin_, 0.01f, 0.0f, alphaMax_);
            alphaMin_ = std::clamp(alphaMin_, 0.0f, alphaMax_);

            ImGui::EndChild();
            ImGui::TreePop();
        }
        ImGui::PopStyleColor(4);
    }
    else {
        ImGui::PopStyleColor(4);
    }

    // パーティクルの数、間隔
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, sectionColors[2]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(sectionColors[2].x, sectionColors[2].y, sectionColors[2].z, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(sectionColors[2].x, sectionColors[2].y, sectionColors[2].z, 0.9f));

    if (ImGui::CollapsingHeader("パーティクルの数、間隔")) {
        ImGui::PopStyleColor(4);

        ImGui::BeginChild("EmissionSection", ImVec2(0, 100), true);

        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "エミッション設定:");

        // エミッション間隔とパーティクル数のスライダー
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
        ImGui::SliderFloat("間隔 (秒)", &emitFrequency_, 0.1f, 5.0f, "%.2f秒");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::DragFloat("##FrequencyFine", &emitFrequency_, 0.01f, 0.1f, 100.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("パーティクル生成の間隔（秒）");

        static int prevCount = count_;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
        ImGui::SliderInt("数", &count_, 1, 100, "%d個");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputInt("##CountFine", &count_, 1, 10);
        count_ = std::clamp(count_, 0, 10000);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("生成するパーティクルの数");

        // 変更があった場合に表示
        if (prevCount != count_) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "警告: 数が多すぎるとパフォーマンスに影響します");
        }

        ImGui::EndChild();
    }
    else {
        ImGui::PopStyleColor(4);
    }

    // 各状態の設定
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, sectionColors[3]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(sectionColors[3].x, sectionColors[3].y, sectionColors[3].z, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(sectionColors[3].x, sectionColors[3].y, sectionColors[3].z, 0.9f));

    if (ImGui::CollapsingHeader("各状態の設定")) {
        ImGui::PopStyleColor(4);

        ImGui::BeginChild("StateSection", ImVec2(0, 100), true);

        ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.8f, 1.0f), "レンダリング設定:");

        ImGui::BeginGroup();
        // ビルボードとカラー設定
        ImGui::Checkbox("ビルボード", &isBillBoard);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("パーティクルをカメラに向けるかどうかを設定します");

        ImGui::SameLine(ImGui::GetWindowWidth() * 0.5f);
        ImGui::Checkbox("ランダムカラー", &isRandomColor);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("パーティクルの色をランダムにするかどうかを設定します");
        ImGui::EndGroup();

        // 色の設定（ランダムカラーが有効な場合のみ表示）
        if (isRandomColor) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.8f, 1.0f), "色範囲設定:");

            //ImGui::ColorEdit3("開始色", (float*)&startColor_, ImGuiColorEditFlags_Float);
            //ImGui::ColorEdit3("終了色", (float*)&endColor_, ImGuiColorEditFlags_Float);
        }

        ImGui::EndChild();
    }
    else {
        ImGui::PopStyleColor(4);
    }

    // コントロールパネル (下部)
    ImGui::Separator();
    ImGui::Spacing();

    // スタイルを元に戻す
    style = oldStyle;

    ImGui::End();

#endif
}
