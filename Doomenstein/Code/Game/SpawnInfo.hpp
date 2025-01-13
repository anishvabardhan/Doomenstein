#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"

struct ActorDefinition;

struct SpawnInfo
{
	ActorDefinition* m_actorDef;
	Vec3 m_pos;
	EulerAngles m_orientation;
	Vec3 m_velocity;
};