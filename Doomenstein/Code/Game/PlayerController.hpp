#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Core/Rgba8.hpp"

#include "Game/GameCommon.hpp"
#include "Game/ActorUID.hpp"
#include "Game/Controller.hpp"

#include <string>

class Map;
class Game;
class Actor;
class Camera;
class SpriteAnimDefinition;

//------------------------------------------------------------------------------------------------
class PlayerController : public Controller
{
	friend class Actor;
public:
					PlayerController(Game* owner, AABB2 viewport, float aspect, int playerIndex);
	virtual			~PlayerController();

	virtual void	Possess(Actor* actor) override;
	virtual Actor*	GetActor() const override;

	virtual void	Update(float deltaseconds) override;

	void			RenderHUD() const;

	void			UpdateCamera(float deltaseconds);
	void			UpdateInput(float deltaseconds);

	void			CheckInWhiteShrine(IntVec2 shrinePos);
	void			CheckInRedShrine(IntVec2 shrinePos);
	void			CheckInGreenShrine(IntVec2 shrinePos);
	void			CheckInBlueShrine(IntVec2 shrinePos);

	virtual bool	IsPlayer() override;
	virtual void	DamagedBy(Actor* actor) override;
	virtual void	KilledBy(Actor* actor) override;
	virtual void	Killed(Actor* actor) override;

public:
	int				m_playerIndex			= -1;
	float			m_playerAspect			= 2.0f;
	float			m_corpseTime			= 0.0f;
	float			m_nextProjectileTimer	= 0.0f;
	float			m_weaponAnimTime		= 0.0f;
	float			m_weaponAnimDuration	= 0.0f;
	float			m_CameraShakeTime		= 0.0f;
	float			m_CameraShakeDuration	= 0.0f;
	std::string		m_weaponAnimName;
	bool			m_respawnPlayer			= false;
	bool			m_isWalking				= false;
	float			m_batteryLife			= 100.0f;
	float			m_stamina				= 100.0f;
	bool			m_isStaminaDrained		= false;
	bool			m_isInRedShrine			= false;
	bool			m_isInGreenShrine		= false;
	bool			m_isInBlueShrine		= false;
	Texture*		m_hudTexture			= nullptr;
	Texture*		m_reticleTexture		= nullptr;
	Camera*			m_worldCamera			= nullptr;
	Camera*			m_screenCamera			= nullptr;
	Vec3			m_position;
	Vec3			m_torchPosition;
	Vec3			m_torchForward;
	float			m_torchCutoff			= 0.5f;
	float			m_batteryDischargeRate	= 1.0f;
	Rgba8			m_torchColor			= Rgba8::WHITE;
	Rgba8			m_color					= Rgba8::WHITE;
	EulerAngles		m_orientationDegrees	= EulerAngles(45.0f, 30.0f, 0.0f);
	EulerAngles		m_angularVelocity;
	Game*			m_game					= nullptr;
	ActorUID		m_actorUID				= ActorUID::INVALID;
	Map*			m_map					= nullptr;
};