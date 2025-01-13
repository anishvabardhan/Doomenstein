#include "Game/PlayerController.hpp"

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteAnimGroupDefinition.hpp"

#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"

extern Map* g_currentMap;

PlayerController::PlayerController(Game* owner, AABB2 viewport, float aspect, int playerIndex)
{
	m_game = owner;
	m_position = Vec3::ZERO;
	m_orientationDegrees = EulerAngles(0.0f, 0.0f, 0.0f);
	
	m_worldCamera = new Camera();
	m_screenCamera = new Camera();

	m_worldCamera->m_normalizedViewport = viewport;
	m_screenCamera->m_normalizedViewport = viewport;

	m_map = g_currentMap;
	m_playerIndex = playerIndex;
	m_playerAspect = aspect;
}

PlayerController::~PlayerController()
{
	//GetActor()->OnUnpossessed(this);
}

void PlayerController::Update(float deltaseconds)
{
	if (!g_theConsole->IsOpen())
	{
		if (m_game->m_isFreeFlyingMode)
		{
			UpdateCamera(deltaseconds);
		}
		else
		{
			UpdateInput(deltaseconds);
		}
	}
}

void PlayerController::RenderHUD() const
{
	if (m_actorUID != ActorUID::INVALID && m_actorUID.GetActor() != nullptr && m_actorUID.GetActor()->m_definition.m_name == "Marine")
	{
		if (!m_actorUID.GetActor()->m_isDead)
		{
			g_theRenderer->BeginCamera(*m_screenCamera);

			Texture* reticleTexture = nullptr;

			Texture* weaponTexture = nullptr;

			Vec2 uvMins, uvMaxs;

			if (m_equippedWeaponIndex == 0)
			{
				reticleTexture = g_theRenderer->CreateOrGetTextureFromFile(m_actorUID.GetActor()->m_weapons[0]->m_definition.m_reticleTexture.c_str());

				if (m_isShooting)
				{
					SpriteAnimDefinition* spriteAnim = WeaponDefinition::s_weaponDefinitions[0].m_weaponAnimDef[1];

					weaponTexture = &spriteAnim->GetSpriteDefAtTime(m_weaponAnimTime).GetTexture();

					spriteAnim->GetSpriteDefAtTime(m_weaponAnimTime).GetUVs(uvMins, uvMaxs);
				}
				else
				{
					SpriteAnimDefinition* spriteAnim = WeaponDefinition::s_weaponDefinitions[0].m_weaponAnimDef[0];

					weaponTexture = &spriteAnim->GetSpriteDefAtTime(m_weaponAnimTime).GetTexture();

					spriteAnim->GetSpriteDefAtTime(m_weaponAnimTime).GetUVs(uvMins, uvMaxs);
				}
			}
			else if (m_equippedWeaponIndex == 1)
			{
				uvMins = Vec2::ZERO;
				uvMaxs = Vec2::ONE;
				weaponTexture = g_theRenderer->CreateOrGetTextureFromFile(m_actorUID.GetActor()->m_weapons[m_equippedWeaponIndex]->m_definition.m_baseTexture.c_str());
			}

			if (m_equippedWeaponIndex != 2)
			{
				std::vector<Vertex_PCU> reticleVerts;

				AABB2 reticleBox = AABB2(-(float)m_actorUID.GetActor()->m_weapons[m_equippedWeaponIndex]->m_definition.m_reticleSize.x * 0.5f * (m_playerAspect / 2.0f), -(float)m_actorUID.GetActor()->m_weapons[m_equippedWeaponIndex]->m_definition.m_reticleSize.y * 0.5f * (m_playerAspect / 2.0f), (float)m_actorUID.GetActor()->m_weapons[m_equippedWeaponIndex]->m_definition.m_reticleSize.x * 0.5f  * (m_playerAspect / 2.0f), (float)m_actorUID.GetActor()->m_weapons[m_equippedWeaponIndex]->m_definition.m_reticleSize.y * 0.5f * (m_playerAspect / 2.0f));

				AddVertsForAABB2D(reticleVerts, reticleBox, Rgba8::WHITE);

				Mat44 transform;
				transform.SetTranslation2D(Vec2(800.0f, 400.0f));

				g_theRenderer->SetModelConstants(transform);
				g_theRenderer->SetBlendMode(BlendMode::ALPHA);
				g_theRenderer->SetDepthMode(DepthMode::ENABLED);
				g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
				g_theRenderer->BindTexture(reticleTexture);
				g_theRenderer->BindShader();
				g_theRenderer->DrawVertexArray((int)reticleVerts.size(), reticleVerts.data());

				std::vector<Vertex_PCU> weaponVerts;

				AABB2 weaponBox = AABB2(-(float)m_actorUID.GetActor()->m_weapons[m_equippedWeaponIndex]->m_definition.m_spriteSize.x * 0.5f * (2.0f / m_playerAspect), (2.0f / m_playerAspect), (float)m_actorUID.GetActor()->m_weapons[m_equippedWeaponIndex]->m_definition.m_spriteSize.x * 0.5f * (2.0f / m_playerAspect), (float)m_actorUID.GetActor()->m_weapons[m_equippedWeaponIndex]->m_definition.m_spriteSize.y * (2.0f / m_playerAspect));

				Vec2 weaponPos = Vec2(SCREEN_SIZE_X * 0.5f, 0.0f);

				transform.SetTranslation2D(weaponPos);

				AddVertsForAABB2D(weaponVerts, weaponBox, Rgba8::WHITE, uvMins, uvMaxs);

				g_theRenderer->SetModelConstants(transform);
				g_theRenderer->SetBlendMode(BlendMode::ALPHA);
				g_theRenderer->SetDepthMode(DepthMode::ENABLED);
				g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
				g_theRenderer->BindTexture(weaponTexture);
				g_theRenderer->BindShader();
				g_theRenderer->DrawVertexArray((int)weaponVerts.size(), weaponVerts.data());

				std::vector<Vertex_PCU> healthVerts;

				AABB2 healthBox = AABB2(0.0f, 0.0f, 500.0f * (GetActor()->m_health / 100.0f), 20.0f);

				Vec2 healthPos = Vec2(20.0f, 750.0f);

				transform.SetTranslation2D(healthPos);

				AddVertsForAABB2D(healthVerts, healthBox, Rgba8::RED);
				AddVertsForAABB2D(healthVerts, AABB2(0.0f, 0.0f, 500.0f, 20.0f), Rgba8(255, 255, 255, 50));

				g_theRenderer->SetModelConstants(transform);
				g_theRenderer->SetBlendMode(BlendMode::ALPHA);
				g_theRenderer->SetDepthMode(DepthMode::ENABLED);
				g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
				g_theRenderer->BindTexture();
				g_theRenderer->BindShader();
				g_theRenderer->DrawVertexArray((int)healthVerts.size(), healthVerts.data());

				std::vector<Vertex_PCU> staminaVerts;

				AABB2 staminaBox = AABB2(0.0f, 0.0f, 500.0f * (m_stamina / 100.0f), 20.0f);

				Vec2 staminaPos = Vec2(20.0f, 725.0f);

				transform.SetTranslation2D(staminaPos);

				AddVertsForAABB2D(staminaVerts, staminaBox, Rgba8(0, 150, 255, 255));
				AddVertsForAABB2D(staminaVerts, AABB2(0.0f, 0.0f, 500.0f, 20.0f), Rgba8(255, 255, 255, 50));

				g_theRenderer->SetModelConstants(transform);
				g_theRenderer->SetBlendMode(BlendMode::ALPHA);
				g_theRenderer->SetDepthMode(DepthMode::ENABLED);
				g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
				g_theRenderer->BindTexture();
				g_theRenderer->BindShader();
				g_theRenderer->DrawVertexArray((int)staminaVerts.size(), staminaVerts.data());

				std::vector<Vertex_PCU> batteryVerts;

				AABB2 batteryBox = AABB2(0.0f, 0.0f, 120.0f * (m_batteryLife / 100.0f), 40.0f);

				Vec2 batteryPos = Vec2(31.0f, 60.0f);

				transform.SetTranslation2D(batteryPos);

				AddVertsForAABB2D(batteryVerts, batteryBox, Rgba8::WHITE);

				g_theRenderer->SetModelConstants(transform);
				g_theRenderer->SetBlendMode(BlendMode::ALPHA);
				g_theRenderer->SetDepthMode(DepthMode::ENABLED);
				g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
				g_theRenderer->BindTexture();
				g_theRenderer->BindShader();
				g_theRenderer->DrawVertexArray((int)batteryVerts.size(), batteryVerts.data());

				std::vector<Vertex_PCU> batteryCaseVerts;

				Texture* batteryTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Textures/Battery.png");

				AABB2 batteryCaseBox = AABB2(0.0f, 0.0f, 150.0f, 100.0f);

				Vec2 batteryCasePos = Vec2(20.0f, 30.0f);

				transform.SetTranslation2D(batteryCasePos);

				AddVertsForAABB2D(batteryCaseVerts, batteryCaseBox, Rgba8::WHITE);

				g_theRenderer->SetModelConstants(transform);
				g_theRenderer->SetBlendMode(BlendMode::ALPHA);
				g_theRenderer->SetDepthMode(DepthMode::ENABLED);
				g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
				g_theRenderer->BindTexture(batteryTexture);
				g_theRenderer->BindShader();
				g_theRenderer->DrawVertexArray((int)batteryCaseVerts.size(), batteryCaseVerts.data());

				if (m_isInRedShrine)
				{
					std::vector<Vertex_PCU> textVerts;
					Vec2 loc =Vec2(300.0f, 400.0f);

					m_game->m_bitmapFont->AddVertsForText2D(textVerts, loc, 20.0f, "PRESS RIGHT MOUSE BUTTON TO SWITCH TO RED FLASHLIGHT", Rgba8::RED);

					g_theRenderer->SetModelConstants();
					g_theRenderer->SetBlendMode(BlendMode::ALPHA);
					g_theRenderer->SetDepthMode(DepthMode::ENABLED);
					g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
					g_theRenderer->BindTexture(&m_game->m_bitmapFont->GetTexture());
					g_theRenderer->BindShader();
					g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
				}

				if (m_isInGreenShrine)
				{
					std::vector<Vertex_PCU> textVerts;
					Vec2 loc = Vec2(300.0f, 400.0f);

					m_game->m_bitmapFont->AddVertsForText2D(textVerts, loc, 20.0f, "PRESS RIGHT MOUSE BUTTON TO SWITCH TO GREEN FLASHLIGHT", Rgba8::GREEN);

					g_theRenderer->SetModelConstants();
					g_theRenderer->SetBlendMode(BlendMode::ALPHA);
					g_theRenderer->SetDepthMode(DepthMode::ENABLED);
					g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
					g_theRenderer->BindTexture(&m_game->m_bitmapFont->GetTexture());
					g_theRenderer->BindShader();
					g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
				}

				if (m_isInBlueShrine)
				{
					std::vector<Vertex_PCU> textVerts;
					Vec2 loc = Vec2(300.0f, 400.0f);

					m_game->m_bitmapFont->AddVertsForText2D(textVerts, loc, 20.0f, "PRESS RIGHT MOUSE BUTTON TO SWITCH TO BLUE FLASHLIGHT", Rgba8::BLUE);

					g_theRenderer->SetModelConstants();
					g_theRenderer->SetBlendMode(BlendMode::ALPHA);
					g_theRenderer->SetDepthMode(DepthMode::ENABLED);
					g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
					g_theRenderer->BindTexture(&m_game->m_bitmapFont->GetTexture());
					g_theRenderer->BindShader();
					g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
				}
			}

			g_theRenderer->EndCamera(*m_screenCamera);
		}
	}
}

