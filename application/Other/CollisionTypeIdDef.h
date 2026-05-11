#pragma once

#include <cstdint>

/// <summary>
/// コリジョンID定義
/// </summary>
enum class CollisionTypeIdDef : uint32_t {
	kNone,
	kDefault,
	kPlayer,
	kPRArm,
	kPLArm,
	kEnemy,
};