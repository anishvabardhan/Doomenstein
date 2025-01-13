#include "Game/Game.hpp"

#include "Game/GameCommon.hpp"
#include "Game/PlayerController.hpp"
#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"
#include "Game/Map.hpp"

#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Texture.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

Map* g_currentMap = nullptr;

Game::Game()
{
}

Game::~Game()
{
}

void Game::StartUp()
{
	m_gameClock = new Clock(Clock::GetSystemClock());

	m_screenCamera = new Camera();
	m_screenCamera->m_normalizedViewport = AABB2(0.0f, 0.0f, 1.0f, 1.0f);

	m_bitmapFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont.png");

	InitializeSoundData();

	g_theAudio->SetNumListeners(2);

	WeaponDefinition::InitializeDefs();
	ActorDefinition::InitializeDefs();
	MapDefinition::InitializeDef();

	EnterAttract();
	AttractScreenBloom();
	ConsoleControls();
}

void Game::Shutdown()
{
	DELETE_PTR(g_currentMap);
	DELETE_PTR(m_screenCamera);
}

void Game::Update(float deltaseconds)
{
	static float rate = 0.0f;
	rate += 100.0f * deltaseconds;
	m_thickness = 5.0f * fabsf(SinDegrees(rate));

	if (m_currentState == GameState::ATTRACT)
	{
		UpdateAttract(deltaseconds);
	}
	else if (m_currentState == GameState::PLAYING)
	{
		for (int i = 0; i < m_numOfPlayers; i++)
		{
			g_theAudio->UpdateListener(i, m_playerController[i]->m_position, m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
		}

		UpdatePlaying(deltaseconds);
	}
}

void Game::Render() const
{
	if (m_currentState == GameState::ATTRACT)
	{
		g_theRenderer->BeginCamera(*m_screenCamera);
	
		RenderAttract();
	
		g_theRenderer->EndCamera(*m_screenCamera);
	}
	else if(m_currentState == GameState::PLAYING)
	{
		RenderPlaying();
	}
}

void Game::EnterState(GameState state)
{
	m_currentState = state;
}

void Game::ExitState(GameState state)
{
	m_currentState = m_nextState;
	m_nextState = state;
}

void Game::EnterAttract()
{
	int size = m_numOfPlayers;

	for (int i = 0; i < size; i++)
	{
		if (m_playerController[i] != nullptr)
		{
			delete m_playerController[i];
			m_playerController[i] = nullptr;

			m_numOfPlayers--;		
		}
	}

	if(m_playerController.size() > 0)
		m_playerController.clear();
	
	if (g_currentMap != nullptr)
	{
		delete g_currentMap;
		g_currentMap = nullptr;
	}

	if (g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]))
	{
		g_theAudio->StopSound(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]);
	}
	
	if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]))
	{
		m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND] = g_theAudio->StartSound(m_allSoundIDs[GAME_ATTRACT_SOUND], true);
	}

	m_currentState = GameState::ATTRACT;
	m_nextState = GameState::PLAYING;

	if(g_currentMap == nullptr)
	{
		g_currentMap = new Map(this, &MapDefinition::s_definitions[2]);
	}
}

void Game::EnterPlaying()
{
	m_isLobby = true;
	m_isPlayMode = false;

	m_currentState = GameState::PLAYING;
	m_nextState = GameState::ATTRACT;

	g_currentMap->AttachAIControllers();
	g_currentMap->InitializeTiles();
}

void Game::ExitAttract()
{
	EnterPlaying();
}

void Game::ExitPlaying()
{
	m_isLobby = false;
	m_isPlayMode = false;

	EnterAttract();
}