void PlayerController::UpdateCamera(float deltaseconds)
{
	m_velocity = Vec3::ZERO;

	m_angularVelocity.m_yawDegrees = 0.05f * g_theInputSystem->GetCursorClientDelta().x;
	m_angularVelocity.m_pitchDegrees = -0.05f * g_theInputSystem->GetCursorClientDelta().y;

	if (g_theInputSystem->IsKeyDown('W'))
	{
		m_velocity = 2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D();
	}

	if (g_theInputSystem->IsKeyDown('S'))
	{
		m_velocity = -2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D();
	}

	if (g_theInputSystem->IsKeyDown('A'))
	{
		m_velocity = 2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetJBasis3D();
	}

	if (g_theInputSystem->IsKeyDown('D'))
	{
		m_velocity = -2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetJBasis3D();
	}

	if (g_theInputSystem->IsKeyDown(KEYCODE_SHIFT))
	{
		m_velocity *= 10.0f;
	}

	m_position += m_velocity * deltaseconds;

	m_orientationDegrees = EulerAngles(m_orientationDegrees.m_yawDegrees + m_angularVelocity.m_yawDegrees, m_orientationDegrees.m_pitchDegrees + m_angularVelocity.m_pitchDegrees, m_orientationDegrees.m_rollDegrees + m_angularVelocity.m_rollDegrees);
	m_orientationDegrees.m_pitchDegrees = GetClamped(m_orientationDegrees.m_pitchDegrees, -85.0f, 85.0f);
	m_orientationDegrees.m_rollDegrees = GetClamped(m_orientationDegrees.m_rollDegrees, -45.0f, 45.0f);

	Vec3 position = Vec3(m_position.x, m_position.y, m_position.z + m_actorUID.GetActor()->m_definition.m_eyeHeight);

	m_worldCamera->SetTransform(position, m_orientationDegrees);
	m_worldCamera->SetRenderBasis(Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
	m_worldCamera->SetPerspectiveView(m_playerAspect, 60.0f, 0.1f, 1500.0f);
}

void PlayerController::UpdateInput(float deltaseconds)
{
	static Vec3 playerPos;
	static EulerAngles playerOrientation;
	static float cameraFOV = 0.0f;

	if (m_actorUID == ActorUID::INVALID)
	{
		if (m_respawnPlayer)
		{
			m_map->SpawnPlayer();
			m_map->PossessPlayer(m_playerIndex);
			m_respawnPlayer = false;
			m_respawnTime = 0.0f;
		}
	}
	else
	{
		if (m_actorUID.GetActor() != nullptr && !m_actorUID.GetActor()->m_isDead)
		{
			m_isInRedShrine = false;
			m_isInGreenShrine = false;
			m_isInBlueShrine = false;

			CheckInWhiteShrine(IntVec2(30, 36));
			CheckInWhiteShrine(IntVec2(15, 47));
			CheckInWhiteShrine(IntVec2(50, 39));
			CheckInWhiteShrine(IntVec2(32, 16));

			CheckInRedShrine(IntVec2(52, 19));
			CheckInRedShrine(IntVec2(34, 53));

			CheckInGreenShrine(IntVec2(22, 10));
			CheckInGreenShrine(IntVec2(13, 52));

			CheckInBlueShrine(IntVec2(9, 31));
			CheckInBlueShrine(IntVec2(49, 52));

			m_batteryLife -= m_batteryDischargeRate * deltaseconds;

			m_batteryLife = GetClamped(m_batteryLife, 0.0f, 100.0f);
			GetActor()->m_health = GetClamped(GetActor()->m_health, 0.0f, 100.0f);

			if (m_equippedWeaponIndex == 0)
			{
				if (g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_PISTOL_FIRE]))
				{
					g_theAudio->SetSoundPosition(m_game->m_allSoundPlaybackIDs[GAME_PISTOL_FIRE], m_position);
				}

				for (int i = 0; i < m_game->m_numOfPlayers; i++)
				{ 
					g_theAudio->UpdateListener(i, m_position, m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
				}

				if (m_isShooting)
				{
					int x = RoundDownToInt(m_position.x);
					int y = RoundDownToInt(m_position.y);

					int tileIndex = x + (y * m_map->m_dimensions.x);

					Vec2 pos = Vec2(m_position.x, m_position.y);

					if (m_map->m_tiles[tileIndex].GetDefinition()->m_name != "RockFloor")
					{
						m_weaponAnimName = m_actorUID.GetActor()->m_weapons[0]->m_definition.m_animName[1];
						m_weaponAnimDuration = m_actorUID.GetActor()->m_weapons[0]->m_definition.m_weaponAnimDef[1]->GetDuration();
					}
				}
				else
				{
					int x = RoundDownToInt(m_position.x);
					int y = RoundDownToInt(m_position.y);

					int tileIndex = x + (y * m_map->m_dimensions.x);

					Vec2 pos = Vec2(m_position.x, m_position.y);

					if (m_map->m_tiles[tileIndex].GetDefinition()->m_name != "RockFloor")
					{
						m_weaponAnimName = m_actorUID.GetActor()->m_weapons[0]->m_definition.m_animName[0];
						m_weaponAnimDuration = m_actorUID.GetActor()->m_weapons[0]->m_definition.m_weaponAnimDef[0]->GetDuration();
					}
				}

				m_weaponAnimTime += deltaseconds;

				if (m_weaponAnimTime >= m_weaponAnimDuration)
					m_weaponAnimTime = 0.0f;
			}
			else if (m_equippedWeaponIndex == 1)
			{
				if (g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_PLASMA_FIRE]))
				{
					g_theAudio->SetSoundPosition(m_game->m_allSoundPlaybackIDs[GAME_PLASMA_FIRE], m_position);
				}

				for (int i = 0; i < m_game->m_numOfPlayers; i++)
				{
					g_theAudio->UpdateListener(i, m_position, m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
				}

				//if (m_isShooting)
				//{
				//	m_weaponAnimName = m_actorUID.GetActor()->m_weapons[1]->m_definition.m_animName[1];
				//	m_weaponAnimDuration = m_actorUID.GetActor()->m_weapons[1]->m_definition.m_weaponAnimDef[1]->GetDuration();
				//}
				//else
				//{
				//	m_weaponAnimName = m_actorUID.GetActor()->m_weapons[1]->m_definition.m_animName[0];
				//	m_weaponAnimDuration = m_actorUID.GetActor()->m_weapons[1]->m_definition.m_weaponAnimDef[0]->GetDuration();
				//}
				//
				//m_weaponAnimTime += deltaseconds;
				//
				//if (m_weaponAnimTime >= m_weaponAnimDuration)
				//	m_weaponAnimTime = 0.0f;
			}

			m_corpseTime = m_actorUID.GetActor()->m_definition.m_corpseLifetime;

			//XboxController const& controller = g_theInputSystem->GetController(0);

			GetActor()->m_accelaration = Vec3::ZERO;
			m_velocity = Vec3::ZERO;

			if (!m_game->m_isLobby && m_game->m_isPlayMode)
			{
				m_isWalking = false;

				//if (m_isControllerInput)
				//{
				//	if (controller.GetButton(BUTTON_X).m_isPressed)
				//	{
				//		m_equippedWeaponIndex = 0;
				//	}
				//
				//	if (controller.GetButton(BUTTON_Y).m_isPressed)
				//	{
				//		m_equippedWeaponIndex = 1;
				//	}
				//
				//	if (controller.GetButton(BUTTON_DPAD_RIGHT).m_isPressed && !controller.GetButton(BUTTON_DPAD_RIGHT).m_wasPressedLastFrame)
				//	{
				//		if (m_equippedWeaponIndex == 1)
				//			m_equippedWeaponIndex = 0;
				//		else
				//			m_equippedWeaponIndex++;
				//	}
				//
				//	if (controller.GetButton(BUTTON_DPAD_LEFT).m_isPressed && !controller.GetButton(BUTTON_DPAD_LEFT).m_wasPressedLastFrame)
				//	{
				//		if (m_equippedWeaponIndex == 0)
				//			m_equippedWeaponIndex = 1;
				//		else
				//			m_equippedWeaponIndex--;
				//	}
				//
				//	m_angularVelocity.m_yawDegrees = 0.0f;
				//	m_angularVelocity.m_pitchDegrees = 0.0f;
				//
				//	if (controller.GetLeftStick().GetMagnitude() > 0.0f)
				//	{
				//		m_actorUID.GetActor()->m_velocity = (-m_actorUID.GetActor()->m_definition.m_runSpeed * controller.GetLeftStick().GetPosition().x * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetJBasis3D()) + (2.0f * controller.GetLeftStick().GetPosition().y * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
				//	}
				//
				//	if (controller.GetRightStick().GetMagnitude() > 0.0f)
				//	{
				//		m_angularVelocity.m_pitchDegrees = 0.5f * -controller.GetRightStick().GetPosition().y;
				//		m_angularVelocity.m_yawDegrees = 0.5f * -controller.GetRightStick().GetPosition().x;
				//	}
				//
				//	if (controller.GetButton(BUTTON_A).m_isPressed)
				//	{
				//		m_actorUID.GetActor()->m_velocity *= 10.0f;
				//	}
				//}
				//else
				{
					if (g_theInputSystem->WasKeyJustPressed('1'))
					{
						m_equippedWeaponIndex = 0;
					}

					if (g_theInputSystem->WasKeyJustPressed('2'))
					{
						m_equippedWeaponIndex = 1;
					}

					if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHTARROW))
					{
						if (m_equippedWeaponIndex == 1)
							m_equippedWeaponIndex = 0;
						else
							m_equippedWeaponIndex++;
					}

					if (g_theInputSystem->WasKeyJustPressed(KEYCODE_LEFTARROW))
					{
						if (m_equippedWeaponIndex == 0)
							m_equippedWeaponIndex = 1;
						else
							m_equippedWeaponIndex--;
					}

					m_torchCutoff = Interpolate(m_torchCutoff, 0.5f, 10.0f * deltaseconds);

					m_angularVelocity.m_yawDegrees = 0.05f * g_theInputSystem->GetCursorClientDelta().x;
					m_angularVelocity.m_pitchDegrees = -0.05f * g_theInputSystem->GetCursorClientDelta().y;

					if (m_equippedWeaponIndex == 1)
					{
						if (g_theInputSystem->IsKeyDown(KEYCODE_LEFT_MOUSE))
						{
							m_torchCutoff = Interpolate(m_torchCutoff, 0.05f, 20.0f * deltaseconds);

							m_batteryDischargeRate = 10.0f;

							if (!g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_TORCH_ATTACK]))
							{
								m_game->m_allSoundPlaybackIDs[GAME_TORCH_ATTACK] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_TORCH_ATTACK], m_position, true, 1.0f, 0.0f, 2.0f);
							}
							else
							{
								g_theAudio->SetSoundPosition(m_game->m_allSoundPlaybackIDs[GAME_TORCH_ATTACK], m_position);
							}
						}
						else
						{
							m_batteryDischargeRate = 1.0f;

							if (g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_TORCH_ATTACK]))
							{
								m_game->m_allSoundPlaybackIDs[GAME_TORCH_ATTACK_OFF] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_TORCH_ATTACK_OFF], m_position, false, 1.0f, 0.0f, 3.0f);
								g_theAudio->StopSound(m_game->m_allSoundPlaybackIDs[GAME_TORCH_ATTACK]);
							}
						}
					}

					if (g_theInputSystem->IsKeyDown('W'))
					{
						m_isWalking = true;
						m_velocity = 0.5f * m_actorUID.GetActor()->m_definition.m_runSpeed * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D();
					}

					if (g_theInputSystem->IsKeyDown('S'))
					{
						m_isWalking = true;
						m_velocity = -0.5f * m_actorUID.GetActor()->m_definition.m_runSpeed * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D();
					}

					if (g_theInputSystem->IsKeyDown('A'))
					{
						m_isWalking = true;
						m_velocity = 0.5f * m_actorUID.GetActor()->m_definition.m_runSpeed * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetJBasis3D();
					}

					if (g_theInputSystem->IsKeyDown('D'))
					{
						m_isWalking = true;
						m_velocity = -0.5f * m_actorUID.GetActor()->m_definition.m_runSpeed * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetJBasis3D();
					}
				}
				
				m_orientationDegrees = EulerAngles(m_actorUID.GetActor()->m_orientation.m_yawDegrees + m_angularVelocity.m_yawDegrees, m_actorUID.GetActor()->m_orientation.m_pitchDegrees + m_angularVelocity.m_pitchDegrees, m_actorUID.GetActor()->m_orientation.m_rollDegrees + m_angularVelocity.m_rollDegrees);
				
				if (m_isStaminaDrained)
				{
					m_stamina += 15.0f * deltaseconds;

					if (m_stamina >= 100.0f)
					{
						m_isStaminaDrained = false;
					}
				}

				if (m_actorUID.GetActor()->m_velocity.GetLengthSquared() > 0.0f)
				{
					if (!m_isStaminaDrained)
					{
						if (m_stamina < 0.0f)
							m_isStaminaDrained = true;

						if (g_theInputSystem->IsKeyDown(KEYCODE_SHIFT))
						{
							m_stamina -= 50.0f * deltaseconds;
						
							m_velocity *= 2.0f;
						
							if (m_CameraShakeTime > 0.1f)
							{
								RandomNumberGenerator random = RandomNumberGenerator();
								
								float endVal = random.RollRandomFloatInRange(-1.0f, 1.0f);
								
								m_orientationDegrees.m_rollDegrees = endVal;
								
								m_CameraShakeTime = 0.0f;
							}
						
							m_CameraShakeTime += deltaseconds;
						}
						else
						{
							m_orientationDegrees.m_rollDegrees = 0.0f;
						}
					}
				}

				m_actorUID.GetActor()->m_velocity = m_velocity;

				m_actorUID.GetActor()->Update(deltaseconds, *m_worldCamera);

				m_orientationDegrees.m_pitchDegrees = GetClamped(m_orientationDegrees.m_pitchDegrees, -85.0f, 85.0f);
				m_orientationDegrees.m_rollDegrees = GetClamped(m_orientationDegrees.m_rollDegrees, -45.0f, 45.0f);

				m_actorUID.GetActor()->m_orientation = m_orientationDegrees;

				m_position = m_actorUID.GetActor()->m_position;

				if (m_isWalking)
				{
					int tileX = RoundDownToInt(m_position.x);
					int tileY = RoundDownToInt(m_position.y);

					int tileIndex = tileX + (tileY * m_map->m_dimensions.x);

					if (m_map->m_tiles[tileIndex].m_definition->m_name == "OpenGrass")
					{
						if (g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_WALKING_STONE_SOUND]))
						{
							g_theAudio->StopSound(m_game->m_allSoundPlaybackIDs[GAME_WALKING_STONE_SOUND]);
						}

						if (!g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_WALKING_SOUND]))
						{
							m_game->m_allSoundPlaybackIDs[GAME_WALKING_SOUND] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_WALKING_SOUND], m_position, true, 0.3f, 0.0f, 2.5f);
						}
						else
						{
							g_theAudio->SetSoundPosition(m_game->m_allSoundPlaybackIDs[GAME_WALKING_SOUND], m_position);
						}
					}
					else if (m_map->m_tiles[tileIndex].m_definition->m_name == "RockFloor")
					{
						if (g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_WALKING_SOUND]))
						{
							g_theAudio->StopSound(m_game->m_allSoundPlaybackIDs[GAME_WALKING_SOUND]);
						}

						if (!g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_WALKING_STONE_SOUND]))
						{
							m_game->m_allSoundPlaybackIDs[GAME_WALKING_STONE_SOUND] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_WALKING_STONE_SOUND], m_position, true, 0.3f, 0.0f, 2.0f);
						}
						else
						{
							g_theAudio->SetSoundPosition(m_game->m_allSoundPlaybackIDs[GAME_WALKING_STONE_SOUND], m_position);
						}
					}
				}
				else
				{
					if (g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_WALKING_SOUND]))
					{
						g_theAudio->StopSound(m_game->m_allSoundPlaybackIDs[GAME_WALKING_SOUND]);
					}

					if (g_theAudio->IsPlaying(m_game->m_allSoundPlaybackIDs[GAME_WALKING_STONE_SOUND]))
					{
						g_theAudio->StopSound(m_game->m_allSoundPlaybackIDs[GAME_WALKING_STONE_SOUND]);
					}
				}

				m_torchPosition = Vec3(m_position.x, m_position.y, m_position.z + GetActor()->m_definition.m_eyeHeight);
				m_torchForward = m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D();

				m_torchCutoff = GetClamped(m_torchCutoff, 0.05f, 0.5f);

				playerPos = Vec3(m_position.x, m_position.y, m_actorUID.GetActor()->m_definition.m_eyeHeight);
				playerOrientation = m_orientationDegrees;
				cameraFOV = m_actorUID.GetActor()->m_definition.m_cameraFOV;
			}
		}
		else
		{
			playerPos.z -= deltaseconds;

			if (!m_actorUID.GetActor())
			{
				m_respawnTime += deltaseconds;
			}

			if (m_respawnTime > m_corpseTime)
			{
				m_respawnPlayer = true;
				m_actorUID = ActorUID::INVALID;
			}
		}

		m_worldCamera->SetTransform(playerPos, playerOrientation);
		m_worldCamera->SetRenderBasis(Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
		m_worldCamera->SetPerspectiveView(m_playerAspect, cameraFOV, 0.1f, 1500.0f);

		m_screenCamera->SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	}
}

