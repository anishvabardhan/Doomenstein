#pragma once

#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Core/Rgba8.hpp"

class Camera;
class Game;
class Actor;
class PlayerController;

class Player
{
public:
	Camera*			m_worldCamera		= nullptr;
	Vec3			m_position;
	Vec3			m_velocity;
	Rgba8			m_color				= Rgba8::WHITE;
	EulerAngles		m_orientationDegrees = EulerAngles(45.0f, 30.0f ,0.0f);
	EulerAngles		m_angularVelocity;
	Game*			m_game				= nullptr;
	//Actor*			m_playerActor		= nullptr;
	PlayerController* m_playerController = nullptr;
public:
	Player();
	Player(Game* owner, Vec3 position);
	~Player();

	void			HandleInput();
	void			Update(float deltaseconds);
	void			Render() const;

	void			SetOrientation();

	Mat44			GetModelMatrix() const;
};