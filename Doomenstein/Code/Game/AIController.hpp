#pragma once

#include "Game/GameCommon.hpp"
#include "Game/ActorUID.hpp"
#include "Game/Controller.hpp"
#include "Game/Map.hpp"

#include <string>

class Weapon;
class Actor;

//------------------------------------------------------------------------------------------------
class AIController : public Controller
{
	friend class Actor;

public:
								AIController();
	virtual						~AIController();

	virtual void				Update(float deltaseconds);

	void						DamagedBy(Actor* attacker);
	Actor*						GetActorWithinSight(float radius, float angle);
	bool						IsTargetInAttackRange();
public:
	std::string					m_animName;
	float						m_goalDegrees			= 0.0f;
	float						m_turnTimer				= 0.0f;
	bool						m_isInAttackingRange	= false;
	bool						m_isTurning				= false;
	RaycastResultDoomenstein	m_raycast;
	float						m_nextAttackTimer		= 0.0f;
	Weapon*						m_meleeWeapon			= nullptr;
	ActorUID					m_targetUID				= ActorUID::INVALID;
};