void PlayerController::CheckInWhiteShrine(IntVec2 shrinePos)
{
	int x = RoundDownToInt(m_position.x);
	int y = RoundDownToInt(m_position.y);

	int posIndex = x + (y * m_map->m_dimensions.x);

	int tileIndex1 = (shrinePos.x + 2) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex2 = (shrinePos.x + 2) + ((shrinePos.y + 1) * m_map->m_dimensions.x);
	int tileIndex3 = (shrinePos.x + 2) + ((shrinePos.y + 0) * m_map->m_dimensions.x);
	int tileIndex4 = (shrinePos.x + 2) + ((shrinePos.y - 1) * m_map->m_dimensions.x);
	int tileIndex5 = (shrinePos.x + 2) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex6 = (shrinePos.x + 1) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex7 = (shrinePos.x + 0) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex8 = (shrinePos.x - 1) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex9 = (shrinePos.x - 2) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex10 = (shrinePos.x - 2) + ((shrinePos.y - 1) * m_map->m_dimensions.x);
	int tileIndex11 = (shrinePos.x - 2) + ((shrinePos.y + 0) * m_map->m_dimensions.x);
	int tileIndex12 = (shrinePos.x - 2) + ((shrinePos.y + 1) * m_map->m_dimensions.x);
	int tileIndex13 = (shrinePos.x - 2) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex14 = (shrinePos.x - 1) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex15 = (shrinePos.x + 0) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex16 = (shrinePos.x + 1) + ((shrinePos.y + 2) * m_map->m_dimensions.x);

	if (posIndex == tileIndex1)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex2)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex3)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex4)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex5)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex6)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex7)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex8)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex9)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex10)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex11)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex12)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex13)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex14)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex15)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
	else if (posIndex == tileIndex16)
	{
		GetActor()->m_health += 5.0f;
		m_batteryDischargeRate = -10.0f;
	}
}

