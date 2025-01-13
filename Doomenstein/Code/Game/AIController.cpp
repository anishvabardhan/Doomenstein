#include "Game/AIController.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Timer.hpp"

#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"
#include "Game/PlayerController.hpp"

AIController::AIController()
{
}

AIController::~AIController()
{
	GetActor()->OnUnpossessed(this);
}

void AIController::Update(float deltaseconds)
{
	Actor* self = m_actorUID.GetActor();
	m_meleeWeapon = self->m_weapons[2];
	
	m_actorUID.GetActor()->m_accelaration = Vec3::ZERO;

	if (!self)
	{
		return;
	}

	Actor* target = nullptr;

	//if (!target)
	{
		target = GetActorWithinSight(self->m_definition.m_sightRadius, self->m_definition.m_sightAngle);
	}

	if (target)
	{
		if (g_theAudio->IsPlaying(m_map->m_game->m_allSoundPlaybackIDs[GAME_DEMON_HURT]))
		{
			g_theAudio->SetSoundPosition(m_map->m_game->m_allSoundPlaybackIDs[GAME_DEMON_HURT], self->m_position);
		}

		for(int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
		{
			g_theAudio->UpdateListener(i, target->m_position, target->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), target->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
		}

		Vec3 directionToTarget = (target->m_position - self->m_position).GetNormalized();

		self->TurnInDirection(directionToTarget.GetAngleAboutZDegrees(), 180.0f * deltaseconds);

		if (!IsTargetInAttackRange())
		{
			self->MoveInDirection(self->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), self->m_definition.m_runSpeed * 4.0f);
		}

		if (IsTargetInAttackRange())
		{
			if (!target->m_controller->m_isPistolHit)
			{
				m_actorUID.GetActor()->m_accelaration = Vec3::ZERO;
				m_actorUID.GetActor()->m_velocity = Vec3::ZERO;
			}

			if (m_actorUID.GetActor()->m_animName == "Hurt")
			{
				if (m_actorUID.GetActor()->m_animTime >= m_actorUID.GetActor()->m_animDuration)
				{
					m_actorUID.GetActor()->m_animName = "Attack";
				}
			}

			m_isInAttackingRange = true;

			m_nextAttackTimer += deltaseconds;

			float timer = m_meleeWeapon->m_definition.m_refireTime;

			if (m_nextAttackTimer >= timer)
			{
				m_meleeWeapon->Fire();

				if (g_theAudio->IsPlaying(m_map->m_game->m_allSoundPlaybackIDs[GAME_DEMON_ATTACK]))
				{
					g_theAudio->SetSoundPosition(m_map->m_game->m_allSoundPlaybackIDs[GAME_DEMON_ATTACK], self->m_position);
				}

				for (int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
				{
					g_theAudio->UpdateListener(i, self->m_position, self->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), self->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
				}

				if (g_theAudio->IsPlaying(m_map->m_game->m_allSoundPlaybackIDs[GAME_PLAYER_HURT]))
				{
					g_theAudio->SetSoundPosition(m_map->m_game->m_allSoundPlaybackIDs[GAME_PLAYER_HURT], target->m_position);
				}

				for (int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
				{
					g_theAudio->UpdateListener(i, target->m_position, target->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), target->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
				}

				Vec3 impulseDirection = (m_targetUID.GetActor()->m_position - self->m_position).GetNormalized();

				target = m_targetUID.GetActor();

				target->m_accelaration = Vec3::ZERO;

				target->AddImpulse(4.0f * m_meleeWeapon->m_definition.m_meleeImpulse * impulseDirection);
				target->UpdatePhysics(deltaseconds);

				m_nextAttackTimer = 0.0f;
			}
		}
		else
		{
			m_isInAttackingRange = false;

			if (m_actorUID.GetActor()->m_animName == "Hurt")
			{
				if (m_actorUID.GetActor()->m_animTime >= m_actorUID.GetActor()->m_animDuration)
				{
					m_actorUID.GetActor()->m_animName = "Walk";
				}
			}
		}
	}
	else if (m_targetUID != ActorUID::INVALID)
	{
		if (m_targetUID.GetActor() != nullptr)
		{
			Vec3 directionToTarget = (m_targetUID.GetActor()->m_position - self->m_position).GetNormalized();

			self->TurnInDirection(directionToTarget.GetAngleAboutZDegrees(), 180.0f * deltaseconds);

			if (!IsTargetInAttackRange())
			{
				self->MoveInDirection(self->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), self->m_definition.m_runSpeed * 4.0f);
			}

			if (IsTargetInAttackRange())
			{
				if (!m_targetUID.GetActor()->m_controller->m_isPistolHit)
				{
					m_actorUID.GetActor()->m_accelaration = Vec3::ZERO;
					m_actorUID.GetActor()->m_velocity = Vec3::ZERO;
				}

				m_actorUID.GetActor()->m_animName = "Attack";

				m_isInAttackingRange = true;

				m_nextAttackTimer += deltaseconds;

				float timer = m_meleeWeapon->m_definition.m_refireTime;

				if (m_nextAttackTimer >= timer)
				{
					m_meleeWeapon->Fire();

					Vec3 impulseDirection = (m_targetUID.GetActor()->m_position - self->m_position).GetNormalized();

					m_targetUID.GetActor()->m_accelaration = Vec3::ZERO;

					m_targetUID.GetActor()->AddImpulse(m_meleeWeapon->m_definition.m_meleeImpulse * impulseDirection);
					m_targetUID.GetActor()->UpdatePhysics(deltaseconds);

					m_nextAttackTimer = 0.0f;
				}
			}
			else
			{
				m_isInAttackingRange = false;

				if (m_actorUID.GetActor()->m_animName == "Hurt")
				{
					if (m_actorUID.GetActor()->m_animTime >= m_actorUID.GetActor()->m_animDuration)
					{
						m_actorUID.GetActor()->m_animName = "Walk";
					}
				}
			}
		}
	}
	else
	{
		m_turnTimer += deltaseconds;

		RandomNumberGenerator random = RandomNumberGenerator();

		if (m_turnTimer > 2.0f)
		{
			m_isTurning = true;

			m_goalDegrees = random.RollRandomFloatInRange(-180.0f, 180.0f);
			m_turnTimer = 0.0f;
		}

		if (m_isTurning)
		{
			self->m_orientation.m_yawDegrees = GetTurnedTowardDegrees(self->m_orientation.m_yawDegrees, m_goalDegrees, 180.0f * deltaseconds);

			if (self->m_orientation.m_yawDegrees == m_goalDegrees)
			{
				m_isTurning = false;
			}
		}
		else
		{
			self->MoveInDirection(self->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), self->m_definition.m_runSpeed * 4.0f);
		}
	}
}

