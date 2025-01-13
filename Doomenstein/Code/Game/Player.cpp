#include "Game/Player.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Actor.hpp"
#include "Game/Map.hpp"

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <vector>

Player::Player()
{
	m_position = Vec3(-2.0f, 0.0f, 0.0f);
	m_orientationDegrees = EulerAngles(0.0f, 0.0f, 0.0f);
	m_worldCamera = new Camera();
}

Player::Player(Game* owner, Vec3 position)
	: m_game(owner)
{
	m_position = position;
	m_orientationDegrees = EulerAngles(0.0f, 0.0f, 0.0f);
	m_worldCamera = new Camera();

	//SpawnInfo playerInfo;
	//
	//playerInfo.m_actorDef = ActorDefinition::GetDefByName("Marine");
	//playerInfo.m_pos = Vec3(5.5f, 8.5f, 0.0625f);
	//playerInfo.m_orientation = EulerAngles(0.0f, 0.0f, 0.0f);
	//
	//m_playerActor = new Actor(*playerInfo.m_actorDef, playerInfo.m_pos, playerInfo.m_orientation, Rgba8::BLUE);
}

Player::~Player()
{
	DELETE_PTR(m_worldCamera);
	//DELETE_PTR(m_playerActor);
}

void Player::Update(float deltaseconds)
{
	m_velocity = Vec3(0.0f, 0.0f, 0.0f);
	m_angularVelocity = EulerAngles(0.0f, 0.0f, 0.0f);

	if(!g_theConsole->IsOpen())
		HandleInput();

	//if (!m_playerActor->IsMovable())
	//{
		m_position += m_velocity * deltaseconds;
	//}
	//else
	//{
	//	m_playerActor->Update(deltaseconds);
	//}

	m_orientationDegrees = EulerAngles(m_orientationDegrees.m_yawDegrees + m_angularVelocity.m_yawDegrees, m_orientationDegrees.m_pitchDegrees + m_angularVelocity.m_pitchDegrees, m_orientationDegrees.m_rollDegrees + m_angularVelocity.m_rollDegrees);
	m_orientationDegrees.m_pitchDegrees = GetClamped(m_orientationDegrees.m_pitchDegrees, -85.0f, 85.0f);
	m_orientationDegrees.m_rollDegrees = GetClamped(m_orientationDegrees.m_rollDegrees, -45.0f, 45.0f);

	m_worldCamera->SetTransform(m_position, m_orientationDegrees);
	m_worldCamera->SetRenderBasis(Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
	m_worldCamera->SetPerspectiveView(2.0f, 60.0f, 0.1f, 100.0f);
}

void Player::Render() const
{
	//m_playerActor->Render();
}

void Player::SetOrientation()
{
}

Mat44 Player::GetModelMatrix() const
{
	return Mat44();
}

void Player::HandleInput()
{
	m_angularVelocity.m_yawDegrees = 0.05f * g_theInputSystem->GetCursorClientDelta().x;
	m_angularVelocity.m_pitchDegrees = -0.05f * g_theInputSystem->GetCursorClientDelta().y;
	
	XboxController const& controller = g_theInputSystem->GetController(0);

	if (controller.GetLeftStick().GetMagnitude() > 0.0f)
	{
		m_velocity = (-2.0f * controller.GetLeftStick().GetPosition().x * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetJBasis3D()) + (2.0f * controller.GetLeftStick().GetPosition().y * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
	}

	if (controller.GetButton(BUTTON_RIGHT_SHOULDER).m_isPressed)
	{
		m_velocity = 2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D();
	}

	if (controller.GetButton(BUTTON_LEFT_SHOULDER).m_isPressed)
	{
		m_velocity = -2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D();
	}

	if (controller.GetButton(BUTTON_START).m_isPressed || g_theInputSystem->WasKeyJustPressed('H'))
	{
		m_position = Vec3(0.0f, 0.0f, 0.0f);
		m_orientationDegrees = EulerAngles(0.0f, 0.0f, 0.0f);
	}
	
	if (controller.GetRightStick().GetMagnitude() > 0.0f)
	{
		m_angularVelocity.m_pitchDegrees = -controller.GetRightStick().GetPosition().y;
		m_angularVelocity.m_yawDegrees = -controller.GetRightStick().GetPosition().x;
	}

	m_angularVelocity.m_rollDegrees = (-1.0f * controller.GetLeftTrigger()) + (1.0f * controller.GetRightTrigger());

	//if (g_theInputSystem->WasKeyJustPressed(KEYCODE_F1))
	//{
	//	//m_playerActor->ToggleStatic();
	//}

	//if (m_playerActor->IsMovable())
	//{
	//	m_playerActor->SetVelocity(Vec3::ZERO);
	//
	//	if (g_theInputSystem->IsKeyDown('W'))
	//	{
	//		m_playerActor->SetVelocity(2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown('S'))
	//	{
	//		m_playerActor->SetVelocity(-2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown('A'))
	//	{
	//		m_playerActor->SetVelocity(2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetJBasis3D());
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown('D'))
	//	{
	//		m_playerActor->SetVelocity(-2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetJBasis3D());
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown('Z'))
	//	{
	//		m_playerActor->SetVelocity(-2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown('C'))
	//	{
	//		m_playerActor->SetVelocity(2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D());
	//	}
	//
	//	if (controller.GetButton(BUTTON_A).m_isPressed || g_theInputSystem->IsKeyDown(KEYCODE_SHIFT))
	//	{
	//		m_playerActor->SetVelocity(m_velocity * 15.0f);
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown('Q'))
	//	{
	//		m_angularVelocity.m_rollDegrees = -0.5f;
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown('E'))
	//	{
	//		m_angularVelocity.m_rollDegrees = 0.5f;
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown(KEYCODE_UPARROW))
	//	{
	//		m_angularVelocity.m_pitchDegrees = -1.5f;
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown(KEYCODE_DOWNARROW))
	//	{
	//		m_angularVelocity.m_pitchDegrees = 1.5f;
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown(KEYCODE_LEFTARROW))
	//	{
	//		m_angularVelocity.m_yawDegrees = 1.5f;
	//	}
	//
	//	if (g_theInputSystem->IsKeyDown(KEYCODE_RIGHTARROW))
	//	{
	//		m_angularVelocity.m_yawDegrees = -1.5f;
	//	}
	//}
	//else
	{
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

		if (g_theInputSystem->IsKeyDown('Z'))
		{
			m_velocity = -2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D();
		}

		if (g_theInputSystem->IsKeyDown('C'))
		{
			m_velocity = 2.0f * m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetKBasis3D();
		}

		//if (controller.GetButton(BUTTON_A).m_isPressed || g_theInputSystem->IsKeyDown(KEYCODE_SHIFT))
		//{
		//	m_velocity *= 15.0f;
		//}

		if (g_theInputSystem->IsKeyDown('Q'))
		{
			m_angularVelocity.m_rollDegrees = -0.5f;
		}

		if (g_theInputSystem->IsKeyDown('E'))
		{
			m_angularVelocity.m_rollDegrees = 0.5f;
		}

		if (g_theInputSystem->IsKeyDown(KEYCODE_UPARROW))
		{
			m_angularVelocity.m_pitchDegrees = -1.5f;
		}

		if (g_theInputSystem->IsKeyDown(KEYCODE_DOWNARROW))
		{
			m_angularVelocity.m_pitchDegrees = 1.5f;
		}

		if (g_theInputSystem->IsKeyDown(KEYCODE_LEFTARROW))
		{
			m_angularVelocity.m_yawDegrees = 1.5f;
		}

		if (g_theInputSystem->IsKeyDown(KEYCODE_RIGHTARROW))
		{
			m_angularVelocity.m_yawDegrees = -1.5f;
		}
	}
}
