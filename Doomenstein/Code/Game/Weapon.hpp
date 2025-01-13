#pragma once

#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/RaycastUtils.hpp"

#include "Game/Map.hpp"

#include <string>
#include <vector>

struct Vec3;
class Actor;
class Timer;
class SpriteAnimDefinition;

struct WeaponDefinition
{
	std::string					m_name;
	float						m_refireTime		= 0.0f;
	int							m_rayCount			= 0;
	float						m_rayCone			= 0.0f;
	float						m_rayRange			= 0.0f;
	FloatRange					m_rayDamage;
	float						m_rayImpulse		= 0.0f;

	int							m_projectileCount	= 0;
	std::string					m_projectileActor;
	float						m_projectileCone	= 0.0f;
	float						m_projectileSpeed	= 0.0f;

	int							m_meleeCount		= 0;
	float						m_meleeArc			= 0.0f;
	float						m_meleeRange		= 0.0f;
	FloatRange					m_meleeDamage;
	float						m_meleeImpulse		= 0.0f;

	// HUD
	std::string					m_shader;
	std::string					m_baseTexture;
	std::string					m_reticleTexture;
	IntVec2						m_reticleSize;
	IntVec2						m_spriteSize;
	Vec2						m_spritePivot;

	// Animation
	std::string					m_animName[2];
	std::string					m_animShader[2];
	std::string					m_spriteSheet[2];
	IntVec2						m_cellCount[2];
	float						m_secondsPerFrame[2]	= {0.0f, 0.0f};
	int							m_startFrame[2]		= {-1, -1};
	int							m_endFrame[2]			= {-1, -1};

	std::vector<SpriteAnimDefinition*> m_weaponAnimDef;

	// Sounds
	std::string					m_sound;
	std::string					m_audioName;

	static WeaponDefinition		s_weaponDefinitions[3];

	static void					InitializeDefs();
};

class Weapon
{
public:
	Actor*						m_owner = nullptr;
	WeaponDefinition			m_definition;
	RaycastResultDoomenstein	m_rayFireCast;
public:
	Weapon();
	Weapon(WeaponDefinition definition, Actor* owner);
	~Weapon();

	void Fire();

	Vec3 GetRandomDirectionInCone() const;
};