void PlayerController::CheckInRedShrine(IntVec2 shrinePos)
{
	int x = RoundDownToInt(m_position.x);
	int y = RoundDownToInt(m_position.y);

	int posIndex = x + (y * m_map->m_dimensions.x);

	int tileIndex1 = (shrinePos.x + 2) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex2 = (shrinePos.x + 2) + ((shrinePos.y + 1) * m_map->m_dimensions.x);
	int tileIndex3 = (shrinePos.x + 2) + ((shrinePos.y + 0) * m_map->m_dimensions.x);
	int tileIndex4 = (shrinePos.x + 2) + ((shrinePos.y - 1) * m_map->m_dimensions.x);
	int tileIndex5 = (shrinePos.x + 2) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex6 = (shrinePos.x + 1) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex7 = (shrinePos.x + 0) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex8 = (shrinePos.x - 1) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex9 = (shrinePos.x - 2) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex10 = (shrinePos.x - 2) + ((shrinePos.y - 1) * m_map->m_dimensions.x);
	int tileIndex11 = (shrinePos.x - 2) + ((shrinePos.y + 0) * m_map->m_dimensions.x);
	int tileIndex12 = (shrinePos.x - 2) + ((shrinePos.y + 1) * m_map->m_dimensions.x);
	int tileIndex13 = (shrinePos.x - 2) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex14 = (shrinePos.x - 1) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex15 = (shrinePos.x + 0) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex16 = (shrinePos.x + 1) + ((shrinePos.y + 2) * m_map->m_dimensions.x);

	if (posIndex == tileIndex1)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex2)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex3)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex4)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex5)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex6)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex7)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex8)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex9)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex10)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::RED;
		}

		m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex11)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::RED;
	}

	m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex12)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::RED;
	}

	m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex13)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::RED;
	}

	m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex14)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::RED;
	}

	m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex15)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::RED;
	}

	m_isInRedShrine = true;
	}
	else if (posIndex == tileIndex16)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::RED;
	}

	m_isInRedShrine = true;
	}
}

