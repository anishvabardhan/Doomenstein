#pragma once

#include "Engine/Math/Vec3.hpp"

#include "Game/GameCommon.hpp"
#include "Game/ActorUID.hpp"

//------------------------------------------------------------------------------------------------
class Actor;
class Map;
class Weapon;

//------------------------------------------------------------------------------------------------
class Controller
{
	friend class Actor;

public:
									Controller();
	virtual							~Controller();

	virtual void					Update(float deltaseconds);

	virtual void					Possess(Actor* actor);
	virtual Actor*					GetActor() const;

	virtual bool					IsPlayer();
	virtual void					DamagedBy(Actor* actor);
	virtual void					KilledBy(Actor* actor);
	virtual void					Killed(Actor* actor);

public:
	int								m_kills						= 0;
	int								m_deaths					= 0;
	int								m_equippedWeaponIndex		= 0;
	//bool							m_isControllerInput			= false;
	bool							m_isShooting				= false;
	bool							m_isPistolHit				= false;
	float							m_respawnTime				= 0.0f;
	ActorUID						m_actorUID					= ActorUID::INVALID;
	Map*							m_map						= nullptr;
	Vec3							m_velocity;
};