void AIController::DamagedBy(Actor* attacker)
{
	if (attacker->m_definition.m_name == "Marine")
	{
		m_targetUID = attacker->m_UID;
	}
}

Actor* AIController::GetActorWithinSight(float radius, float angle)
{
	Actor* self = m_actorUID.GetActor();

	Actor* target = nullptr;

	for (size_t index = 0; index < m_map->m_actorList.size(); index++)
	{
		if (m_map->m_actorList[index])
		{
			if (m_map->m_actorList[index]->m_definition.m_name == "Marine")
			{
				if (IsPointInsideOrientedSector2D(Vec2(m_map->m_actorList[index]->m_position.x, m_map->m_actorList[index]->m_position.y), Vec2(self->m_position.x, self->m_position.y), self->m_orientation.GetYaw(), angle, radius))
				{
					target = m_map->m_actorList[index];

					m_targetUID = target->m_UID;
				}
			}
		}
	}

	return target;
}

bool AIController::IsTargetInAttackRange()
{
	Actor* self = m_actorUID.GetActor();

	for (size_t index = 0; index < m_map->m_actorList.size(); index++)
	{
		if (m_map->m_actorList[index])
		{
			if (m_map->m_actorList[index]->m_definition.m_name == "Marine")
			{
				if (IsPointInsideOrientedSector2D(Vec2(m_map->m_actorList[index]->m_position.x, m_map->m_actorList[index]->m_position.y), Vec2(self->m_position.x, self->m_position.y), self->m_orientation.GetYaw(), m_meleeWeapon->m_definition.m_meleeArc, m_meleeWeapon->m_definition.m_meleeRange))
				{
					return true;
				}
			}
		}
	}

	return false;
}