void Game::UpdateAttract(float deltaseconds)
{
	UNUSED(deltaseconds);

	if (!g_theConsole->IsOpen())
	{
		HandleInput();
	}
	
	m_screenCamera->SetOrthoView(Vec2(0.0f, 0.0f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game::UpdatePlaying(float deltaseconds)
{
	if (!g_theConsole->IsOpen())
	{
		HandleInput();
		
		for (int i = 0; i < m_numOfPlayers; i++)
		{
			if (m_playerController[i])
			{
				m_playerController[i]->Update(deltaseconds);
			}
		}

		if (!m_isLobby && m_isPlayMode)
		{
			g_currentMap->Update(deltaseconds);
		}
	}
}

void Game::RenderAttract() const
{
	Mat44 transform;
	transform.SetTranslation2D(Vec2(800.0f, 400.0f));

	Mat44 transform1;
	transform1.SetTranslation2D(Vec2(800.0f, 100.0f));

	Mat44 transform2;
	transform2.SetTranslation2D(Vec2(800.0f, 80.0f));

	Mat44 transform3;
	transform3.SetTranslation2D(Vec2(800.0f, 60.0f));

	DebugAddScreenText("THE PAC-MAN WITCH PROJECT", transform, 50.0f, Vec2(0.5f, 0.5f), 0.0f);

	DebugAddScreenText("Press SPACE to join with mouse and keyboard", transform1, 20.0f, Vec2(0.5f, 0.5f), 0.0f);
	DebugAddScreenText("Press ESCAPE or BACK to exit the game", transform3, 20.0f, Vec2(0.5f, 0.5f), 0.0f);

	DebugRenderScreen(*m_screenCamera);
}

void Game::RenderPlaying() const
{
	if (m_isLobby && !m_isPlayMode)
	{
		for (int i = 0; i < m_numOfPlayers; i++)
		{
			g_theRenderer->BeginCamera(*m_playerController[i]->m_screenCamera);

			//if (m_playerController[i]->m_isControllerInput)
			//{
			//	std::vector<Vertex_PCU> textVerts;
			//
			//	Vec2 loc1 = Vec2(600.0f, 400.0f);
			//	Vec2 loc2 = Vec2(500.0f, 100.0f);
			//	Vec2 loc3 = Vec2(500.0f, 80.0f);
			//	Vec2 loc4 = Vec2(500.0f, 60.0f);
			//
			//	m_bitmapFont->AddVertsForText2D(textVerts, loc1, 20.0f, Stringf("PLAYER %d: CONTROLLER", i + 1));
			//	m_bitmapFont->AddVertsForText2D(textVerts, loc2, 20.0f, "Press START to start the game");
			//	m_bitmapFont->AddVertsForText2D(textVerts, loc3, 20.0f, "Press SPACE to join");
			//	m_bitmapFont->AddVertsForText2D(textVerts, loc4, 20.0f, "Press BACK to exit the lobby");
			//
			//	g_theRenderer->SetModelConstants();
			//	g_theRenderer->BindTexture(&m_bitmapFont->GetTexture());
			//	g_theRenderer->BindShader();
			//	g_theRenderer->DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
			//
			//	g_theRenderer->EndCamera(*m_playerController[i]->m_screenCamera);
			//}
			//else
			{
				std::vector<Vertex_PCU> textVerts;

				Vec2 loc1 = Vec2(500.0f, 400.0f);
				Vec2 loc2 = Vec2(500.0f, 100.0f);
				Vec2 loc3 = Vec2(500.0f, 80.0f);
				Vec2 loc4 = Vec2(500.0f, 60.0f);

				m_bitmapFont->AddVertsForText2D(textVerts, loc1, 20.0f, Stringf("PLAYER: MOUSE and KEYBOARD", i + 1));
				m_bitmapFont->AddVertsForText2D(textVerts, loc2, 20.0f, "Press SPACE to start the game");
				m_bitmapFont->AddVertsForText2D(textVerts, loc4, 20.0f, "Press BACK to exit the lobby");

				g_theRenderer->SetModelConstants();
				g_theRenderer->BindTexture(&m_bitmapFont->GetTexture());
				g_theRenderer->BindShader();
				g_theRenderer->DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());

				g_theRenderer->EndCamera(*m_playerController[i]->m_screenCamera);
			}
		}
	}
	else if (!m_isLobby && m_isPlayMode)
	{
		for (int i = 0; i < m_numOfPlayers; i++)
		{
			g_theRenderer->BeginCamera(*m_playerController[i]->m_worldCamera);

			g_currentMap->Render(*m_playerController[i]->m_worldCamera);

			g_theRenderer->EndCamera(*m_playerController[i]->m_worldCamera);

			if (!m_isFreeFlyingMode)
			{
				if(m_playerController[i]->m_actorUID != ActorUID::INVALID && m_playerController[i]->GetActor())
				{
					m_playerController[i]->RenderHUD();

					DebugRenderScreen(*m_playerController[i]->m_screenCamera);
				}
			}
		}
	}
}

void Game::HandleInput()
{
	if (m_currentState == GameState::ATTRACT)
	{
		XboxController const& controller = g_theInputSystem->GetController(0);

		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC) || (controller.GetButton(BUTTON_BACK).m_isPressed && !controller.GetButton(BUTTON_BACK).m_wasPressedLastFrame))
		{
			g_theApp->HandleQuitRequested();
		}

		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_SPACE))
		{
			PlayerController* player = new PlayerController(this, AABB2(0.0f, 0.0f, 1.0f, 1.0f), 2.0f, m_numOfPlayers);
			m_playerController.push_back(player);
			g_currentMap->PossessPlayer(m_numOfPlayers);
			m_numOfPlayers++;

			g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
			ExitAttract();
		}

		//if (controller.GetButton(BUTTON_START).m_isPressed && !controller.GetButton(BUTTON_START).m_wasPressedLastFrame)
		//{
		//	PlayerController* player = new PlayerController(this, AABB2(0.0f, 0.0f, 1.0f, 1.0f), 2.0f, m_numOfPlayers);
		//	player->m_isControllerInput = true;
		//	m_playerController.push_back(player);
		//	g_currentMap->PossessPlayer(m_numOfPlayers);
		//	m_numOfPlayers++;
		//
		//	g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
		//	ExitAttract();
		//}
	}
	else if(m_currentState == GameState::PLAYING)
	{
		if (m_isLobby && !m_isPlayMode)
		{
			//if (m_playerController[0]->m_isControllerInput)
			//{
			//	XboxController const& controller = g_theInputSystem->GetController(0);
			//
			//	if (m_numOfPlayers == 1)
			//	{
			//		if (controller.GetButton(BUTTON_BACK).m_isPressed && !controller.GetButton(BUTTON_BACK).m_wasPressedLastFrame)
			//		{
			//			g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
			//			m_isLobby = false;
			//			m_isPlayMode = false;
			//			ExitPlaying();
			//		}
			//
			//		if (controller.GetButton(BUTTON_START).m_isPressed && !controller.GetButton(BUTTON_START).m_wasPressedLastFrame)
			//		{
			//			m_isLobby = false;
			//			m_isPlayMode = true;
			//	
			//			if (g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]))
			//			{
			//				g_theAudio->StopSound(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]);
			//			}
			//	
			//			if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]))
			//			{
			//				m_allSoundPlaybackIDs[GAME_PLAYING_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_PLAYING_SOUND], m_playerController[0]->m_position, true, 0.5f);
			//			}
			//
			//			if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND]))
			//			{
			//				m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_AMBIENCE_SOUND], m_playerController[0]->m_position, true, 0.3f);
			//			}
			//		}
			//	}
			//	else if(m_numOfPlayers > 1 && !g_theInputSystem->WasKeyJustReleased(KEYCODE_SPACE))
			//	{
			//		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC))
			//		{
			//			g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
			//			m_isLobby = false;
			//			m_isPlayMode = false;
			//			ExitPlaying();
			//		}
			//		else if (controller.GetButton(BUTTON_BACK).m_isPressed && !controller.GetButton(BUTTON_BACK).m_wasPressedLastFrame)
			//		{
			//			g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
			//			m_isLobby = false;
			//			m_isPlayMode = false;
			//			ExitPlaying();
			//		}
			//
			//		if (controller.GetButton(BUTTON_START).m_isPressed && !controller.GetButton(BUTTON_START).m_wasPressedLastFrame)
			//		{
			//			m_isLobby = false;
			//			m_isPlayMode = true;
			//	
			//			if (g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]))
			//			{
			//				g_theAudio->StopSound(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]);
			//			}
			//	
			//			if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]))
			//			{
			//				m_allSoundPlaybackIDs[GAME_PLAYING_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_PLAYING_SOUND], m_playerController[0]->m_position, true, 0.5f);
			//			}
			//
			//			if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND]))
			//			{
			//				m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_AMBIENCE_SOUND], m_playerController[0]->m_position, true, 0.3f);
			//			}
			//
			//			return;
			//		}
			//		else if (g_theInputSystem->WasKeyJustPressed(KEYCODE_SPACE))
			//		{
			//			m_isLobby = false;
			//			m_isPlayMode = true;
			//	
			//			if (g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]))
			//			{
			//				g_theAudio->StopSound(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]);
			//			}
			//	
			//			if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]))
			//			{
			//				m_allSoundPlaybackIDs[GAME_PLAYING_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_PLAYING_SOUND], m_playerController[0]->m_position, true, 0.5f);
			//			}
			//
			//			if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND]))
			//			{
			//				m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_AMBIENCE_SOUND], m_playerController[0]->m_position, true, 0.3f);
			//			}
			//
			//			return;
			//		}
			//	}
			//
			//	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_SPACE))
			//	{
			//		PlayerController* player = new PlayerController(this, AABB2(0.0f, 0.0f, 1.0f, 0.5f), 4.0f, m_numOfPlayers);
			//		m_playerController.push_back(player);
			//		g_currentMap->SpawnPlayer();
			//		g_currentMap->PossessPlayer(m_numOfPlayers);
			//		m_numOfPlayers++;
			//
			//		m_playerController[0]->m_worldCamera->m_normalizedViewport = AABB2(0.0f, 0.5f, 1.0f, 1.0f);
			//		m_playerController[0]->m_screenCamera->m_normalizedViewport = AABB2(0.0f, 0.5f, 1.0f, 1.0f);
			//		m_playerController[0]->m_playerAspect = 4.0f;
			//
			//		g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
			//	}
			//}
			//else
			{
				//XboxController const& controller = g_theInputSystem->GetController(0);

				if (m_numOfPlayers == 1)
				{
					if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC))
					{
						g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
						m_isLobby = false;
						m_isPlayMode = false;
						ExitPlaying();
					}

					if (g_theInputSystem->WasKeyJustPressed(KEYCODE_SPACE))
					{
						m_isLobby = false;
						m_isPlayMode = true;
				
						if (g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]))
						{
							g_theAudio->StopSound(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]);
						}
				
						if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]))
						{
							m_allSoundPlaybackIDs[GAME_PLAYING_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_PLAYING_SOUND], m_playerController[0]->m_position, true, 0.5f);
						}

						if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND]))
						{
							m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_AMBIENCE_SOUND], m_playerController[0]->m_position, true, 0.3f);
						}
					}
				}
				//else if(m_numOfPlayers > 1 && (controller.GetButton(BUTTON_START).m_isPressed || !controller.GetButton(BUTTON_START).m_wasPressedLastFrame))
				//{
				//	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC))
				//	{
				//		g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
				//		m_isLobby = false;
				//		m_isPlayMode = false;
				//		ExitPlaying();
				//	}
				//	else if (controller.GetButton(BUTTON_BACK).m_isPressed && !controller.GetButton(BUTTON_BACK).m_wasPressedLastFrame)
				//	{
				//		g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
				//		m_isLobby = false;
				//		m_isPlayMode = false;
				//		ExitPlaying();
				//	}
				//
				//	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_SPACE))
				//	{
				//		m_isLobby = false;
				//		m_isPlayMode = true;
				//
				//		if (g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]))
				//		{
				//			g_theAudio->StopSound(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]);
				//		}
				//
				//		if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]))
				//		{
				//			m_allSoundPlaybackIDs[GAME_PLAYING_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_PLAYING_SOUND], m_playerController[0]->m_position, true, 0.5f);
				//		}
				//
				//		if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND]))
				//		{
				//			m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_AMBIENCE_SOUND], m_playerController[0]->m_position, true, 0.3f);
				//		}
				//		return;
				//	}
				//	else if (controller.GetButton(BUTTON_START).m_isPressed && !controller.GetButton(BUTTON_START).m_wasPressedLastFrame)
				//	{
				//		m_isLobby = false;
				//		m_isPlayMode = true;
				//
				//		if (g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]))
				//		{
				//			g_theAudio->StopSound(m_allSoundPlaybackIDs[GAME_ATTRACT_SOUND]);
				//		}
				//
				//		if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]))
				//		{
				//			m_allSoundPlaybackIDs[GAME_PLAYING_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_PLAYING_SOUND], m_playerController[0]->m_position, true, 0.5f);
				//		}
				//
				//		if (!g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND]))
				//	 	{
				//			m_allSoundPlaybackIDs[GAME_AMBIENCE_SOUND] = g_theAudio->StartSoundAt(m_allSoundIDs[GAME_AMBIENCE_SOUND], m_playerController[0]->m_position, true, 0.3f);
				//		}
				//		return;
				//	}
				//}
				//
				//if (controller.GetButton(BUTTON_START).m_isPressed && !controller.GetButton(BUTTON_START).m_wasPressedLastFrame)
				//{
				//	PlayerController* player = new PlayerController(this, AABB2(0.0f, 0.0f, 1.0f, 0.5f), 4.0f, m_numOfPlayers);
				//	player->m_isControllerInput = true;
				//	m_playerController.push_back(player);
				//	g_currentMap->SpawnPlayer();
				//	g_currentMap->PossessPlayer(m_numOfPlayers);
				//	m_numOfPlayers++;
				//
				//	m_playerController[0]->m_worldCamera->m_normalizedViewport = AABB2(0.0f, 0.5f, 1.0f, 1.0f);
				//	m_playerController[0]->m_screenCamera->m_normalizedViewport = AABB2(0.0f, 0.5f, 1.0f, 1.0f);
				//	m_playerController[0]->m_playerAspect = 4.0f;
				//
				//	g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
				//}
			}
		}
		else if (!m_isLobby && m_isPlayMode)
		{
			if (g_theAudio->IsPlaying(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND]))
			{
				for (int i = 0; i < m_numOfPlayers; i++)
				{
					if (m_playerController[i] != nullptr)
					{
						g_theAudio->SetSoundPosition(m_allSoundPlaybackIDs[GAME_PLAYING_SOUND], m_playerController[i]->m_position);
					}
				}
			}

			for (int i = 0; i < m_numOfPlayers; i++)
			{
				if (m_playerController[i] != nullptr)
				{
					g_theAudio->UpdateListener(i, m_playerController[i]->m_position, m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), m_playerController[i]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
				}
			}

			if (g_theInputSystem->WasKeyJustPressed('F'))
			{
				g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
				m_isFreeFlyingMode = !m_isFreeFlyingMode;
			}

			XboxController const& controller = g_theInputSystem->GetController(0);

			if (controller.GetButton(BUTTON_BACK).m_isPressed && !controller.GetButton(BUTTON_BACK).m_wasPressedLastFrame)
			{
				g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
				m_isLobby = false;
				m_isPlayMode = false;
				ExitPlaying();
			}

			if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC))
			{
				g_theAudio->StartSound(m_allSoundIDs[GAME_BUTTON_CLICK]);
				m_isLobby = false;
				m_isPlayMode = false;
				ExitPlaying();
			}
		}
	}
}


