#pragma once
#include <cstdint>

namespace eng::physics {

// Box2D collision filter bits. Each entity type belongs to exactly one
// category; its mask lists the categories it should collide with.
constexpr uint16_t kCatWorld        = 0x0001; // static map geometry
constexpr uint16_t kCatPlayer       = 0x0002; // player body
constexpr uint16_t kCatEnemy        = 0x0004; // enemy bodies
constexpr uint16_t kCatPlayerAttack = 0x0008; // player hitbox sensor
constexpr uint16_t kCatEnemyAttack  = 0x0010; // enemy hitbox sensor / projectile
constexpr uint16_t kCatProjectile   = 0x0020; // physical projectile body

// Precomputed masks ---------------------------------------------------------
// World: hit by player and enemy bodies (not attack sensors).
constexpr uint16_t kMaskWorld   = kCatPlayer | kCatEnemy | kCatProjectile;
// Player body: collides with world and enemy bodies, NOT with attack sensors.
constexpr uint16_t kMaskPlayer  = kCatWorld | kCatEnemy;
// Enemy body: collides with world and player body.
constexpr uint16_t kMaskEnemy   = kCatWorld | kCatPlayer;
// Player hitbox sensor: only triggers enemy hurtboxes (sensors matched by listener).
constexpr uint16_t kMaskPlayerAttack = kCatEnemy;
// Enemy hitbox sensor / projectile: only triggers player hurtboxes.
constexpr uint16_t kMaskEnemyAttack  = kCatPlayer;
// Projectile: collides with world and player body.
constexpr uint16_t kMaskProjectile   = kCatWorld | kCatPlayer;

} // namespace eng::physics
