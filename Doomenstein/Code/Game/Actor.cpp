#include "Game/Actor.hpp"

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Controller.hpp"
#include "Game/PlayerController.hpp"
#include "Game/AIController.hpp"
#include "Game/Weapon.hpp"

#include<vector>

ActorDefinition ActorDefinition::s_actorDefinitions[7];

Actor::Actor()
{
}

void Actor::PlayAnimation(Camera cameraPosition)
{
	for (int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
	{
		if (m_map->m_game->m_playerController[i] != nullptr)
		{
			Vec3 direction = (m_position - cameraPosition.m_position).GetNormalized();
			int desiredDirIndex = 0;
			float maxDot = 0.0f;
	
			Mat44 transform = GetModelMatrix().GetOrthonormalInverse();
	
			Vec3 localDir = transform.TransformVectorQuantity3D(direction) * -1.0f;
	
			for (size_t j = 0; j < m_currentAnimGrp->m_spriteDirection.size(); j++)
			{
				if (DotProduct3D(localDir.GetNormalized(), m_currentAnimGrp->m_spriteDirection[j]) > maxDot)
				{
					maxDot = DotProduct3D(localDir.GetNormalized(), m_currentAnimGrp->m_spriteDirection[j]);
					desiredDirIndex = (int)j;
				}
			}
	
			m_animDuration = m_currentAnimGrp->m_spriteAnimDefs[desiredDirIndex]->GetDuration();
	
			if (m_isActorProjectile)
			{
				if (!m_isProjectileDead)
				{
					if (m_animTime >= m_animDuration)
						m_animTime = 0.0f;
				}
			}
			else
			{
				if (!m_isActorCorpse)
				{
					if (m_animTime >= m_animDuration)
						m_animTime = 0.0f;
				}
			}
		}
	}
}

void Actor::RenderAnimation(Camera cameraPosition) const
{
	Vec3 direction = (cameraPosition.m_position - m_position).GetNormalized();
	Vec2 uvMins, uvMaxs;
	//int desiredDirIndex = 0;
	//float maxDot = -1.0f;

	//Mat44 transformDir = GetModelMatrix().GetOrthonormalInverse();
	//
	//Vec3 localDir = transformDir.TransformVectorQuantity3D(direction) * -1.0f;
	//
	//for (size_t j = 0; j < m_currentAnimGrp->m_spriteDirection.size(); j++)
	//{
	//	if (DotProduct3D(localDir.GetNormalized(), m_currentAnimGrp->m_spriteDirection[j]) > maxDot)
	//	{
	//		maxDot = DotProduct3D(localDir.GetNormalized(), m_currentAnimGrp->m_spriteDirection[j]);
	//		desiredDirIndex = (int)j;
	//	}
	//}
	//
	//m_currentAnimGrp->m_spriteAnimDefs[desiredDirIndex]->GetSpriteDefAtTime(m_animTime).GetUVs(uvMins, uvMaxs);
	//
	//if (m_definition.m_name == "Marine" || m_definition.m_name == "Demon")
	//{
	//	Vec2 tempUVMin = uvMins;
	//	Vec2 tempUVMax = uvMaxs;
	//
	//	uvMins.y = 1.0f - tempUVMax.y;
	//	uvMaxs.y = 1.0f - tempUVMin.y;
	//}
	
	std::vector<Vertex_PCUTBN> actorVerts;
	std::vector<unsigned int> actorIndices;

	AddQuadVertsAndIndices(actorVerts, actorIndices);

	Mat44 transform;

	if (m_isActorProjectile || m_isActorEffect)
	{
		if (m_definition.m_name == "BulletHit")
		{
			transform = GetBillboardMatrix(BillboardType::FULL_CAMERA_OPPOSING, cameraPosition.GetModelMatrix(), Vec3(m_position.x, m_position.y, m_position.z - (0.1f)));
		}
		else if (m_definition.m_name == "PlasmaProjectile")
		{
			transform = GetBillboardMatrix(BillboardType::FULL_CAMERA_OPPOSING, cameraPosition.GetModelMatrix(), Vec3(m_position.x, m_position.y, m_position.z));
		}
		else
		{
			transform = GetBillboardMatrix(BillboardType::FULL_CAMERA_OPPOSING, cameraPosition.GetModelMatrix(), Vec3(m_position.x, m_position.y, m_position.z - (0.2f)));
		}
	}
	else
	{
		transform = GetBillboardMatrix(BillboardType::WORLD_UP_CAMERA_FACING, cameraPosition.GetModelMatrix(), m_position);
	}

	g_theRenderer->SetModelConstants(transform);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->BindShader(m_shader);

	if (!m_isWeak)
	{
		g_theRenderer->BindTexture(&m_spriteSheet->GetTexture());
	}
	else
	{
		Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Textures/WeakEnemy.png");
		g_theRenderer->BindTexture(texture);
	}

	g_theRenderer->DrawVertexArrayIndexed((int)actorVerts.size(), actorVerts.data(), actorIndices);
}

Actor::Actor(ActorDefinition definition, Map* owner, Vec3 position, EulerAngles orientation, Rgba8 color)
{
	m_definition = definition;
	m_position = position;
	m_orientation = orientation;
	m_color = color;
	m_physicsHeight = m_definition.m_height;
	m_physicsRadius = m_definition.m_radius;
	m_health = m_definition.m_health;
	m_isMovable = m_definition.m_simulated;
	m_map = owner;

	for (int index = 0; index < 3; index++)
	{
		Weapon* weapon = new Weapon(WeaponDefinition::s_weaponDefinitions[index], this);
		m_weapons.push_back(weapon);
	}
	
	if (m_definition.m_name == "PlasmaProjectile")
	{
		for (int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
		{
			if (m_map->m_game->m_playerController[i] != nullptr && m_map->m_game->m_playerController[i]->m_actorUID != ActorUID::INVALID)
			{ 
				m_projectileOwner = m_map->m_game->m_playerController[i]->m_actorUID.GetActor();
				m_isActorProjectile = true;
				m_accelaration = 10.0f * m_weapons[m_projectileOwner->m_controller->m_equippedWeaponIndex]->GetRandomDirectionInCone();
			}
		}
	}

	if (m_definition.m_name == "BulletHit" || m_definition.m_name == "BloodSplatter")
	{
		m_isActorEffect = true;
	}

	if(!m_definition.m_shader.empty())
		m_shader = g_theRenderer->CreateShader(m_definition.m_shader.c_str(), VertexType::PCUTBN);

	if (!m_definition.m_spriteSheet.empty())
	{
		Texture* texture = g_theRenderer->CreateOrGetTextureFromFile(m_definition.m_spriteSheet.c_str());
		m_spriteSheet = new SpriteSheet(*texture, m_definition.m_cellCount);
	}

	if (m_definition.m_name != "SpawnPoint" && m_definition.m_name != "PlasmaProjectile")
	{
		//m_currentAnimGrp = &m_definition.m_spriteAnimGrpDefs[0];
		//m_animDuration = m_currentAnimGrp->m_spriteAnimDefs[0]->GetDuration();
	}

	if (m_definition.m_name == "PlasmaProjectile")
	{
		m_currentAnimGrp = &m_definition.m_spriteAnimGrpDefs[0];
		m_animDuration = m_currentAnimGrp->m_spriteAnimDefs[0]->GetDuration();
	}

	m_quadVBO = g_theRenderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN));
	m_quadIBO = g_theRenderer->CreateIndexBuffer(sizeof(unsigned int));
}