void Game::InitializeSoundData()
{
	m_allSoundIDs[GAME_ATTRACT_SOUND]			= g_theAudio->CreateOrGetSound3D("Data/Audio/AttractGameSound.mp3");
	m_allSoundIDs[GAME_PLAYING_SOUND]			= g_theAudio->CreateOrGetSound3D("Data/Audio/GameplayAmbience.wav");
	m_allSoundIDs[GAME_AMBIENCE_SOUND]			= g_theAudio->CreateOrGetSound3D("Data/Audio/NightAmbience.mp3");
	m_allSoundIDs[GAME_WALKING_SOUND]			= g_theAudio->CreateOrGetSound3D("Data/Audio/Walking.mp3");
	m_allSoundIDs[GAME_WALKING_STONE_SOUND]		= g_theAudio->CreateOrGetSound3D("Data/Audio/WalkingStone.mp3");
	m_allSoundIDs[GAME_BUTTON_CLICK]			= g_theAudio->CreateOrGetSound3D("Data/Audio/Click.mp3");
	m_allSoundIDs[GAME_TORCH_ATTACK]			= g_theAudio->CreateOrGetSound3D("Data/Audio/FlashlightAttack.wav");
	m_allSoundIDs[GAME_TORCH_ATTACK_OFF]		= g_theAudio->CreateOrGetSound3D("Data/Audio/FlashlightAttackOff.wav");
	m_allSoundIDs[GAME_PISTOL_FIRE]				= g_theAudio->CreateOrGetSound3D("Data/Audio/PistolFire.wav");
	m_allSoundIDs[GAME_PLASMA_FIRE]				= g_theAudio->CreateOrGetSound3D("Data/Audio/PlasmaFire.wav");
	m_allSoundIDs[GAME_PLASMA_HIT]				= g_theAudio->CreateOrGetSound3D("Data/Audio/PlasmaHit.wav");
	m_allSoundIDs[GAME_DEMON_DEFAULT]			= g_theAudio->CreateOrGetSound3D("Data/Audio/GhostWalk.mp3");
	m_allSoundIDs[GAME_DEMON_HURT]				= g_theAudio->CreateOrGetSound3D("Data/Audio/DemonHurt.wav");
	m_allSoundIDs[GAME_DEMON_DEATH]				= g_theAudio->CreateOrGetSound3D("Data/Audio/GhostDeath.mp3");
	m_allSoundIDs[GAME_DEMON_ATTACK]			= g_theAudio->CreateOrGetSound3D("Data/Audio/DemonAttack.wav");
	m_allSoundIDs[GAME_PLAYER_HURT]				= g_theAudio->CreateOrGetSound3D("Data/Audio/PlayerHurt.wav");
	m_allSoundIDs[GAME_PLAYER_DEATH]			= g_theAudio->CreateOrGetSound3D("Data/Audio/PlayerDeath1.wav");
}

