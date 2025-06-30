#pragma once

#include <cstdint>

// コリジョンID定義
enum class CollisionTypeIdDef : uint32_t {
	kDefault,
	kPlayer,
	kPWeapon,
	kEnemy,
};