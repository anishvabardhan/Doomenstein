#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/Tile.hpp"
#include "Game/SpawnInfo.hpp"
#include "Game/ActorUID.hpp"

#include <string>
#include <vector>

class Image;
class SpriteSheet;
class Shader;
class Game;
class VertexBuffer;
class IndexBuffer;
class Actor;

struct RaycastResultDoomenstein
{
	RaycastResult3D m_raycast;
	Actor* m_impactedActor = nullptr;
};

struct MapDefinition
{
	std::string					m_name						= "";
	std::string					m_image						= "";
	std::string					m_shaderName				= "";
	std::string					m_spriteSheetTexture		= "";
	IntVec2						m_spriteSheetCellCount		= IntVec2::ZERO;

	// SPAWN INFO
	std::vector<SpawnInfo>		m_spawnInfo;

	static MapDefinition		s_definitions[3];

	static void					InitializeDef();
};

class Map
{
public:
	MapDefinition				m_definition;
	std::vector<SpawnInfo>		m_spawnPoints;
	IntVec2						m_dimensions				= IntVec2::ZERO;
	std::vector<Tile>			m_tiles;
	Game*						m_game						= nullptr;
	SpriteSheet*				m_mapTerrainSpriteSheet		= nullptr;
	Image*						m_mapInfo					= nullptr;
	Shader*						m_shader					= nullptr;
	VertexBuffer*				m_vbo						= nullptr;
	IndexBuffer*				m_ibo						= nullptr;
	std::vector<Vertex_PCUTBN>	m_vertices;
	std::vector<unsigned int>	m_indices;
	RaycastResultDoomenstein	m_mapRaycast;
	std::vector<Actor*>			m_actorList;
	std::vector<Vec3>			m_pointLightPos;
	std::vector<Rgba8>			m_pointLightColor;
	static unsigned int const	MAX_ACTOR_SALT				= 0x0000fffeu;
	unsigned int				m_actorSalt					= MAX_ACTOR_SALT;
	float						m_spawnTimer				= 0.0f;
	float						m_spawnDuration				= 2.0f;
	int							m_maxAI						= 6;
	int							m_currentNumOfAI			= 0;

	Vec3						m_sunDirection;
	float						m_sunIntensity;
	float						m_ambientIntensity;
public:
								Map();
								Map(Game* owner, MapDefinition* definition);
								~Map();

	void						InitializeTiles();
	void						AddVertsForTile(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indices, int tileIndex, int& indicess, SpriteSheet const& spriteSheet) const;

	void						Update(float deltaseconds);
	void						Render(Camera cameraPosition) const;
	void						RenderActors(Camera cameraPosition) const;

	bool						IsPositionInBounds(Vec3 position, float const tolerance = 0.0f) const;
	bool						AreCoordsInBounds(int x, int y) const;
	Tile const*					GetTile(IntVec2 tile) const;

	void						SpawnPlayer();
	void						PossessPlayer(int playerIndex);
	void						SpawnActor(SpawnInfo info);
	void						SpawnActors();
	void						AttachAIControllers();
	Actor*						GetActorByUID(ActorUID const actorUID) const;

	void						CollideActors();
	void						CollideActors(Actor& actorA, Actor& actorB);
	void						CollideActorsWithMap();
	void						CollideActorWithMap(Actor& actor);

	void						DebugPossessNext();
	void						DeleteDestroyedActors();

	RaycastResultDoomenstein	RaycastAll(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResultDoomenstein	RaycastWorldXY(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResultDoomenstein	RaycastWorldZ(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResultDoomenstein	RaycastWorldActors(Vec3 const& start, Vec3 const& direction, float distance) const;
};