void Game::AttractScreenBloom()
{
	m_ogTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Textures/sphere.png");
	m_thresholdTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Textures/sphereBoxBlur.png");/*g_theRenderer->CreateModifiableTexture(IntVec2(m_ogTexture->m_dimensions.x, m_ogTexture->m_dimensions.y));*/
	m_downSampledTexture = g_theRenderer->CreateModifiableTexture(IntVec2(m_ogTexture->m_dimensions.x / 2, m_ogTexture->m_dimensions.y / 2));
	m_hBlurredTexture = g_theRenderer->CreateModifiableTexture(IntVec2(m_ogTexture->m_dimensions.x, m_ogTexture->m_dimensions.y));
	m_blurredTexture = g_theRenderer->CreateModifiableTexture(IntVec2(m_ogTexture->m_dimensions.x, m_ogTexture->m_dimensions.y));
	m_upSampledTexture = g_theRenderer->CreateModifiableTexture(IntVec2(m_ogTexture->m_dimensions.x, m_ogTexture->m_dimensions.y));
	m_compositeTexture = g_theRenderer->CreateModifiableTexture(IntVec2(m_ogTexture->m_dimensions.x, m_ogTexture->m_dimensions.y));
	
	//g_theRenderer->CreateThresholdCSShader("Data/Shaders/ThresholdingCS.hlsl");
	//g_theRenderer->RunThresholdCSShader(*m_ogTexture, *m_thresholdTexture);
	
	//g_theRenderer->CreateDownsampleCSShader("Data/Shaders/DownsamplingCS.hlsl");
	//g_theRenderer->RunDownsampleCSShader(*m_thresholdTexture, *m_downSampledTexture);
	
	// 10-12 Blur passes.
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_thresholdTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/HorizontalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_blurredTexture, *m_hBlurredTexture);
	//
	//g_theRenderer->CreateBlurCSShader("Data/Shaders/VerticalBlurCS.hlsl");
	//g_theRenderer->RunBlurCSShader(*m_hBlurredTexture, *m_blurredTexture);
	//
	//g_theRenderer->CreateUpsampleCSShader("Data/Shaders/UpsamplingCS.hlsl");
	//g_theRenderer->RunUpsampleCSShader(*m_blurredTexture, *m_upSampledTexture);
	
	//g_theRenderer->CreateCompositeCSShader("Data/Shaders/CompositeCS.hlsl");
	//g_theRenderer->RunCompositeCSShader(*m_ogTexture, *m_thresholdTexture, *m_compositeTexture);
}

