#pragma once

#include <cstdint>

/// <summary>
/// コリジョンID定義
/// </summary>
enum class CollisionTypeIdDef : uint32_t {
	kDefault,
	kPlayer,
	kPArm,
	kEnemy,
};