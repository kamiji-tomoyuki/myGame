#pragma once

#include <cstdint>

/// <summary>
/// コリジョンID定義
/// </summary>
using namespace Engine;
enum class CollisionTypeIdDef : uint32_t {
	kNone,
	kDefault,
	kPlayer,
	kPRArm,
	kPLArm,
	kEnemy,
};