void Game::ConsoleControls()
{
	DevConsoleLine line = DevConsoleLine("----------------------------------------", DevConsole::INFO_MAJOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS F											----> TOGGLE BETWEEN PLAYER AND CAMERA MODE", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("FREE-FLY CONTROLS", DevConsole::COMMAND_ECHO);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("HOLD SHIFT/A BUTTON								----> INCREASE CAMERA SPEED BY A FACTOR OF 10 WHILE HELD", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS Z/LEFT SHOULDER							----> MOVE UP RELATIVE TO WORLD", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS C/RIGHT SHOULDER							----> MOVE DOWN RELATIVE TO WORLD", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS W/LEFT STICK Y-AXIS						----> MOVE CAMERA FORWARD", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS S/LEFT STICK Y-AXIS						----> MOVE CAMERA BACKWARD", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS D/LEFT STICK X-AXIS						----> MOVE CAMERA RIGHT", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS A/LEFT STICK X-AXIS						----> MOVE CAMERA LEFT", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS UP/MOUSE									----> PITCH CAMERA UP", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS DOWN/MOUSE									----> PITCH CAMERA DOWN", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS RIGHT/MOUSE								----> YAW CAMERA RIGHT", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS LEFT/MOUSE									----> YAW CAMERA LEFT", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("----------------------------------------", DevConsole::INFO_MAJOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PLAYER CONTROLS", DevConsole::COMMAND_ECHO);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("HOLD SHIFT/A BUTTON								----> INCREASE CAMERA SPEED BY A FACTOR OF 10 WHILE HELD", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS W/LEFT STICK Y-AXIS						----> MOVE PLAYER FORWARD", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS S/LEFT STICK Y-AXIS						----> MOVE PLAYER BACKWARD", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS D/LEFT STICK X-AXIS						----> MOVE PLAYER RIGHT", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS A/LEFT STICK X-AXIS						----> MOVE PLAYER LEFT", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("MOUSE UP/RIGHT STICK Y-AXIS						----> PITCH PLAYER UP", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("MOUSE DOWN/RIGHT STICK Y-AXIS					----> PITCH PLAYER DOWN", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("MOUSE RIGHT/RIGHT STICK X-AXIS					----> YAW PLAYER RIGHT", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("MOUSE LEFT/RIGHT STICK X-AXIS					----> YAW PLAYER LEFT", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("HOLD MOUSE LEFT BUTTON/RIGHT TRIGGER				----> FIRE WEAPON", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS 1/X BUTTON									----> WEAPON 1", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS 2/Y BUTTON									----> WEAPON 2", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS RIGHT ARROW/D PAD RIGHT BUTTON             ----> GET NEXT WEAPON", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("PRESS LEFT ARROW/D PAD LEFT BUTTON				----> GET PREVIOUS WEAPON", DevConsole::INFO_MINOR);
	g_theConsole->m_lines.push_back(line);

	line = DevConsoleLine("----------------------------------------", DevConsole::INFO_MAJOR);
	g_theConsole->m_lines.push_back(line);
}
