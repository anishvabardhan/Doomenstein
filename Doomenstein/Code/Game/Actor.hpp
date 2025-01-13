#pragma once

#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/SpriteAnimGroupDefinition.hpp"

#include "Game/ActorUID.hpp"

#include <string>
#include <vector>

class Weapon;
class Clock;
class Map;
class Controller;
class AIController;
class Timer;
class Shader;
class SpriteSheet;
class SpriteAnimDefinition;
class VertexBuffer;
class IndexBuffer;

struct ActorDefinition
{
	std::string								m_name;
	std::string								m_faction;
	float									m_health = 0;
	bool									m_canBePossessed = false;
	float									m_corpseLifetime = 0.0f;
	bool									m_visible = false;

	//Collision
	float									m_radius = 0.0f;
	float									m_height = 0.0f;
	bool									m_collidesWithWorld = false;
	bool									m_collidesWithActors = false;
	FloatRange								m_damageOnCollide;
	float									m_impulseOnCollide = 0.0f;
	bool									m_dieOnCollide = false;

	//Physics
	bool									m_simulated = false;
	float									m_walkSpeed = 0.0f;
	float									m_runSpeed = 0.0f;
	float									m_turnSpeed = 0.0f;
	bool									m_flying = false;
	float									m_drag;

	//Camera
	float									m_eyeHeight = 0.0f;
	float									m_cameraFOV = 0.0f;

	//AI
	bool									m_aiEnabled = false;;
	float									m_sightRadius = 0.0f;
	float									m_sightAngle = 0.0f;

	//Visuals
	Vec2									m_size;
	Vec2									m_pivot;
	std::string								m_billboardType;
	bool									m_renderLit;
	bool									m_renderRounded;
	std::string								m_shader;
	std::string								m_spriteSheet;
	IntVec2									m_cellCount;
		//AnimationGroup
	std::string								m_animGrpName;
	bool									m_scaleBySpeed = false;
	float									m_secondsPerFrame;
	std::string								m_playbackMode;	
	std::vector<SpriteAnimGroupDefinition>	m_spriteAnimGrpDefs;

	//Sound
	std::string								m_sound;
	std::string								m_soundName;

	//Inventory
	  //Weapon
	std::string								m_weaponName;

	static ActorDefinition					s_actorDefinitions[7];

	static ActorDefinition*					GetDefByName(std::string actorName);
	static void								InitializeProjectileDefs();
	static void								InitializeDefs();
};

class Actor
{
public:
	ActorUID					m_UID;
	ActorDefinition				m_definition;
	Map*						m_map					= nullptr;
	Vec3						m_position;
	Vec3						m_velocity;
	Vec3						m_accelaration;
	EulerAngles					m_orientation;
	Controller*					m_controller			= nullptr;
	AIController*				m_aiController			= nullptr;
	std::vector<Weapon*>		m_weapons;
	Actor*						m_projectileOwner		= nullptr;
	float						m_actorLifetime			= 0.0f;
	bool						m_isActorCorpse			= false;
	Rgba8						m_color;
	float						m_physicsHeight			= 0.0f;
	SpriteAnimGroupDefinition*	m_currentAnimGrp		= nullptr;
	float						m_physicsRadius			= 0.0f;
	bool						m_isMovable				= false;
	float						m_projectileLifetime	= 0.0f;
	bool						m_isProjectileDead		= false;
	bool						m_isActorProjectile		= false;
	bool						m_isActorEffect			= false;
	bool						m_isWeak				= false;
	bool						m_isDead				= false;
	float						m_health;
	std::string					m_animName				= "Walk";
	float						m_animTime				= 0.0f;
	float						m_animDuration			= 0.0f;
	VertexBuffer*				m_quadVBO				= nullptr;
	IndexBuffer*				m_quadIBO				= nullptr;
	Shader*						m_shader				= nullptr;
	SpriteSheet*				m_spriteSheet			= nullptr;
	SpriteAnimDefinition*		m_actorAnim				= nullptr;
public:
								Actor();
								Actor(ActorDefinition definition, Map* owner, Vec3 position, EulerAngles orientation, Rgba8 color);
								~Actor();

	void						AddQuadVertsAndIndices(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indices, AABB2 const& uvs = AABB2::ZERO_TO_ONE) const;

	void						Update(float deltaseconds, Camera cameraPosition);
	void						Render(Camera cameraPosition) const;

	void						PlayAnimation(Camera cameraPosition);
	void						RenderAnimation(Camera cameraPosition) const;

	void						RenderBillBoardQuad(Camera cameraPosition) const;

	Mat44						GetModelMatrix() const;
	bool						IsMovable();

	void						SetVelocity(Vec3 velocity);
	void						SetStatic(bool movable);
	void						ToggleStatic();

	void						UpdatePhysics(float deltaseconds);
	void						Damage(float damage);
	void						AddForce(Vec3 forceValue);
	void						AddImpulse(Vec3 forceValue);
	void						OnCollide(Actor* otherActor);
	void						OnPossessed(Controller* controller);
	void						OnUnpossessed(Controller* controller);
	void						MoveInDirection(Vec3 direction, float force);
	void						TurnInDirection(float degrees, float maxAngles);
	void						Attack();
	void						EquipWeapon();
};