void PlayerController::CheckInGreenShrine(IntVec2 shrinePos)
{
	int x = RoundDownToInt(m_position.x);
	int y = RoundDownToInt(m_position.y);

	int posIndex = x + (y * m_map->m_dimensions.x);

	int tileIndex1 = (shrinePos.x + 2) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex2 = (shrinePos.x + 2) + ((shrinePos.y + 1) * m_map->m_dimensions.x);
	int tileIndex3 = (shrinePos.x + 2) + ((shrinePos.y + 0) * m_map->m_dimensions.x);
	int tileIndex4 = (shrinePos.x + 2) + ((shrinePos.y - 1) * m_map->m_dimensions.x);
	int tileIndex5 = (shrinePos.x + 2) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex6 = (shrinePos.x + 1) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex7 = (shrinePos.x + 0) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex8 = (shrinePos.x - 1) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex9 = (shrinePos.x - 2) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex10 = (shrinePos.x - 2) + ((shrinePos.y - 1) * m_map->m_dimensions.x);
	int tileIndex11 = (shrinePos.x - 2) + ((shrinePos.y + 0) * m_map->m_dimensions.x);
	int tileIndex12 = (shrinePos.x - 2) + ((shrinePos.y + 1) * m_map->m_dimensions.x);
	int tileIndex13 = (shrinePos.x - 2) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex14 = (shrinePos.x - 1) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex15 = (shrinePos.x + 0) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex16 = (shrinePos.x + 1) + ((shrinePos.y + 2) * m_map->m_dimensions.x);

	if (posIndex == tileIndex1)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex2)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex3)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex4)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex5)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex6)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex7)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex8)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex9)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex10)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::GREEN;
		}

		m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex11)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::GREEN;
	}

	m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex12)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::GREEN;
	}

	m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex13)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::GREEN;
	}

	m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex14)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::GREEN;
	}

	m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex15)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::GREEN;
	}

	m_isInGreenShrine = true;
	}
	else if (posIndex == tileIndex16)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::GREEN;
	}

	m_isInGreenShrine = true;
	}
}

