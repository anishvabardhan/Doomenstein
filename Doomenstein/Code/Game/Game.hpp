#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Audio/AudioSystem.hpp"

#include "Game/GameCommon.hpp"

#include <vector>

class Entity;
class PlayerController;
class Prop;
class Clock;
class BitmapFont;
class Map;
class Texture;

enum class GameState
{
	NONE,
	ATTRACT,
	LOBBY,
	PLAYING,
	COUNT
};

enum GameSoundID
{
	DEFAULT = -1,
	GAME_ATTRACT_SOUND,
	GAME_PLAYING_SOUND,
	GAME_AMBIENCE_SOUND,
	GAME_WALKING_SOUND,
	GAME_WALKING_STONE_SOUND,
	GAME_BUTTON_CLICK,
	GAME_TORCH_ATTACK,
	GAME_TORCH_ATTACK_OFF,
	GAME_PISTOL_FIRE,
	GAME_PLASMA_FIRE,
	GAME_PLASMA_HIT,
	GAME_DEMON_DEFAULT,
	GAME_DEMON_HURT,
	GAME_DEMON_DEATH,
	GAME_DEMON_ATTACK,
	GAME_PLAYER_HURT,
	GAME_PLAYER_DEATH,

	TOTAL_NUM_OF_SOUNDS
};

class Game
{
public:
	int					m_playerIndex					= 0;
	Camera*				m_screenCamera					= nullptr;
	std::vector<PlayerController*>	m_playerController;
	Clock*				m_gameClock						= nullptr;
	BitmapFont*			m_bitmapFont					= nullptr;

	int					m_numOfPlayers					= 0;

	SoundID				m_allSoundIDs[TOTAL_NUM_OF_SOUNDS];
	SoundPlaybackID		m_allSoundPlaybackIDs[TOTAL_NUM_OF_SOUNDS];

	float				m_thickness						= 1.0f;
	bool				m_isFreeFlyingMode				= false;
	bool				m_isLobby						= false;
	bool				m_isPlayMode					= false;

	GameState			m_currentState					= GameState::ATTRACT;
	GameState			m_nextState						= GameState::ATTRACT;

	Texture*			m_ogTexture						= nullptr;
	Texture*			m_thresholdTexture				= nullptr;
	Texture*			m_downSampledTexture			= nullptr;
	Texture*			m_upSampledTexture				= nullptr;
	Texture*			m_hBlurredTexture				= nullptr;
	Texture*			m_blurredTexture				= nullptr;
	Texture*            m_compositeTexture				= nullptr;
public:
						Game();
						~Game();

	void				StartUp();
	void				Shutdown();

	void				Update(float deltaseconds);
	void				Render() const ;

	void				HandleInput();

	void				InitializeSoundData();

	void				AttractScreenBloom();

	void				ConsoleControls();

	void				EnterState(GameState state);
	void				ExitState(GameState state);

	void				EnterAttract();
	void				EnterPlaying();

	void				ExitAttract();
	void				ExitPlaying();

	void				UpdateAttract(float deltaseconds);
	void				UpdatePlaying(float deltaseconds);

	void				RenderAttract() const;
	void				RenderPlaying() const;

};