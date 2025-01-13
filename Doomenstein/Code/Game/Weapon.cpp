#include "Game/Weapon.hpp"

#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

#include "Game/AIController.hpp"
#include "Game/PlayerController.hpp"
#include "Game/Actor.hpp"
#include "Game/Game.hpp"

WeaponDefinition WeaponDefinition::s_weaponDefinitions[3];

void WeaponDefinition::InitializeDefs()
{
	XmlDocument tileDoc;

	XmlError result = tileDoc.LoadFile("Data/Definitions/WeaponDefinitions.xml");

	if (result != tinyxml2::XML_SUCCESS)
		GUARANTEE_OR_DIE(false, "COULD NOT LOAD XML");

	XmlElement* element = tileDoc.RootElement()->FirstChildElement();

	for (int index = 0; index < 3; index++)
	{
		XmlElement* hudElement = nullptr;
		XmlElement* animElement = nullptr;
		XmlElement* soundsElement = nullptr;

		s_weaponDefinitions[index].m_name					= ParseXmlAttribute(*element, "name", s_weaponDefinitions[index].m_name);
		s_weaponDefinitions[index].m_refireTime				= ParseXmlAttribute(*element, "refireTime", s_weaponDefinitions[index].m_refireTime);
		
		if (index == 0)
		{
			s_weaponDefinitions[index].m_rayCount			= ParseXmlAttribute(*element, "rayCount", s_weaponDefinitions[index].m_rayCount);
			s_weaponDefinitions[index].m_rayCone			= ParseXmlAttribute(*element, "rayCone", s_weaponDefinitions[index].m_rayCone);
			s_weaponDefinitions[index].m_rayRange			= ParseXmlAttribute(*element, "rayRange", s_weaponDefinitions[index].m_rayRange);
			s_weaponDefinitions[index].m_rayDamage			= ParseXmlAttribute(*element, "rayDamage", s_weaponDefinitions[index].m_rayDamage);
			s_weaponDefinitions[index].m_rayImpulse			= ParseXmlAttribute(*element, "rayImpulse", s_weaponDefinitions[index].m_rayImpulse);
		}
		else if (index == 1)
		{
			s_weaponDefinitions[index].m_projectileCount	= ParseXmlAttribute(*element, "projectileCount", s_weaponDefinitions[index].m_projectileCount);
			s_weaponDefinitions[index].m_projectileActor	= ParseXmlAttribute(*element, "projectileActor", s_weaponDefinitions[index].m_projectileActor);
			s_weaponDefinitions[index].m_projectileCone		= ParseXmlAttribute(*element, "projectileCone", s_weaponDefinitions[index].m_projectileCone);
			s_weaponDefinitions[index].m_projectileSpeed	= ParseXmlAttribute(*element, "projectileSpeed", s_weaponDefinitions[index].m_projectileSpeed);
		}
		else if (index == 2)
		{
			s_weaponDefinitions[index].m_meleeCount			= ParseXmlAttribute(*element, "meleeCount", s_weaponDefinitions[index].m_meleeCount);
			s_weaponDefinitions[index].m_meleeArc			= ParseXmlAttribute(*element, "meleeArc", s_weaponDefinitions[index].m_meleeArc);
			s_weaponDefinitions[index].m_meleeRange			= ParseXmlAttribute(*element, "meleeRange", s_weaponDefinitions[index].m_meleeRange);
			s_weaponDefinitions[index].m_meleeDamage		= ParseXmlAttribute(*element, "meleeDamage", s_weaponDefinitions[index].m_meleeDamage);
			s_weaponDefinitions[index].m_meleeImpulse		= ParseXmlAttribute(*element, "meleeImpulse", s_weaponDefinitions[index].m_meleeImpulse);
		}

		if (element->FirstChildElement())
		{
			std::string name = std::string(element->FirstChildElement()->Name());

			if (name == "HUD")
			{
				hudElement = element->FirstChildElement();
			}
			else if (name == "Animation")
			{
				animElement = element->FirstChildElement();
			}
			else if (name == "Sounds")
			{
				soundsElement = element->FirstChildElement();
			}
		}

		if (hudElement)
		{
			s_weaponDefinitions[index].m_shader			= ParseXmlAttribute(*hudElement, "shader", s_weaponDefinitions[index].m_shader);
			s_weaponDefinitions[index].m_baseTexture	= ParseXmlAttribute(*hudElement, "baseTexture", s_weaponDefinitions[index].m_baseTexture);
			s_weaponDefinitions[index].m_reticleTexture	= ParseXmlAttribute(*hudElement, "reticleTexture", s_weaponDefinitions[index].m_reticleTexture);
			s_weaponDefinitions[index].m_reticleSize	= ParseXmlAttribute(*hudElement, "reticleSize", s_weaponDefinitions[index].m_reticleSize);
			s_weaponDefinitions[index].m_spriteSize		= ParseXmlAttribute(*hudElement, "spriteSize", s_weaponDefinitions[index].m_spriteSize);
			s_weaponDefinitions[index].m_spritePivot	= ParseXmlAttribute(*hudElement, "spritePivot", s_weaponDefinitions[index].m_spritePivot);

			if (hudElement->FirstChildElement())
			{
				std::string name = std::string(hudElement->FirstChildElement()->Name());

				if (name == "Animation")
				{
					animElement = hudElement->FirstChildElement();
				}

				int animElementIndex = 0;

				while (animElement)
				{
					s_weaponDefinitions[index].m_animName[animElementIndex] = ParseXmlAttribute(*animElement, "name", s_weaponDefinitions[index].m_animName[animElementIndex]);
					s_weaponDefinitions[index].m_animShader[animElementIndex] = ParseXmlAttribute(*animElement, "shader", s_weaponDefinitions[index].m_animShader[animElementIndex]);
					s_weaponDefinitions[index].m_spriteSheet[animElementIndex] = ParseXmlAttribute(*animElement, "spriteSheet", s_weaponDefinitions[index].m_spriteSheet[animElementIndex]);
					s_weaponDefinitions[index].m_cellCount[animElementIndex] = ParseXmlAttribute(*animElement, "cellCount", s_weaponDefinitions[index].m_cellCount[animElementIndex]);
					s_weaponDefinitions[index].m_secondsPerFrame[animElementIndex] = ParseXmlAttribute(*animElement, "secondsPerFrame", s_weaponDefinitions[index].m_secondsPerFrame[animElementIndex]);
					s_weaponDefinitions[index].m_startFrame[animElementIndex] = ParseXmlAttribute(*animElement, "startFrame", s_weaponDefinitions[index].m_startFrame[animElementIndex]);
					s_weaponDefinitions[index].m_endFrame[animElementIndex] = ParseXmlAttribute(*animElement, "endFrame", s_weaponDefinitions[index].m_endFrame[animElementIndex]);

					Texture* weaponTexture = g_theRenderer->CreateOrGetTextureFromFile(s_weaponDefinitions[index].m_spriteSheet[animElementIndex].c_str());
					SpriteSheet* sprite = new SpriteSheet(*weaponTexture, s_weaponDefinitions[index].m_cellCount[animElementIndex]);
					SpriteAnimDefinition* spriteAnim = new SpriteAnimDefinition(*sprite, s_weaponDefinitions[index].m_startFrame[animElementIndex], s_weaponDefinitions[index].m_endFrame[animElementIndex], (s_weaponDefinitions[index].m_endFrame[animElementIndex] - s_weaponDefinitions[index].m_startFrame[animElementIndex] + 1) * s_weaponDefinitions[index].m_secondsPerFrame[animElementIndex]);

					s_weaponDefinitions[index].m_weaponAnimDef.push_back(spriteAnim);

					animElementIndex++;

					animElement = animElement->NextSiblingElement();
				}

				name = std::string(hudElement->NextSiblingElement()->Name());

				if (name == "Sounds")
				{
					soundsElement = hudElement->NextSiblingElement();
				}
			}
		}

		if (soundsElement)
		{
			XmlElement* soundElement = soundsElement->FirstChildElement();

			s_weaponDefinitions[index].m_sound			= ParseXmlAttribute(*soundElement, "sound", s_weaponDefinitions[index].m_sound);
			s_weaponDefinitions[index].m_audioName		= ParseXmlAttribute(*soundElement, "name", s_weaponDefinitions[index].m_audioName);
		}

		element = element->NextSiblingElement();
	}
}