Actor::~Actor()
{
	DELETE_PTR(m_quadVBO);
	DELETE_PTR(m_quadIBO);
	DELETE_PTR(m_shader);
	DELETE_PTR(m_spriteSheet);
}

void Actor::Update(float deltaseconds, Camera cameraPosition)
{
	if (m_isActorProjectile)
	{
		if (!m_isDead)
		{
			if (m_isProjectileDead)
			{
				m_animName = "Death";

				static int flag = 0;

				if (flag == 0)
				{
					m_map->m_game->m_allSoundPlaybackIDs[GAME_PLASMA_HIT] = g_theAudio->StartSoundAt(m_map->m_game->m_allSoundIDs[GAME_PLASMA_HIT], m_position);
					flag++;
				}

				m_currentAnimGrp = &m_definition.m_spriteAnimGrpDefs[1];

				if (m_animTime >= m_currentAnimGrp->m_spriteAnimDefs[0]->GetDuration())
				{
					m_isDead = true;
					flag = 0;
				}

				if (g_theAudio->IsPlaying(m_map->m_game->m_allSoundPlaybackIDs[GAME_PLASMA_HIT]))
				{
					g_theAudio->SetSoundPosition(m_map->m_game->m_allSoundPlaybackIDs[GAME_PLASMA_HIT], m_position);

					for (int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
					{
						if (m_map->m_game->m_playerController[i] != nullptr)
						{
							g_theAudio->UpdateListener(i, m_map->m_game->m_playerController[i]->m_position, m_map->m_game->m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), m_map->m_game->m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
						}
					}
				}

				m_animTime += deltaseconds;
			}
			else
			{
				m_animTime += deltaseconds;

				m_velocity += m_accelaration * deltaseconds;
				m_position += m_velocity * deltaseconds;
			}

			PlayAnimation(cameraPosition);
		}
	}
	else if(m_definition.m_name == "Marine" || m_definition.m_name == "RedGhost" || m_definition.m_name == "GreenGhost" || m_definition.m_name == "BlueGhost")
	{
		if (m_controller)
		{
			if (!m_isDead)
			{
				if (m_health <= 0 && !m_isActorCorpse)
				{
					m_isActorCorpse = true;
					m_map->m_game->m_allSoundPlaybackIDs[GAME_PLAYER_DEATH] = g_theAudio->StartSoundAt(m_map->m_game->m_allSoundIDs[GAME_PLAYER_DEATH], m_position);
				}

				if (m_isActorCorpse)
				{
					if (g_theAudio->IsPlaying(m_map->m_game->m_allSoundPlaybackIDs[GAME_PLAYER_DEATH]))
					{
						g_theAudio->SetSoundPosition(m_map->m_game->m_allSoundPlaybackIDs[GAME_PLAYER_DEATH], m_position);

						for (int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
						{
							if (m_map->m_game->m_playerController[i] != nullptr)
							{
								g_theAudio->UpdateListener(i, m_map->m_game->m_playerController[i]->m_position, m_map->m_game->m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), m_map->m_game->m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
							}
						}
					}

					//m_animName = "Death";

					//m_currentAnimGrp = &m_definition.m_spriteAnimGrpDefs[3];

					//if (m_controller->GetActor()->m_animTime >= m_currentAnimGrp->m_spriteAnimDefs[0]->GetDuration())
					{
						m_isDead = true;
					}

					//m_controller->GetActor()->m_animTime += deltaseconds;
				}
				else
				{
					//m_controller->GetActor()->m_animTime += deltaseconds;

					UpdatePhysics(deltaseconds);

					//if (m_controller->m_velocity.GetLengthSquared() <= 0.001f)
					//	m_animTime = 0.0f;
					//
					//for (size_t i = 0; i < m_definition.m_spriteAnimGrpDefs.size(); i++)
					//{
					//	if (m_animName == m_definition.m_spriteAnimGrpDefs[i].m_name)
					//	{
					//		m_currentAnimGrp = &m_definition.m_spriteAnimGrpDefs[i];
					//		break;
					//	}
					//}
				}

				//PlayAnimation(cameraPosition);
			}
		}
		else if (m_aiController)
		{
			if (!m_isDead)
			{
				if (m_health <= 0 && !m_isActorCorpse)
				{
					m_isActorCorpse = true;
					m_map->m_game->m_allSoundPlaybackIDs[GAME_DEMON_DEATH] = g_theAudio->StartSoundAt(m_map->m_game->m_allSoundIDs[GAME_DEMON_DEATH], m_position);
				}

				if (m_isActorCorpse)
				{
					//if (g_theAudio->IsPlaying(m_map->m_game->m_allSoundPlaybackIDs[GAME_DEMON_DEATH]))
					//{
					//	g_theAudio->SetSoundPosition(m_map->m_game->m_allSoundPlaybackIDs[GAME_DEMON_DEATH], m_position);
					//
					//	for (int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
					//	{
					//		if (m_map->m_game->m_playerController[i] != nullptr)
					//		{
					//			g_theAudio->UpdateListener(i, m_map->m_game->m_playerController[i]->m_position, m_map->m_game->m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), m_map->m_game->m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
					//		}
					//	}
					//}
					//
					//m_animName = "Death";
					//
					//m_currentAnimGrp = &m_definition.m_spriteAnimGrpDefs[3];

					//if (m_aiController->GetActor()->m_animTime >= m_currentAnimGrp->m_spriteAnimDefs[0]->GetDuration())
					{
						m_isDead = true;
					}

					//m_aiController->GetActor()->m_animTime += deltaseconds;
				}
				else
				{
					//m_aiController->GetActor()->m_animTime += deltaseconds;

					m_aiController->Update(deltaseconds);

					UpdatePhysics(deltaseconds);

					//if (m_aiController->m_targetUID == ActorUID::INVALID)
					//	m_animTime = 0.0f;
					//
					//for (size_t i = 0; i < m_definition.m_spriteAnimGrpDefs.size(); i++)
					//{
					//	if (m_animName == m_definition.m_spriteAnimGrpDefs[i].m_name)
					//	{
					//		m_currentAnimGrp = &m_definition.m_spriteAnimGrpDefs[i];
					//		break;
					//	}
					//}
				}

				//PlayAnimation(cameraPosition);
			}
		}
	}
	//else if (m_definition.m_name == "BulletHit" || m_definition.m_name == "BloodSplatter")
	//{
	//    if (!m_isDead)
	//    {
	//		m_animName = "Death";
	//
	//		m_animTime += deltaseconds;
	//
	//		if(m_animTime > m_animDuration)
	//			m_isDead = true;
	//
	//		PlayAnimation(cameraPosition);
	//	}
	//}
}

void Actor::Render(Camera cameraPosition) const
{
	if (!m_isDead)
	{
		if (m_definition.m_name != "SpawnPoint")
		{
			RenderBillBoardQuad(cameraPosition);
		}
	}
}

void Actor::AddQuadVertsAndIndices(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indices, AABB2 const& uvs) const
{
	Vec2 dims = Vec2(m_definition.m_size.x * 0.5f, m_definition.m_size.y);

	Vec3 a = Vec3(0.0f, -dims.x, 0.0f);
	Vec3 b = Vec3::ZERO;
	Vec3 c = Vec3(0.0f, dims.x, 0.0f);
	Vec3 d = Vec3(0.0f, dims.x, dims.y);
	Vec3 e = Vec3(0.0f, 0.0f, dims.y);
	Vec3 f = Vec3(0.0f, -dims.x, dims.y);

	Vec3 leftNormal = (a - b).GetNormalized();
	Vec3 rightNormal = -1.0f * leftNormal;
	Vec3 centerNormal = CrossProduct3D((e - b).GetNormalized(), leftNormal).GetNormalized();

	verts.push_back(Vertex_PCUTBN(a, Rgba8::WHITE, uvs.m_mins,													Vec3::ZERO, Vec3::ZERO, leftNormal));
	verts.push_back(Vertex_PCUTBN(b, Rgba8::WHITE, Vec2((uvs.m_maxs.x + uvs.m_mins.x) * 0.5f, uvs.m_mins.y),	Vec3::ZERO, Vec3::ZERO, centerNormal));
	verts.push_back(Vertex_PCUTBN(c, Rgba8::WHITE, Vec2(uvs.m_maxs.x, uvs.m_mins.y),							Vec3::ZERO, Vec3::ZERO, rightNormal));
	verts.push_back(Vertex_PCUTBN(d, Rgba8::WHITE, uvs.m_maxs,													Vec3::ZERO, Vec3::ZERO, rightNormal));
	verts.push_back(Vertex_PCUTBN(e, Rgba8::WHITE, Vec2((uvs.m_maxs.x + uvs.m_mins.x) * 0.5f, uvs.m_maxs.y),	Vec3::ZERO, Vec3::ZERO, centerNormal));
	verts.push_back(Vertex_PCUTBN(f, Rgba8::WHITE, Vec2(uvs.m_mins.x, uvs.m_maxs.y),							Vec3::ZERO, Vec3::ZERO, leftNormal));

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(4);
	indices.push_back(4);
	indices.push_back(5);
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(3);
	indices.push_back(4);
	indices.push_back(1);
}

void Actor::RenderBillBoardQuad(Camera cameraPosition) const
{
	if (this != nullptr)
	{
		for (int i = 0; i < m_map->m_game->m_numOfPlayers; i++)
		{
			if ((m_map->m_game->m_playerController[i]))
			{
				RenderAnimation(cameraPosition);
			}
		}
	}
}

Mat44 Actor::GetModelMatrix() const
{
	Mat44 modelmatrix;

	modelmatrix.SetTranslation3D(m_position);
	modelmatrix.Append(m_orientation.GetAsMatrix_XFwd_YLeft_ZUp());

	return modelmatrix;
}

bool Actor::IsMovable()
{
	return m_isMovable;
}

void Actor::SetVelocity(Vec3 velocity)
{
	m_velocity = velocity;
}

void Actor::SetStatic(bool movable)
{
	m_isMovable = movable;
}

void Actor::ToggleStatic()
{
	m_isMovable = !m_isMovable;
}

void ActorDefinition::InitializeDefs()
{
	XmlDocument tileDoc;

	XmlError result = tileDoc.LoadFile("Data/Definitions/ActorDefinitions.xml");

	if (result != tinyxml2::XML_SUCCESS)
		GUARANTEE_OR_DIE(false, "COULD NOT LOAD XML");

	XmlElement* element = tileDoc.RootElement()->FirstChildElement();

	for (int index = 0; index < 7; index++)
	{
		XmlElement* collisionElement = nullptr;
		XmlElement* physicsElement = nullptr;
		XmlElement* cameraElement = nullptr;
		XmlElement* aiElement = nullptr;
		XmlElement* visualsElement = nullptr;
		XmlElement* soundsElement = nullptr;
		XmlElement* inventoryElement = nullptr;

		s_actorDefinitions[index].m_name = ParseXmlAttribute(*element, "name", s_actorDefinitions[index].m_name);
		s_actorDefinitions[index].m_faction = ParseXmlAttribute(*element, "faction", s_actorDefinitions[index].m_faction);
		s_actorDefinitions[index].m_health = (float)ParseXmlAttribute(*element, "health", 0);
		s_actorDefinitions[index].m_canBePossessed = ParseXmlAttribute(*element, "canBePossessed", s_actorDefinitions[index].m_canBePossessed);
		s_actorDefinitions[index].m_corpseLifetime = ParseXmlAttribute(*element, "corpseLifetime", s_actorDefinitions[index].m_corpseLifetime);
		s_actorDefinitions[index].m_visible = ParseXmlAttribute(*element, "visible", s_actorDefinitions[index].m_visible);

		if (element->FirstChildElement())
		{
			std::string name = std::string(element->FirstChildElement()->Name());

			if (name == "Collision")
			{
				collisionElement = element->FirstChildElement();
			}
			else if (name == "Physics")
			{
				physicsElement = element->FirstChildElement();
			}
			else if (name == "Camera")
			{
				cameraElement = element->FirstChildElement();
			}
			else if (name == "AI")
			{
				aiElement = element->FirstChildElement();
			}
			else if (name == "Visuals")
			{
				visualsElement = element->FirstChildElement();
			}
			else if (name == "Sounds")
			{
				soundsElement = element->FirstChildElement();
			}
			else if (name == "Inventory")
			{
				inventoryElement = element->FirstChildElement();
			}
		}

		if (collisionElement)
		{
			s_actorDefinitions[index].m_radius = ParseXmlAttribute(*collisionElement, "radius", s_actorDefinitions[index].m_radius);
			s_actorDefinitions[index].m_height = ParseXmlAttribute(*collisionElement, "height", s_actorDefinitions[index].m_height);
			s_actorDefinitions[index].m_collidesWithWorld = ParseXmlAttribute(*collisionElement, "collidesWithWorld", s_actorDefinitions[index].m_collidesWithWorld);
			s_actorDefinitions[index].m_collidesWithActors = ParseXmlAttribute(*collisionElement, "collidesWithActors", s_actorDefinitions[index].m_collidesWithActors);

			if (collisionElement->NextSiblingElement())
			{
				std::string name = std::string(collisionElement->NextSiblingElement()->Name());

				if (name == "Physics")
				{
					physicsElement = collisionElement->NextSiblingElement();
				}
				else if (name == "Camera")
				{
					cameraElement = collisionElement->NextSiblingElement();
				}
				else if (name == "AI")
				{
					aiElement = collisionElement->NextSiblingElement();
				}
				else if (name == "Visuals")
				{
					visualsElement = collisionElement->NextSiblingElement();
				}
				else if (name == "Sounds")
				{
					soundsElement = collisionElement->NextSiblingElement();
				}
				else if (name == "Inventory")
				{
					inventoryElement = collisionElement->NextSiblingElement();
				}
			}
		}

		if (physicsElement)
		{
			s_actorDefinitions[index].m_simulated = ParseXmlAttribute(*physicsElement, "simulated", s_actorDefinitions[index].m_simulated);
			s_actorDefinitions[index].m_walkSpeed = ParseXmlAttribute(*physicsElement, "walkSpeed", s_actorDefinitions[index].m_walkSpeed);
			s_actorDefinitions[index].m_runSpeed = ParseXmlAttribute(*physicsElement, "runSpeed", s_actorDefinitions[index].m_runSpeed);
			s_actorDefinitions[index].m_turnSpeed = ParseXmlAttribute(*physicsElement, "turnSpeed", s_actorDefinitions[index].m_turnSpeed);
			s_actorDefinitions[index].m_drag = ParseXmlAttribute(*physicsElement, "drag", s_actorDefinitions[index].m_drag);

			if (physicsElement->NextSiblingElement())
			{
				std::string name = std::string(physicsElement->NextSiblingElement()->Name());

				if (name == "Camera")
				{
					cameraElement = physicsElement->NextSiblingElement();
				}
				else if (name == "AI")
				{
					aiElement = physicsElement->NextSiblingElement();
				}
				else if (name == "Visuals")
				{
					visualsElement = physicsElement->NextSiblingElement();
				}
				else if (name == "Sounds")
				{
					soundsElement = physicsElement->NextSiblingElement();
				}
				else if (name == "Inventory")
				{
					inventoryElement = physicsElement->NextSiblingElement();
				}
			}
		}

		if (cameraElement)
		{
			s_actorDefinitions[index].m_eyeHeight = ParseXmlAttribute(*cameraElement, "eyeHeight", s_actorDefinitions[index].m_eyeHeight);
			s_actorDefinitions[index].m_cameraFOV = ParseXmlAttribute(*cameraElement, "cameraFOV", s_actorDefinitions[index].m_cameraFOV);

			if (cameraElement->NextSiblingElement())
			{
				std::string name = std::string(cameraElement->NextSiblingElement()->Name());

				if (name == "AI")
				{
					aiElement = cameraElement->NextSiblingElement();
				}
				else if (name == "Visuals")
				{
					visualsElement = cameraElement->NextSiblingElement();
				}
				else if (name == "Sounds")
				{
					soundsElement = cameraElement->NextSiblingElement();
				}
				else if (name == "Inventory")
				{
					inventoryElement = cameraElement->NextSiblingElement();
				}
			}
		}

		if (aiElement)
		{
			s_actorDefinitions[index].m_aiEnabled = ParseXmlAttribute(*aiElement, "aiEnabled", s_actorDefinitions[index].m_aiEnabled);
			s_actorDefinitions[index].m_sightRadius = ParseXmlAttribute(*aiElement, "sightRadius", s_actorDefinitions[index].m_sightRadius);
			s_actorDefinitions[index].m_sightAngle = ParseXmlAttribute(*aiElement, "sightAngle", s_actorDefinitions[index].m_sightAngle);

			if (aiElement->NextSiblingElement())
			{
				std::string name = std::string(aiElement->NextSiblingElement()->Name());

				if (name == "Visuals")
				{
					visualsElement = aiElement->NextSiblingElement();
				}
				else if (name == "Sounds")
				{
					soundsElement = aiElement->NextSiblingElement();
				}
				else if (name == "Inventory")
				{
					inventoryElement = aiElement->NextSiblingElement();
				}
			}
		}

		if (visualsElement)
		{
			s_actorDefinitions[index].m_size = ParseXmlAttribute(*visualsElement, "size", s_actorDefinitions[index].m_size);
			s_actorDefinitions[index].m_pivot = ParseXmlAttribute(*visualsElement, "pivot", s_actorDefinitions[index].m_pivot);
			s_actorDefinitions[index].m_billboardType = ParseXmlAttribute(*visualsElement, "billboardType", s_actorDefinitions[index].m_billboardType);
			s_actorDefinitions[index].m_renderLit = ParseXmlAttribute(*visualsElement, "renderLit", s_actorDefinitions[index].m_renderLit);
			s_actorDefinitions[index].m_renderRounded = ParseXmlAttribute(*visualsElement, "renderRounded", s_actorDefinitions[index].m_renderRounded);
			s_actorDefinitions[index].m_shader = ParseXmlAttribute(*visualsElement, "shader", s_actorDefinitions[index].m_shader);
			s_actorDefinitions[index].m_spriteSheet = ParseXmlAttribute(*visualsElement, "spriteSheet", s_actorDefinitions[index].m_spriteSheet);
			s_actorDefinitions[index].m_cellCount = ParseXmlAttribute(*visualsElement, "cellCount", s_actorDefinitions[index].m_cellCount);

			XmlElement* animGrp = visualsElement->FirstChildElement();

			while (animGrp)
			{
				std::string name = std::string(animGrp->Name());
				SpriteAnimGroupDefinition* spriteAnimGrp = new SpriteAnimGroupDefinition();

				if (name == "AnimationGroup")
				{
					s_actorDefinitions[index].m_animGrpName		= ParseXmlAttribute(*animGrp, "name", s_actorDefinitions[index].m_animGrpName);
					s_actorDefinitions[index].m_scaleBySpeed = ParseXmlAttribute(*animGrp, "scaleBySpeed", s_actorDefinitions[index].m_scaleBySpeed);
					s_actorDefinitions[index].m_secondsPerFrame = ParseXmlAttribute(*animGrp, "secondsPerFrame", s_actorDefinitions[index].m_secondsPerFrame);
					s_actorDefinitions[index].m_playbackMode = ParseXmlAttribute(*animGrp, "playbackMode", s_actorDefinitions[index].m_playbackMode);

					spriteAnimGrp->m_name = s_actorDefinitions[index].m_animGrpName;
					spriteAnimGrp->m_scaleBySpeed = s_actorDefinitions[index].m_scaleBySpeed;
					spriteAnimGrp->m_secondsPerFrame = s_actorDefinitions[index].m_secondsPerFrame;
					spriteAnimGrp->m_playbackMode = s_actorDefinitions[index].m_playbackMode;

					XmlElement* animGrpDir = animGrp->FirstChildElement();

					while (animGrpDir)
					{
						name = std::string(animGrp->FirstChildElement()->Name());

						if (name == "Direction")
						{
							Vec3 direction = ParseXmlAttribute(*animGrpDir, "vector", Vec3::ZERO);

							spriteAnimGrp->m_spriteDirection.push_back(direction.GetNormalized());
						}

						if (animGrpDir->FirstChildElement())
						{
							name = std::string(animGrpDir->FirstChildElement()->Name());

							if (name == "Animation")
							{
								Texture* spriteTexture = g_theRenderer->CreateOrGetTextureFromFile(s_actorDefinitions[index].m_spriteSheet.c_str());
								SpriteSheet* spriteSheet = new SpriteSheet(*spriteTexture, s_actorDefinitions[index].m_cellCount);
								spriteSheet->CalculateUVsOfSpriteSheet();

								int startIndex = ParseXmlAttribute(*animGrpDir->FirstChildElement(), "startFrame", -1);
								int endIndex  = ParseXmlAttribute(*animGrpDir->FirstChildElement(), "endFrame", -1);

								float duration = (endIndex - startIndex + 1) * s_actorDefinitions[index].m_secondsPerFrame;

								SpriteAnimDefinition* spriteAnim = new SpriteAnimDefinition(*spriteSheet, startIndex, endIndex, duration);
								 
								spriteAnimGrp->m_spriteAnimDefs.push_back(spriteAnim);
							}
						}

						animGrpDir = animGrpDir->NextSiblingElement();
					}

					s_actorDefinitions[index].m_spriteAnimGrpDefs.push_back(*spriteAnimGrp);
				}

				animGrp = animGrp->NextSiblingElement();
			}

			if (visualsElement->NextSiblingElement())
			{
				std::string name = std::string(visualsElement->NextSiblingElement()->Name());

				if (name == "Sounds")
				{
					soundsElement = visualsElement->NextSiblingElement();
				}
				else if (name == "Inventory")
				{
					inventoryElement = visualsElement->NextSiblingElement();
				}
			}
		}

		if (soundsElement)
		{
			s_actorDefinitions[index].m_sound = ParseXmlAttribute(*soundsElement, "sound", s_actorDefinitions[index].m_sound);
			s_actorDefinitions[index].m_soundName = ParseXmlAttribute(*soundsElement, "name", s_actorDefinitions[index].m_soundName);

			if (soundsElement->NextSiblingElement())
			{
				std::string name = std::string(soundsElement->NextSiblingElement()->Name());

				if (name == "Inventory")
				{
					inventoryElement = soundsElement->NextSiblingElement();
				}
			}
		}

		if (inventoryElement)
		{
			s_actorDefinitions[index].m_weaponName = ParseXmlAttribute(*inventoryElement, "name", s_actorDefinitions[index].m_weaponName);
		}

		element = element->NextSiblingElement();
	}
}

ActorDefinition* ActorDefinition::GetDefByName(std::string actorName)
{
	for (int index = 0; index < 7; index++)
	{
		if (ActorDefinition::s_actorDefinitions[index].m_name == actorName)
		{
			return &ActorDefinition::s_actorDefinitions[index];
		}
	}

	return new ActorDefinition();  
}   

void ActorDefinition::InitializeProjectileDefs()
{
	XmlDocument tileDoc;

	XmlError result = tileDoc.LoadFile("Data/Definitions/ProjectileActorDefinitions.xml");

	if (result != tinyxml2::XML_SUCCESS)
		GUARANTEE_OR_DIE(false, "COULD NOT LOAD XML");

	XmlElement* element = tileDoc.RootElement()->FirstChildElement();

	XmlElement* collisionElement = element->FirstChildElement();
	XmlElement* physicsElement = collisionElement->NextSiblingElement();
	XmlElement* visualsElement = physicsElement->NextSiblingElement();

	s_actorDefinitions[5].m_name = ParseXmlAttribute(*element, "name", s_actorDefinitions[5].m_name);
	s_actorDefinitions[5].m_faction = ParseXmlAttribute(*element, "faction", s_actorDefinitions[5].m_faction);
	s_actorDefinitions[5].m_health = (float)ParseXmlAttribute(*element, "health", 0);
	s_actorDefinitions[5].m_canBePossessed = ParseXmlAttribute(*element, "canBePossessed", s_actorDefinitions[5].m_canBePossessed);
	s_actorDefinitions[5].m_corpseLifetime = ParseXmlAttribute(*element, "corpseLifetime", s_actorDefinitions[5].m_corpseLifetime);
	s_actorDefinitions[5].m_visible = ParseXmlAttribute(*element, "visible", s_actorDefinitions[5].m_visible);

	s_actorDefinitions[5].m_radius = ParseXmlAttribute(*collisionElement, "radius", s_actorDefinitions[5].m_radius);
	s_actorDefinitions[5].m_height = ParseXmlAttribute(*collisionElement, "height", s_actorDefinitions[5].m_height);
	s_actorDefinitions[5].m_collidesWithWorld = ParseXmlAttribute(*collisionElement, "collidesWithWorld", s_actorDefinitions[5].m_collidesWithWorld);
	s_actorDefinitions[5].m_collidesWithActors = ParseXmlAttribute(*collisionElement, "collidesWithActors", s_actorDefinitions[5].m_collidesWithActors);
	s_actorDefinitions[5].m_damageOnCollide = ParseXmlAttribute(*collisionElement, "damageOnCollide", s_actorDefinitions[5].m_damageOnCollide);
	s_actorDefinitions[5].m_impulseOnCollide = ParseXmlAttribute(*collisionElement, "impulseOnCollide", s_actorDefinitions[5].m_impulseOnCollide);
	s_actorDefinitions[5].m_dieOnCollide = ParseXmlAttribute(*collisionElement, "dieOnCollide", s_actorDefinitions[5].m_dieOnCollide);

	s_actorDefinitions[5].m_simulated = ParseXmlAttribute(*physicsElement, "simulated", s_actorDefinitions[5].m_simulated);
	s_actorDefinitions[5].m_turnSpeed = ParseXmlAttribute(*physicsElement, "turnSpeed", s_actorDefinitions[5].m_turnSpeed);
	s_actorDefinitions[5].m_flying = ParseXmlAttribute(*physicsElement, "flying", s_actorDefinitions[5].m_flying);
	s_actorDefinitions[5].m_drag = ParseXmlAttribute(*physicsElement, "drag", s_actorDefinitions[5].m_drag);

	s_actorDefinitions[5].m_size = ParseXmlAttribute(*visualsElement, "size", s_actorDefinitions[5].m_size);
	s_actorDefinitions[5].m_pivot = ParseXmlAttribute(*visualsElement, "pivot", s_actorDefinitions[5].m_pivot);
	s_actorDefinitions[5].m_billboardType = ParseXmlAttribute(*visualsElement, "billboardType", s_actorDefinitions[5].m_billboardType);
	s_actorDefinitions[5].m_renderLit = ParseXmlAttribute(*visualsElement, "renderLit", s_actorDefinitions[5].m_renderLit);
	s_actorDefinitions[5].m_renderRounded = ParseXmlAttribute(*visualsElement, "renderRounded", s_actorDefinitions[5].m_renderRounded);
	s_actorDefinitions[5].m_shader = ParseXmlAttribute(*visualsElement, "shader", s_actorDefinitions[5].m_shader);
	s_actorDefinitions[5].m_spriteSheet = ParseXmlAttribute(*visualsElement, "spriteSheet", s_actorDefinitions[5].m_spriteSheet);
	s_actorDefinitions[5].m_cellCount = ParseXmlAttribute(*visualsElement, "cellCount", s_actorDefinitions[5].m_cellCount);

	XmlElement* animGrp = visualsElement->FirstChildElement();

	while (animGrp)
	{
		std::string name = std::string(animGrp->Name());
		SpriteAnimGroupDefinition* spriteAnimGrp = new SpriteAnimGroupDefinition();

		if (name == "AnimationGroup")
		{
			s_actorDefinitions[5].m_animGrpName = ParseXmlAttribute(*animGrp, "name", s_actorDefinitions[5].m_animGrpName);
			s_actorDefinitions[5].m_scaleBySpeed = ParseXmlAttribute(*animGrp, "scaleBySpeed", s_actorDefinitions[5].m_scaleBySpeed);
			s_actorDefinitions[5].m_secondsPerFrame = ParseXmlAttribute(*animGrp, "secondsPerFrame", s_actorDefinitions[5].m_secondsPerFrame);
			s_actorDefinitions[5].m_playbackMode = ParseXmlAttribute(*animGrp, "playbackMode", s_actorDefinitions[5].m_playbackMode);

			spriteAnimGrp->m_name = s_actorDefinitions[5].m_animGrpName;
			spriteAnimGrp->m_scaleBySpeed = s_actorDefinitions[5].m_scaleBySpeed;
			spriteAnimGrp->m_secondsPerFrame = s_actorDefinitions[5].m_secondsPerFrame;
			spriteAnimGrp->m_playbackMode = s_actorDefinitions[5].m_playbackMode;

			XmlElement* animGrpDir = animGrp->FirstChildElement();

			while (animGrpDir)
			{
				name = std::string(animGrp->FirstChildElement()->Name());

				if (name == "Direction")
				{
					Vec3 direction = ParseXmlAttribute(*animGrpDir, "vector", Vec3::ZERO);

					spriteAnimGrp->m_spriteDirection.push_back(direction.GetNormalized());
				}

				if (animGrpDir->FirstChildElement())
				{
					name = std::string(animGrpDir->FirstChildElement()->Name());

					if (name == "Animation")
					{
						Texture* spriteTexture = g_theRenderer->CreateOrGetTextureFromFile(s_actorDefinitions[5].m_spriteSheet.c_str());
						SpriteSheet* spriteSheet = new SpriteSheet(*spriteTexture, s_actorDefinitions[5].m_cellCount);
						spriteSheet->CalculateUVsOfSpriteSheet();

						int startIndex = ParseXmlAttribute(*animGrpDir->FirstChildElement(), "startFrame", -1);
						int endIndex = ParseXmlAttribute(*animGrpDir->FirstChildElement(), "endFrame", -1);

						float duration = (endIndex - startIndex + 1) * s_actorDefinitions[5].m_secondsPerFrame;

						SpriteAnimDefinition* spriteAnim = new SpriteAnimDefinition(*spriteSheet, startIndex, endIndex, duration);

						spriteAnimGrp->m_spriteAnimDefs.push_back(spriteAnim);
					}
				}

				animGrpDir = animGrpDir->NextSiblingElement();
			}

			s_actorDefinitions[5].m_spriteAnimGrpDefs.push_back(*spriteAnimGrp);
		}

		animGrp = animGrp->NextSiblingElement();
	}
}

void Actor::UpdatePhysics(float deltaseconds)
{
	AddForce(m_velocity * -1.0f * m_definition.m_drag);

	m_velocity += m_accelaration * deltaseconds;
	m_position += m_velocity * deltaseconds;

	m_position.z = 0.0f;
}

void Actor::Damage(float damage)
{
	m_health -= damage;
	m_animName = "Hurt";
}

void Actor::AddForce(Vec3 forceValue)
{
	m_accelaration += forceValue;
}

void Actor::AddImpulse(Vec3 forceValue)
{
	m_velocity += forceValue;
}

void Actor::OnCollide(Actor* otherActor)
{
	UNUSED(otherActor);
}

void Actor::OnPossessed(Controller* controller)
{
	m_controller = controller;
}

void Actor::OnUnpossessed(Controller* controller)
{
	m_controller = controller;
}

void Actor::MoveInDirection(Vec3 direction, float force)
{							
	AddForce(direction * force);
}

void Actor::TurnInDirection(float degrees, float maxAngles)
{
	Vec2 forward2D = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis2D();
	float forwardDirection = forward2D.GetOrientationDegrees();

	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(forwardDirection, degrees, maxAngles);
}

void Actor::Attack()
{
}

void Actor::EquipWeapon()
{
}