void PlayerController::CheckInBlueShrine(IntVec2 shrinePos)
{
	int x = RoundDownToInt(m_position.x);
	int y = RoundDownToInt(m_position.y);

	int posIndex = x + (y * m_map->m_dimensions.x);

	int tileIndex1 = (shrinePos.x + 2) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex2 = (shrinePos.x + 2) + ((shrinePos.y + 1) * m_map->m_dimensions.x);
	int tileIndex3 = (shrinePos.x + 2) + ((shrinePos.y + 0) * m_map->m_dimensions.x);
	int tileIndex4 = (shrinePos.x + 2) + ((shrinePos.y - 1) * m_map->m_dimensions.x);
	int tileIndex5 = (shrinePos.x + 2) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex6 = (shrinePos.x + 1) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex7 = (shrinePos.x + 0) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex8 = (shrinePos.x - 1) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex9 = (shrinePos.x - 2) + ((shrinePos.y - 2) * m_map->m_dimensions.x);
	int tileIndex10 = (shrinePos.x - 2) + ((shrinePos.y - 1) * m_map->m_dimensions.x);
	int tileIndex11 = (shrinePos.x - 2) + ((shrinePos.y + 0) * m_map->m_dimensions.x);
	int tileIndex12 = (shrinePos.x - 2) + ((shrinePos.y + 1) * m_map->m_dimensions.x);
	int tileIndex13 = (shrinePos.x - 2) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex14 = (shrinePos.x - 1) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex15 = (shrinePos.x + 0) + ((shrinePos.y + 2) * m_map->m_dimensions.x);
	int tileIndex16 = (shrinePos.x + 1) + ((shrinePos.y + 2) * m_map->m_dimensions.x);

	if (posIndex == tileIndex1)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex2)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex3)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex4)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex5)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex6)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex7)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex8)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex9)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex10)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex11)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::BLUE;
	}

	m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex12)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::BLUE;
	}

	m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex13)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::BLUE;
	}

	m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex14)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::BLUE;
	}

	m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex15)
	{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		m_torchColor = Rgba8::BLUE;
	}

	m_isInBlueShrine = true;
	}
	else if (posIndex == tileIndex16)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_torchColor = Rgba8::BLUE;
		}

		m_isInBlueShrine = true;
	}
}

void PlayerController::Possess(Actor* actor)
{
	if (actor)
	{
		if (m_actorUID != ActorUID::INVALID)
		{
			GetActor()->OnUnpossessed(this);
		}

		m_actorUID = actor->m_UID;

		if (m_actorUID != ActorUID::INVALID)
		{
			GetActor()->OnPossessed(this);
		}
	}
}

Actor* PlayerController::GetActor() const
{
	return m_actorUID.GetActor();
}

bool PlayerController::IsPlayer()
{
	return false;
}

void PlayerController::DamagedBy(Actor* actor)
{
	UNUSED(actor);
}

void PlayerController::KilledBy(Actor* actor)
{
	UNUSED(actor);
}

void PlayerController::Killed(Actor* actor)
{
	UNUSED(actor);
}