Weapon::Weapon()
{

}

Weapon::Weapon(WeaponDefinition definition, Actor* owner)
{
	m_definition = definition;
	m_owner = owner;
}

Weapon::~Weapon()
{
}

void Weapon::Fire()
{
	if (m_definition.m_name == WeaponDefinition::s_weaponDefinitions[0].m_name)
	{
		m_rayFireCast = m_owner->m_map->RaycastAll(Vec3(m_owner->m_position.x, m_owner->m_position.y, m_owner->m_position.z + m_owner->m_definition.m_eyeHeight), m_owner->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), m_definition.m_rayRange);	
		
		m_owner->m_map->m_game->m_allSoundPlaybackIDs[GAME_PISTOL_FIRE] = g_theAudio->StartSoundAt(m_owner->m_map->m_game->m_allSoundIDs[GAME_PISTOL_FIRE], m_owner->m_position);

		if (m_rayFireCast.m_impactedActor)
		{
			//m_rayFireCast.m_impactedActor->m_animName = "Hurt";
			m_owner->m_controller->m_isPistolHit = m_rayFireCast.m_raycast.m_didImpact;
		}
	}
	else if (m_definition.m_name == WeaponDefinition::s_weaponDefinitions[1].m_name)
	{
		//m_owner->m_map->m_game->m_allSoundPlaybackIDs[GAME_PLASMA_FIRE] = g_theAudio->StartSoundAt(m_owner->m_map->m_game->m_allSoundIDs[GAME_PLASMA_FIRE], m_owner->m_position);

		//m_owner->m_map->SpawnProjectile();
	}   
	else if (m_definition.m_name == WeaponDefinition::s_weaponDefinitions[2].m_name)
	{
		RandomNumberGenerator random = RandomNumberGenerator();
		Actor* target = m_owner->m_aiController->m_targetUID.GetActor();
		
		m_owner->m_map->m_game->m_allSoundPlaybackIDs[GAME_DEMON_ATTACK] = g_theAudio->StartSoundAt(m_owner->m_map->m_game->m_allSoundIDs[GAME_DEMON_ATTACK], target->m_position);
		m_owner->m_map->m_game->m_allSoundPlaybackIDs[GAME_PLAYER_HURT] = g_theAudio->StartSoundAt(m_owner->m_map->m_game->m_allSoundIDs[GAME_PLAYER_HURT], target->m_position);
		target->Damage(random.RollRandomFloatInRange(5.0f, 10.0f));
	}
}

Vec3 Weapon::GetRandomDirectionInCone() const
{
	Vec3 projectileDirection;
	EulerAngles orientation;

	RandomNumberGenerator random = RandomNumberGenerator();

	float projectileYawOffset = random.RollRandomFloatInRange(-m_definition.m_projectileCone, m_definition.m_projectileCone);
	float projectilePitchOffset = random.RollRandomFloatInRange(-m_definition.m_projectileCone, m_definition.m_projectileCone);

	orientation.m_yawDegrees = m_owner->m_orientation.m_yawDegrees + projectileYawOffset;
	orientation.m_pitchDegrees = m_owner->m_orientation.m_pitchDegrees + projectilePitchOffset;

	projectileDirection = orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D();

	return projectileDirection;
}
