#include "Game/Map.hpp"

#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

#include "Game/Actor.hpp"
#include "Game/Player.hpp"
#include "Game/PlayerController.hpp"
#include "Game/AIController.hpp"
#include "Game/Weapon.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"

#include <algorithm>

extern Map* g_currentMap;

MapDefinition MapDefinition::s_definitions[3];

Map::Map()
{
}

Map::Map(Game* owner, MapDefinition* definition)
	: m_definition(*definition)
{
	m_actorSalt = 0x00000000u;

	m_game = owner;

	m_mapInfo = new Image(m_definition.m_image.c_str());
	m_dimensions = m_mapInfo->GetDimensions();

	m_shader = g_theRenderer->CreateShader(m_definition.m_shaderName.c_str(), VertexType::PCUTBN);

	Texture* spriteTexture = g_theRenderer->CreateOrGetTextureFromFile(m_definition.m_spriteSheetTexture.c_str());

	m_mapTerrainSpriteSheet = new SpriteSheet(*spriteTexture, m_definition.m_spriteSheetCellCount);
	
	TileDefinition::InitializeDef();

	for (int i = 0; i < 10; i++)
	{
		m_pointLightPos.push_back(Vec3::ZERO);
		m_pointLightColor.push_back(Rgba8::BLACK);
	}

	InitializeTiles();

	int indicess = 0;

	for (int index = 0; index < m_dimensions.x * m_dimensions.y; index++)
	{
		AddVertsForTile(m_vertices, m_indices, index, indicess, *m_mapTerrainSpriteSheet);
	}

	m_vbo = g_theRenderer->CreateVertexBuffer(m_vertices.size() * sizeof(Vertex_PCUTBN));
	g_theRenderer->CopyCPUToGPU(m_vertices.data(), m_vertices.size() * sizeof(Vertex_PCUTBN), m_vbo);
	g_theRenderer->BindVertexBuffer(m_vbo, 0);
	
	m_ibo = g_theRenderer->CreateIndexBuffer(m_indices.size() * sizeof(unsigned int));
	g_theRenderer->CopyCPUToGPU(m_indices.data(), m_indices.size() * sizeof(unsigned int), m_ibo);
	g_theRenderer->BindIndexBuffer(m_ibo);

	m_sunDirection = Vec3(2.0f, 1.0f, -1.0f);
	m_sunIntensity = 0.1f;
	m_ambientIntensity = 0.1f;

	for (size_t index = 0; index < MapDefinition::s_definitions[2].m_spawnInfo.size(); index++)
	{
		if (MapDefinition::s_definitions[0].m_spawnInfo[index].m_actorDef->m_name == "SpawnPoint")
		{
			m_spawnPoints.push_back(MapDefinition::s_definitions[2].m_spawnInfo[index]);
		}
	}

	SpawnActors();
	SpawnPlayer();
}

Map::~Map()
{
	for (size_t index = 0; index < m_actorList.size(); index++)
	{
		if (m_actorList[index])
		{
			DELETE_PTR(m_actorList[index]);
		}
	}

	m_actorList.clear();

	DELETE_PTR(m_mapTerrainSpriteSheet);
	DELETE_PTR(m_mapInfo);
	DELETE_PTR(m_shader);
	DELETE_PTR(m_vbo);
	DELETE_PTR(m_ibo);
}

void Map::InitializeTiles()
{
	int lightIndex = 0;

	for (int index = 0; index < m_dimensions.x * m_dimensions.y; index++)
	{
		Rgba8 texelData = m_mapInfo->m_rgbaTexels[index];

		int tileX = static_cast<int>(index) % m_dimensions.x;
		int tileY = static_cast<int>(index) / m_dimensions.x;

		if (texelData.r == 0 && texelData.g == 0 && texelData.b == 0 && texelData.a == 255)
		{
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[3]));
		}
		else if (texelData.r == 138 && texelData.g == 111 && texelData.b == 48 && texelData.a == 255)
		{
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[5]));
		}
		else if (texelData.r == 200 && texelData.g == 200 && texelData.b == 200 && texelData.a == 255)
		{
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[4]));
		}
		else if (texelData.r == 100 && texelData.g == 100 && texelData.b == 100 && texelData.a == 255)
		{
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[6]));
		}
		else if (texelData.r == 255 && texelData.g == 255 && texelData.b == 255 && texelData.a == 255)
		{
			m_pointLightPos[lightIndex] = Vec3(tileX + 0.5f, tileY + 0.5f, 1.5f);
			m_pointLightColor[lightIndex] = Rgba8::WHITE;
			lightIndex++;
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[7]));
		}
		else if (texelData.r == 200 && texelData.g == 0 && texelData.b == 0 && texelData.a == 255)
		{
			m_pointLightPos[lightIndex] = Vec3(tileX + 0.5f, tileY + 0.5f, 1.5f);
			m_pointLightColor[lightIndex] = Rgba8::RED;
			lightIndex++;
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[8]));
		}
		else if (texelData.r == 0 && texelData.g == 200 && texelData.b == 0 && texelData.a == 255)
		{
			m_pointLightPos[lightIndex] = Vec3(tileX + 0.5f, tileY + 0.5f, 1.5f);
			m_pointLightColor[lightIndex] = Rgba8::GREEN;
			lightIndex++;
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[9]));
		}
		else if (texelData.r == 0 && texelData.g == 0 && texelData.b == 200 && texelData.a == 255)
		{
			m_pointLightPos[lightIndex] = Vec3(tileX + 0.5f, tileY + 0.5f, 1.5f);
			m_pointLightColor[lightIndex] = Rgba8::BLUE;
			lightIndex++;
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[10]));
		}
		else if (texelData.r == 0 && texelData.g == 150 && texelData.b == 0 && texelData.a == 255)
		{
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[11]));
		}
		else if (texelData.r == 0 && texelData.g == 100 && texelData.b == 0 && texelData.a == 255)
		{
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[12]));
		}
		else
		{
			m_tiles.push_back(Tile(IntVec2(tileX, tileY), &TileDefinition::s_definitions[4]));
		}
	}
}

void Map::AddVertsForTile(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indices, int tileIndex, int& indicess, SpriteSheet const& spriteSheet) const
{
	AABB3 tileBounds = m_tiles[tileIndex].GetBounds();

	if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[5])
	{
		int floorSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_floorSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_floorSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& floorSpriteDef = spriteSheet.GetSpriteDef(floorSpriteIndex);

		Vec2 uvMins, uvMaxs;
		floorSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));

		indicess += 4;
	}
	else if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[3] || m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[6])
	{
		int wallSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& wallSpriteDef = spriteSheet.GetSpriteDef(wallSpriteIndex);

		Vec2 uvMins, uvMaxs;
		wallSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), tileBounds.m_maxs, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), tileBounds.m_maxs, Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), tileBounds.m_mins, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), tileBounds.m_maxs, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	}
	else if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[4])
	{
		int floorSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_floorSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_floorSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& floorSpriteDef = spriteSheet.GetSpriteDef(floorSpriteIndex);

		Vec2 uvMins, uvMaxs;
		floorSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));

		indicess += 4;
	}
	else if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[7])
	{
		int wallSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& wallSpriteDef = spriteSheet.GetSpriteDef(wallSpriteIndex);

		Vec2 uvMins, uvMaxs;
		wallSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;																																																																			  
																																																																								  
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;																																																																			  
																																																																								  
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;																																																																			  
																																																																								  
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;																																																																			 
																																																																								 
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;																																																																			  
																																																																								  
		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	}
	else if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[8])
	{
		int wallSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& wallSpriteDef = spriteSheet.GetSpriteDef(wallSpriteIndex);

		Vec2 uvMins, uvMaxs;
		wallSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::RED, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::RED, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::RED, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::RED, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::RED, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Rgba8::RED, AABB2(uvMins, uvMaxs));
		indicess += 4;
	}
	else if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[9])
	{
		int wallSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& wallSpriteDef = spriteSheet.GetSpriteDef(wallSpriteIndex);

		Vec2 uvMins, uvMaxs;
		wallSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::GREEN, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::GREEN, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::GREEN, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::GREEN, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::GREEN, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Rgba8::GREEN, AABB2(uvMins, uvMaxs));
		indicess += 4;
	}
	else if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[10])
	{
		int wallSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_wallSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);
		
		SpriteDefinition const& wallSpriteDef = spriteSheet.GetSpriteDef(wallSpriteIndex);
		
		Vec2 uvMins, uvMaxs;
		wallSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::BLUE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::BLUE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::BLUE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::BLUE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::BLUE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Rgba8::BLUE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	}
	else if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[11])
	{
		int wallSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_ceilSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_ceilSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);
	
		SpriteDefinition const& wallSpriteDef = spriteSheet.GetSpriteDef(wallSpriteIndex);
	
		Vec2 uvMins, uvMaxs;
		wallSpriteDef.GetUVs(uvMins, uvMaxs);
	
		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), tileBounds.m_maxs, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), tileBounds.m_maxs, Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), tileBounds.m_mins, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		int floorSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_floorSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_floorSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& floorSpriteDef = spriteSheet.GetSpriteDef(floorSpriteIndex);

		floorSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	}
	else if (m_tiles[tileIndex].GetDefinition() == &TileDefinition::s_definitions[12])
	{
		int barkSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_floorSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_floorSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& barkSpriteDef = spriteSheet.GetSpriteDef(barkSpriteIndex);

		Vec2 uvMins, uvMaxs;
		barkSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, tileBounds.m_mins, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_mins.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), tileBounds.m_maxs, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), tileBounds.m_maxs, Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_mins.z), tileBounds.m_mins, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	
		int treeSpriteIndex = m_tiles[tileIndex].GetDefinition()->m_ceilSpriteCoords.x + (m_tiles[tileIndex].GetDefinition()->m_ceilSpriteCoords.y * m_definition.m_spriteSheetCellCount.x);

		SpriteDefinition const& treeSpriteDef = spriteSheet.GetSpriteDef(treeSpriteIndex);

		treeSpriteDef.GetUVs(uvMins, uvMaxs);

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 2), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 2), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 2), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 2), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 2), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 2), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 2), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 2), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 2), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 2), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 2), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 2), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		//
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y - 1, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		//
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y + 1, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		//
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x + 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		//
		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x - 1, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x - 1, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x - 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x - 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x - 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x - 1, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x - 1, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x - 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x - 1, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Vec3(tileBounds.m_mins.x - 1, tileBounds.m_maxs.y, tileBounds.m_maxs.z + 1), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;

		AddVertsForQuad3D(verts, indices, indicess, Vec3(tileBounds.m_mins.x - 1, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, tileBounds.m_maxs.z), Vec3(tileBounds.m_mins.x - 1, tileBounds.m_mins.y, tileBounds.m_maxs.z), Rgba8::WHITE, AABB2(uvMins, uvMaxs));
		indicess += 4;
	}
}

void Map::Update(float deltaseconds)
{
	for (int i = 0; i < m_game->m_numOfPlayers; i++)
	{
		m_game->m_playerController[i]->m_isShooting = false;
	}
	
	if (m_currentNumOfAI <= m_maxAI)
	{
		m_spawnTimer += deltaseconds;

		if (m_spawnTimer > m_spawnDuration)
		{
			RandomNumberGenerator random = RandomNumberGenerator();

			int flag = random.RollRandomIntInRange(0, 2);

			if (flag == 0)
			{
				SpawnInfo redEnemy;
				redEnemy.m_actorDef = &ActorDefinition::s_actorDefinitions[4];
				redEnemy.m_pos = MapDefinition::s_definitions[2].m_spawnInfo[flag].m_pos;

				SpawnActor(redEnemy);
				m_currentNumOfAI++;
			}
			else if (flag == 1)
			{
				SpawnInfo greenEnemy;
				greenEnemy.m_actorDef = &ActorDefinition::s_actorDefinitions[5];
				greenEnemy.m_pos = MapDefinition::s_definitions[2].m_spawnInfo[flag].m_pos;

				SpawnActor(greenEnemy);
				m_currentNumOfAI++;
			}
			else if (flag == 2)
			{
				SpawnInfo blueEnemy;
				blueEnemy.m_actorDef = &ActorDefinition::s_actorDefinitions[6];
				blueEnemy.m_pos = MapDefinition::s_definitions[2].m_spawnInfo[flag].m_pos;

				SpawnActor(blueEnemy);
				m_currentNumOfAI++;
			}

			AttachAIControllers();

			m_spawnTimer = 0.0f;
		}
	}

	if (g_theInputSystem->WasKeyJustPressed('N'))
	{
		DebugPossessNext();
	}

	m_sunIntensity = GetClamped(m_sunIntensity, 0.0f, 0.5f);
	m_ambientIntensity = GetClamped(m_ambientIntensity, 0.0f, 0.5f);

	for (int i = 0; i < m_game->m_numOfPlayers; i++)
	{
		if (m_game->m_playerController[i] != nullptr && m_game->m_playerController[i]->m_actorUID != ActorUID::INVALID && m_game->m_playerController[i]->GetActor() != nullptr)
		{
			if (m_game->m_playerController[i]->GetActor()->m_definition.m_name == "Marine")
			{
				//if (m_game->m_playerController[i]->m_isControllerInput)
				//{
				//	XboxController const& controller = g_theInputSystem->GetController(0);
				//
				//	if (controller.GetRightTrigger() > 0.0f)
				//	{
				//		m_game->m_playerController[i]->m_isShooting = true;
				//		m_game->m_playerController[i]->GetActor()->m_animName = "Attack";
				//	}
				//	else
				//	{
				//		m_game->m_playerController[i]->GetActor()->m_animName = "Walk";
				//	}
				//}
				//else
				{
					if (g_theInputSystem->IsKeyDown(KEYCODE_LEFT_MOUSE))
					{
						m_game->m_playerController[i]->m_isShooting = true;
						m_game->m_playerController[i]->GetActor()->m_animName = "Attack";
					}
					else
					{
						m_game->m_playerController[i]->GetActor()->m_animName = "Walk";
					}
				}
			}
		}
	}

	if (m_game->m_isFreeFlyingMode)
	{
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
		{
			m_mapRaycast = g_currentMap->RaycastAll(m_game->m_playerController[0]->m_position, m_game->m_playerController[0]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), 10.0f);

			DebugAddWorldPoint(m_mapRaycast.m_raycast.m_impactPos, 0.06f, 10.0f);
			DebugAddWorldArrow(m_mapRaycast.m_raycast.m_impactPos, m_mapRaycast.m_raycast.m_impactPos + (m_mapRaycast.m_raycast.m_impactNormal * 0.3f), 0.03f, 10.0f, Rgba8::BLUE, Rgba8::BLUE);

			DebugAddWorldLine(m_mapRaycast.m_raycast.m_rayStartPos, m_mapRaycast.m_raycast.m_rayStartPos + (m_mapRaycast.m_raycast.m_rayFwdNormal * m_mapRaycast.m_raycast.m_rayMaxLength), 0.01f, 10.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
		}

		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			m_mapRaycast = g_currentMap->RaycastAll(m_game->m_playerController[0]->m_position, m_game->m_playerController[0]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), 0.25f);
			
			DebugAddWorldPoint(m_mapRaycast.m_raycast.m_impactPos, 0.06f, 10.0f);
			DebugAddWorldArrow(m_mapRaycast.m_raycast.m_impactPos, m_mapRaycast.m_raycast.m_impactPos + (m_mapRaycast.m_raycast.m_impactNormal * 0.3f), 0.03f, 10.0f, Rgba8::BLUE, Rgba8::BLUE);

			DebugAddWorldLine(m_mapRaycast.m_raycast.m_rayStartPos, m_mapRaycast.m_raycast.m_rayStartPos + (m_mapRaycast.m_raycast.m_rayFwdNormal * m_mapRaycast.m_raycast.m_rayMaxLength), 0.01f, 10.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
		}
	}
	else
	{
		for (int i = 0; i < m_game->m_numOfPlayers; i++)
		{
			if (m_game->m_playerController[i] && m_game->m_playerController[i]->m_actorUID != ActorUID::INVALID && m_game->m_playerController[i]->GetActor())
			{
				if (m_game->m_playerController[i]->m_isShooting)
				{
					float timer = m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_refireTime;
		
					if (m_game->m_playerController[i]->m_nextProjectileTimer == 0.0f)
					{
						int x = RoundDownToInt(m_game->m_playerController[0]->m_position.x);
						int y = RoundDownToInt(m_game->m_playerController[0]->m_position.y);

						int tileIndex = x + (y * m_dimensions.x);

						Vec2 pos = Vec2(m_game->m_playerController[0]->m_position.x, m_game->m_playerController[0]->m_position.y);

						if (m_tiles[tileIndex].GetDefinition()->m_name != "RockFloor")
						{
							m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->Fire();
						}
		
						//if (m_game->m_playerController[i]->m_equippedWeaponIndex == 0)
						//{
						//	if (m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor)
						//	{
						//		RandomNumberGenerator random = RandomNumberGenerator();
						//
						//		m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->Damage(random.RollRandomFloatInRange(WeaponDefinition::s_weaponDefinitions[0].m_rayDamage.m_min, WeaponDefinition::s_weaponDefinitions[0].m_rayDamage.m_max));
						//
						//		SpawnInfo bloodHit;
						//		bloodHit.m_actorDef = &ActorDefinition::s_actorDefinitions[3];
						//		bloodHit.m_pos = m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_raycast.m_impactPos;
						//
						//		SpawnActor(bloodHit);
						//	}
						//	else
						//	{
						//		SpawnInfo bulletHit;
						//		bulletHit.m_actorDef = &ActorDefinition::s_actorDefinitions[2];
						//		bulletHit.m_pos = m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_raycast.m_impactPos;
						//
						//		SpawnActor(bulletHit);
						//	}
						//}
					}
		
					m_game->m_playerController[i]->m_nextProjectileTimer += deltaseconds;
		
					if (m_game->m_playerController[i]->m_nextProjectileTimer >= timer)
						m_game->m_playerController[i]->m_nextProjectileTimer = 0.0f;
				}
				else
				{
					m_game->m_playerController[i]->m_nextProjectileTimer = 0.0f;
				}
			}
		}
	}

	for (int i = 0; i < m_game->m_numOfPlayers; i++)
	{
		if (m_game->m_playerController[i]->m_isPistolHit)
		{
			if (m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->m_aiController)
			{
				RandomNumberGenerator random = RandomNumberGenerator();

				m_game->m_allSoundPlaybackIDs[GAME_DEMON_HURT] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_DEMON_HURT], m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->m_position);

				m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->m_aiController->DamagedBy(m_game->m_playerController[i]->GetActor());
				
				if (m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->m_isWeak)
				{
					m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->Damage(random.RollRandomFloatInRange(m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_rayDamage.m_min, m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_rayDamage.m_max));
				}
				else
				{
					m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->Damage(0.5f * random.RollRandomFloatInRange(m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_rayDamage.m_min, m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_rayDamage.m_max));
				}
				
				m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->AddImpulse(m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_rayImpulse* m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_raycast.m_rayFwdNormal * 2.0f);
				m_game->m_playerController[i]->m_isPistolHit = false;
			}
			else
			{
				RandomNumberGenerator random = RandomNumberGenerator();

				m_game->m_allSoundPlaybackIDs[GAME_PLAYER_HURT] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_PLAYER_HURT], m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->m_position);

				m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->Damage(random.RollRandomFloatInRange(m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_rayDamage.m_min, m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_rayDamage.m_max));
				m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_impactedActor->AddImpulse(m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_definition.m_rayImpulse* m_game->m_playerController[i]->GetActor()->m_weapons[m_game->m_playerController[i]->m_equippedWeaponIndex]->m_rayFireCast.m_raycast.m_rayFwdNormal * 2.0f);
				m_game->m_playerController[i]->m_isPistolHit = false;
			}
		}
	}

	for (size_t index = 0; index < m_actorList.size(); index++)
	{
		for (int i = 0; i < m_game->m_numOfPlayers; i++)
		{
			if (m_game->m_playerController[i] != nullptr && m_game->m_playerController[i]->m_actorUID != ActorUID::INVALID && m_actorList[index] != nullptr)
			{
				if (m_actorList[index] != m_game->m_playerController[i]->GetActor())
				{
					m_actorList[index]->Update(deltaseconds, *m_game->m_playerController[i]->m_worldCamera);
				}
			}
		}
	}

	if (m_game->m_playerController[0]->m_equippedWeaponIndex == 1)
	{
		RaycastResultDoomenstein raycast = RaycastAll(m_game->m_playerController[0]->m_position, m_game->m_playerController[0]->m_orientationDegrees.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D(), 1.5f);

		if (raycast.m_raycast.m_didImpact)
		{
			if (raycast.m_impactedActor)
			{
				if (raycast.m_impactedActor->m_color.r == m_game->m_playerController[0]->m_torchColor.r && raycast.m_impactedActor->m_color.g == m_game->m_playerController[0]->m_torchColor.g && raycast.m_impactedActor->m_color.b == m_game->m_playerController[0]->m_torchColor.b && raycast.m_impactedActor->m_color.a == m_game->m_playerController[0]->m_torchColor.a)
				{
					if (g_theInputSystem->IsKeyDown(KEYCODE_LEFT_MOUSE))
					{
						raycast.m_impactedActor->m_isWeak = true;
					}
				}
			}
		}
	}

	CollideActors();
	CollideActorsWithMap();

	DeleteDestroyedActors();
}

void Map::Render(Camera cameraPosition) const
{
	std::vector<Vertex_PCU> moonVerts;

	AddVertsForQuad3D(moonVerts, Vec3(0.0f, -50.0f, -50.0f), Vec3(0.0f, 50.0f, -50.0f), Vec3(0.0, 50.0f, 50.0f), Vec3(0.0f, -50.0f, 50.0f), Rgba8::WHITE, Rgba8::WHITE);

	Mat44 transform = GetBillboardMatrix(BillboardType::FULL_CAMERA_FACING, m_game->m_playerController[0]->m_worldCamera->GetModelMatrix(), Vec3(400.0f, 400.0f, 400.0f));

	g_theRenderer->SetModelConstants(transform);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->BindTexture(m_game->m_compositeTexture);
	g_theRenderer->BindShader();
	g_theRenderer->DrawVertexArray((int)moonVerts.size(), moonVerts.data());

	std::vector<Vertex_PCU> skyBoxVerts;
	
	Texture* skyBox = g_theRenderer->CreateOrGetTextureFromFile("Data/Textures/Background.png");
	
	AddVertsForAABB3D(skyBoxVerts, AABB3(-500.0f, -500.0f, -0.5f, 500.0f, 500.0f, 500.0f));
	
	g_theRenderer->SetModelConstants(Mat44(), Rgba8(100, 100, 100, 255));
	
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(skyBox);
	g_theRenderer->BindShader();
	g_theRenderer->DrawVertexArray((int)skyBoxVerts.size(), skyBoxVerts.data());

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetPointLightConstants(m_pointLightPos, m_pointLightColor);

	if (m_game->m_playerController[0]->m_batteryLife > 0.0f)
	{
		std::vector<Vec3> lightPos;
		std::vector<float> lightCutOff;
		std::vector<Vec3> lightDirection;
		std::vector<Rgba8> lightColor;

		lightPos.push_back(m_game->m_playerController[0]->m_torchPosition);
		lightPos.push_back(m_game->m_playerController[0]->m_torchPosition);

		lightCutOff.push_back(m_game->m_playerController[0]->m_torchCutoff);
		lightCutOff.push_back(0.5f);

		lightDirection.push_back(m_game->m_playerController[0]->m_torchForward);
		lightDirection.push_back(m_game->m_playerController[0]->m_torchForward);

		lightColor.push_back(m_game->m_playerController[0]->m_torchColor);
		lightColor.push_back(Rgba8::WHITE);

		g_theRenderer->SetSpotLightConstants(lightPos, lightCutOff, lightDirection, lightColor);
	}
	else
	{
		std::vector<Vec3> lightPos;
		std::vector<float> lightCutOff;
		std::vector<Vec3> lightDirection;
		std::vector<Rgba8> lightColor;

		lightPos.push_back(Vec3::ZERO);
		lightPos.push_back(Vec3::ZERO);

		lightCutOff.push_back(0.0f);
		lightCutOff.push_back(0.0f);

		lightDirection.push_back(Vec3::ZERO);
		lightDirection.push_back(Vec3::ZERO);

		lightColor.push_back(Rgba8::BLACK);
		lightColor.push_back(Rgba8::BLACK);

		g_theRenderer->SetSpotLightConstants(lightPos, lightCutOff, lightDirection, lightColor);
	}

	g_theRenderer->SetDirectionalLightConstants(m_sunDirection, m_sunIntensity, m_ambientIntensity);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&m_mapTerrainSpriteSheet->GetTexture());
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->DrawVertexBufferIndexed(m_vbo, m_ibo, 60);

	RenderActors(cameraPosition);
}

void Map::RenderActors(Camera cameraPosition) const
{
	for (size_t index = 0; index < m_actorList.size(); index++)
	{
		if (m_game->m_playerController[0]->m_actorUID != ActorUID::INVALID)
		{
			if (m_actorList[index] && m_actorList[index] != m_game->m_playerController[0]->GetActor())
			{
				m_actorList[index]->Render(cameraPosition);
			}
		}
	}
}

void MapDefinition::InitializeDef()
{
	XmlDocument tileDoc;

	XmlError result = tileDoc.LoadFile("Data/Definitions/MapDefinitions.xml");

	if (result != tinyxml2::XML_SUCCESS)
		GUARANTEE_OR_DIE(false, "COULD NOT LOAD XML");

	XmlElement* element = tileDoc.RootElement()->FirstChildElement();

	for (int index = 0; index < 3; index++)
	{
		s_definitions[index].m_name = ParseXmlAttribute(*element, "name", s_definitions[index].m_name);
		s_definitions[index].m_image = ParseXmlAttribute(*element, "image", s_definitions[index].m_image);
		s_definitions[index].m_shaderName = ParseXmlAttribute(*element, "shader", s_definitions[index].m_shaderName);
		s_definitions[index].m_spriteSheetTexture = ParseXmlAttribute(*element, "spriteSheetTexture", s_definitions[index].m_spriteSheetTexture);
		s_definitions[index].m_spriteSheetCellCount = ParseXmlAttribute(*element, "spriteSheetCellCount", s_definitions[index].m_spriteSheetCellCount);

		XmlElement* spawnElement = element->FirstChildElement()->FirstChildElement();

		while (spawnElement)
		{
			SpawnInfo spawnInfo;
			
			ActorDefinition* actorDef = ActorDefinition::GetDefByName(ParseXmlAttribute(*spawnElement, "actor", ""));

			spawnInfo.m_actorDef = actorDef;
			spawnInfo.m_pos = ParseXmlAttribute(*spawnElement, "position", Vec3());
			spawnInfo.m_orientation = ParseXmlAttribute(*spawnElement, "orientation", EulerAngles());

			s_definitions[index].m_spawnInfo.push_back(spawnInfo);

			spawnElement = spawnElement->NextSiblingElement();
		}

		element = element->NextSiblingElement();
	}
}

bool Map::IsPositionInBounds(Vec3 position, float const tolerance) const
{
	UNUSED(tolerance);

	if (position.z > 0.0f && position.z < 1.0f)
	{
		int x = RoundDownToInt(position.x);
		int y = RoundDownToInt(position.y);

		if (x < 0 || x > m_dimensions.x || y < 0 || y > m_dimensions.y)
			return false;

		int tileIndex = x + (y * m_dimensions.x);

		if (m_tiles[tileIndex].m_definition->m_isSolid)
		{
			return true;
		}
	}

	return false;
}

bool Map::AreCoordsInBounds(int x, int y) const
{
	if(x < 0 || x > m_dimensions.x || y < 0 || y > m_dimensions.y)
		return false;

	int tileIndex = x + (y * m_dimensions.x);

	if (m_tiles[tileIndex].m_definition->m_isSolid)
	{
		return true;
	}

	return false;
}

Tile const* Map::GetTile(IntVec2 tile) const
{
	int tileIndex = tile.x + (tile.y * m_dimensions.x);

	return &m_tiles[tileIndex];
}

void Map::SpawnPlayer()
{
	RandomNumberGenerator random = RandomNumberGenerator();

	int spawnPointIndex = random.RollRandomIntInRange(0, (int)m_spawnPoints.size() - 1);

	Actor* actor = new Actor(ActorDefinition::s_actorDefinitions[1], this, Vec3(2.5f, 2.5f, 0.0f), m_spawnPoints[spawnPointIndex].m_orientation, Rgba8::GREEN);

	for (size_t i = 0; i < m_actorList.size(); i++)
	{
		if (m_actorList[i] == nullptr)
		{
			actor->m_UID = ActorUID(m_actorSalt, (unsigned int)i);

			m_actorList[i] = actor;
			m_actorSalt++;
			return;
		}
	}

	actor->m_UID = ActorUID(m_actorSalt, (int)m_actorList.size());
	m_actorList.push_back(actor);
	m_actorSalt++;
}

void Map::PossessPlayer(int playerIndex)
{
	for (size_t index = 0; index < m_actorList.size(); index++)
	{
		if (m_actorList[index])
		{
			if (m_actorList[index]->m_definition.m_name == "Marine" && m_actorList[index]->m_controller == nullptr && m_game->m_playerController[playerIndex]->m_actorUID == ActorUID::INVALID)
			{
				m_game->m_playerController[playerIndex]->Possess(m_actorList[index]);
			}
		}
	}
}

void Map::SpawnActor(SpawnInfo info)
{
	Actor* actor = new Actor(*info.m_actorDef, this, info.m_pos, info.m_orientation, Rgba8::WHITE);

	if (info.m_actorDef->m_name == "RedGhost")
	{
		actor->m_color = Rgba8::RED;
	}
	else if (info.m_actorDef->m_name == "GreenGhost")
	{
		actor->m_color = Rgba8::GREEN;
	}
	else if (info.m_actorDef->m_name == "BlueGhost")
	{
		actor->m_color = Rgba8::BLUE;
	}

	for (size_t i = 0; i < m_actorList.size(); i++)
	{
		if (m_actorList[i] == nullptr)
		{
			actor->m_UID = ActorUID(m_actorSalt, (unsigned int)i);

			m_actorList[i] = actor;
			m_actorSalt++;

			return;
		}
	}

	actor->m_UID = ActorUID(m_actorSalt, (unsigned int)m_actorList.size());

	m_actorList.push_back(actor);
	m_actorSalt++;
}

void Map::SpawnActors()
{
	for (size_t index = 0; index < MapDefinition::s_definitions[2].m_spawnInfo.size(); index++)
	{
		RandomNumberGenerator random = RandomNumberGenerator();

		int flag = random.RollRandomIntInRange(0, 2);

		if (flag == 0)
		{
			SpawnInfo redEnemy;
			redEnemy.m_actorDef = &ActorDefinition::s_actorDefinitions[4];
			redEnemy.m_pos = MapDefinition::s_definitions[2].m_spawnInfo[index].m_pos;

			SpawnActor(redEnemy);
			m_currentNumOfAI++;
		}
		else if (flag == 1)
		{
			SpawnInfo greenEnemy;
			greenEnemy.m_actorDef = &ActorDefinition::s_actorDefinitions[5];
			greenEnemy.m_pos = MapDefinition::s_definitions[2].m_spawnInfo[index].m_pos;

			SpawnActor(greenEnemy);
			m_currentNumOfAI++;
		}
		else if (flag == 2)
		{
			SpawnInfo blueEnemy;
			blueEnemy.m_actorDef = &ActorDefinition::s_actorDefinitions[6];
			blueEnemy.m_pos = MapDefinition::s_definitions[2].m_spawnInfo[index].m_pos;

			SpawnActor(blueEnemy);
			m_currentNumOfAI++;
		}
	}
}

//void Map::SpawnProjectile()
//{
//	for (int i = 0; i < m_game->m_numOfPlayers; i++)
//	{
//		if (m_game->m_playerController[i]->m_isShooting && m_game->m_playerController[i]->m_equippedWeaponIndex == 1)
//		{
//			Vec3 spawnPos = Vec3(m_game->m_playerController[i]->GetActor()->m_position.x, m_game->m_playerController[i]->GetActor()->m_position.y, m_game->m_playerController[i]->GetActor()->m_position.z + (m_game->m_playerController[i]->GetActor()->m_definition.m_eyeHeight * 0.5f)) + (m_game->m_playerController[i]->GetActor()->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D() * 0.5f);
//
//			Actor* actor = new Actor(ActorDefinition::s_actorDefinitions[5], this, spawnPos, m_game->m_playerController[i]->GetActor()->m_orientation, Rgba8::CYAN);
//			
//			for (size_t j = 0; j < m_actorList.size(); j++)
//			{
//				if (m_actorList[j] == nullptr)
//				{
//					actor->m_UID = ActorUID(m_actorSalt, (unsigned int)j);
//
//					m_actorList[j] = actor;
//					m_actorSalt++;
//
//					return;
//				}
//			}
//
//			actor->m_UID = ActorUID(m_actorSalt, (unsigned int)m_actorList.size());
//
//			m_actorList.push_back(actor);
//			m_actorSalt++;
//		}
//	}
//}

void Map::AttachAIControllers()
{
	for (size_t index = 0; index < m_actorList.size(); index++)
	{
		if (m_actorList[index] && m_actorList[index]->m_aiController == nullptr)
		{
			if (m_actorList[index]->m_definition.m_name == "RedGhost" || m_actorList[index]->m_definition.m_name == "GreenGhost" || m_actorList[index]->m_definition.m_name == "BlueGhost" || m_actorList[index]->m_definition.m_aiEnabled)
			{
				m_actorList[index]->m_aiController = new AIController();
				m_actorList[index]->m_aiController->m_actorUID = m_actorList[index]->m_UID;
				m_actorList[index]->m_aiController->m_map = this;
			}
		}
	}
}

Actor* Map::GetActorByUID(ActorUID const actorUID) const
{
	return m_actorList[actorUID.GetIndex()];
}

void Map::CollideActors()
{
	for (size_t i = 0; i < m_actorList.size(); i++)
	{
		if (m_actorList[i] != nullptr)
		{
			for (int j = 0; j < m_game->m_numOfPlayers; j++)
			{
				if (m_game->m_playerController[j] != nullptr && m_game->m_playerController[j]->m_actorUID != ActorUID::INVALID && m_game->m_playerController[j]->m_actorUID.GetActor() != nullptr)
				{
					if (m_game->m_playerController[j]->m_actorUID != m_actorList[i]->m_UID)
					{
						if (!m_game->m_playerController[j]->m_actorUID.GetActor()->m_isActorCorpse)
						{
							CollideActors(*m_game->m_playerController[j]->m_actorUID.GetActor(), *m_actorList[i]);
						}
					}
				}
			}

			for (size_t j = i + 1; j < m_actorList.size(); j++)
			{
				if (m_actorList[j] != nullptr)
				{
					CollideActors(*m_actorList[i], *m_actorList[j]);
				}
			}
		}
	}
}

void Map::CollideActors(Actor& actorA, Actor& actorB)
{
	if (actorA.m_UID.GetActor() != nullptr && actorB.m_UID.GetActor() != nullptr)
	{
		if (actorA.m_definition.m_collidesWithActors && actorB.m_definition.m_collidesWithActors)
		{
			if (!actorA.m_isActorCorpse && !actorB.m_isActorCorpse)
			{
				//if (!actorA.m_isDead && !actorA.m_isActorCorpse && actorA.m_definition.m_name == "PlasmaProjectile")
				//{
				//	if (&actorB != actorA.m_projectileOwner && actorB.m_definition.m_name != actorA.m_definition.m_name)
				//	{
				//		if (actorB.m_UID != actorA.m_projectileOwner->m_UID)
				//		{
				//			if (DoZCylindersOverlap(actorA.m_position, actorA.m_physicsHeight, actorA.m_physicsRadius, actorB.m_position, actorB.m_physicsHeight, actorB.m_physicsRadius))
				//			{
				//				if (!actorA.m_isProjectileDead)
				//				{
				//					m_game->m_allSoundPlaybackIDs[GAME_PLAYER_HURT] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_PLAYER_HURT], actorB.m_position);
				//				}
				//
				//				actorA.m_isProjectileDead = true;
				//
				//				Vec2 pos = Vec2(actorB.m_position.x, actorB.m_position.y);
				//
				//				PushDiscOutOfDisc2D(pos, actorB.m_physicsRadius, Vec2(actorA.m_position.x, actorA.m_position.y), actorA.m_physicsRadius);
				//
				//				actorB.m_position.x = pos.x;
				//				actorB.m_position.y = pos.y;
				//
				//				actorB.AddImpulse(actorA.m_definition.m_impulseOnCollide * actorA.m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
				//
				//				if (actorB.m_aiController)
				//				{
				//					RandomNumberGenerator random = RandomNumberGenerator();
				//					actorB.m_aiController->DamagedBy(actorA.m_UID.GetActor()->m_projectileOwner);
				//					actorB.Damage(random.RollRandomFloatInRange(actorA.m_definition.m_damageOnCollide.m_min, actorA.m_definition.m_damageOnCollide.m_max));
				//				}
				//				else if (actorB.m_controller)
				//				{
				//					RandomNumberGenerator random = RandomNumberGenerator();
				//					actorB.Damage(random.RollRandomFloatInRange(actorA.m_definition.m_damageOnCollide.m_min, actorA.m_definition.m_damageOnCollide.m_max));
				//				}
				//			}
				//		}
				//		else
				//		{
				//			if (DoZCylindersOverlap(actorA.m_position, actorA.m_physicsHeight, actorA.m_physicsRadius, actorB.m_position, actorB.m_physicsHeight, actorB.m_physicsRadius))
				//			{
				//				if (!actorA.m_isProjectileDead)
				//				{
				//					m_game->m_allSoundPlaybackIDs[GAME_DEMON_HURT] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_DEMON_HURT], actorB.m_position);
				//				}
				//
				//				actorA.m_isProjectileDead = true;
				//
				//				Vec2 pos = Vec2(actorB.m_position.x, actorB.m_position.y);
				//
				//				PushDiscOutOfDisc2D(pos, actorB.m_physicsRadius, Vec2(actorA.m_position.x, actorA.m_position.y), actorA.m_physicsRadius);
				//
				//				actorB.m_position.x = pos.x;
				//				actorB.m_position.y = pos.y;
				//
				//				actorB.AddImpulse(actorA.m_definition.m_impulseOnCollide * actorA.m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
				//
				//				if (actorB.m_aiController)
				//				{
				//					RandomNumberGenerator random = RandomNumberGenerator();
				//					actorB.m_aiController->DamagedBy(actorA.m_UID.GetActor()->m_projectileOwner);
				//					actorB.Damage(random.RollRandomFloatInRange(actorA.m_definition.m_damageOnCollide.m_min, actorA.m_definition.m_damageOnCollide.m_max));
				//				}
				//				else if (actorB.m_controller)
				//				{
				//					RandomNumberGenerator random = RandomNumberGenerator();
				//					actorB.Damage(random.RollRandomFloatInRange(actorA.m_definition.m_damageOnCollide.m_min, actorA.m_definition.m_damageOnCollide.m_max));
				//				}
				//			}
				//		}
				//	}
				//}
				//else if (!actorB.m_isDead && !actorB.m_isActorCorpse && actorB.m_definition.m_name == "PlasmaProjectile")
				//{
				//	if (&actorA != actorB.m_projectileOwner && actorA.m_definition.m_name != actorB.m_definition.m_name)
				//	{
				//		if (actorA.m_definition.m_name == "Marine")
				//		{
				//			if (DoZCylindersOverlap(actorA.m_position, actorA.m_physicsHeight, actorA.m_physicsRadius, actorB.m_position, actorB.m_physicsHeight, actorB.m_physicsRadius))
				//			{
				//				if (!actorB.m_isProjectileDead)
				//				{
				//					m_game->m_allSoundPlaybackIDs[GAME_PLAYER_HURT] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_PLAYER_HURT], actorA.m_position);
				//				}
				//
				//				actorB.m_isProjectileDead = true;
				//
				//				Vec2 pos = Vec2(actorA.m_position.x, actorA.m_position.y);
				//
				//				PushDiscOutOfDisc2D(pos, actorA.m_physicsRadius, Vec2(actorB.m_position.x, actorB.m_position.y), actorB.m_physicsRadius);
				//
				//				actorA.m_position.x = pos.x;
				//				actorA.m_position.y = pos.y;
				//
				//				actorA.AddImpulse(actorB.m_definition.m_impulseOnCollide * actorB.m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
				//
				//				if (actorA.m_aiController)
				//				{
				//					RandomNumberGenerator random = RandomNumberGenerator();
				//					actorA.m_aiController->DamagedBy(actorB.m_UID.GetActor()->m_projectileOwner);
				//					actorA.Damage(random.RollRandomFloatInRange(actorB.m_definition.m_damageOnCollide.m_min, actorB.m_definition.m_damageOnCollide.m_max));
				//				}
				//				else if (actorA.m_controller)
				//				{
				//					RandomNumberGenerator random = RandomNumberGenerator();
				//					actorA.Damage(random.RollRandomFloatInRange(actorB.m_definition.m_damageOnCollide.m_min, actorB.m_definition.m_damageOnCollide.m_max));
				//				}
				//			}
				//		}
				//		else
				//		{
				//			if (DoZCylindersOverlap(actorA.m_position, actorA.m_physicsHeight, actorA.m_physicsRadius, actorB.m_position, actorB.m_physicsHeight, actorB.m_physicsRadius))
				//			{
				//				if (!actorB.m_isProjectileDead)
				//				{
				//					m_game->m_allSoundPlaybackIDs[GAME_DEMON_HURT] = g_theAudio->StartSoundAt(m_game->m_allSoundIDs[GAME_DEMON_HURT], actorA.m_position);
				//				}
				//
				//				actorB.m_isProjectileDead = true;
				//
				//				Vec2 pos = Vec2(actorA.m_position.x, actorA.m_position.y);
				//
				//				PushDiscOutOfDisc2D(pos, actorA.m_physicsRadius, Vec2(actorB.m_position.x, actorB.m_position.y), actorB.m_physicsRadius);
				//
				//				actorA.m_position.x = pos.x;
				//				actorA.m_position.y = pos.y;
				//
				//				actorA.AddImpulse(actorB.m_definition.m_impulseOnCollide * actorB.m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
				//
				//				if (actorA.m_aiController)
				//				{
				//					RandomNumberGenerator random = RandomNumberGenerator();
				//					actorA.m_aiController->DamagedBy(actorB.m_UID.GetActor()->m_projectileOwner);
				//					actorA.Damage(random.RollRandomFloatInRange(actorB.m_definition.m_damageOnCollide.m_min, actorB.m_definition.m_damageOnCollide.m_max));
				//				}
				//				else if (actorA.m_controller)
				//				{
				//					RandomNumberGenerator random = RandomNumberGenerator();
				//					actorA.Damage(random.RollRandomFloatInRange(actorB.m_definition.m_damageOnCollide.m_min, actorB.m_definition.m_damageOnCollide.m_max));
				//				}
				//			}
				//		}
				//	}
				//}
				//else
				{
					if (actorB.m_definition.m_name != "PlasmaProjectile" && actorA.m_definition.m_name != "PlasmaProjectile")
					{
						if (DoZCylindersOverlap(actorA.m_position, actorA.m_physicsHeight, actorA.m_physicsRadius, actorB.m_position, actorB.m_physicsHeight, actorB.m_physicsRadius))
						{
							bool aPushb = !actorA.m_definition.m_simulated && actorB.m_definition.m_simulated;
							bool bPusha = actorA.m_definition.m_simulated && !actorB.m_definition.m_simulated;
							bool bothPush = actorA.m_definition.m_simulated && actorB.m_definition.m_simulated;

							if (aPushb)
							{
								Vec2 pos = Vec2(actorB.m_position.x, actorB.m_position.y);

								PushDiscOutOfDisc2D(pos, actorB.m_physicsRadius, Vec2(actorA.m_position.x, actorA.m_position.y), actorA.m_physicsRadius);

								actorB.m_position.x = pos.x;
								actorB.m_position.y = pos.y;
							}
							else if (bPusha)
							{
								Vec2 pos = Vec2(actorA.m_position.x, actorA.m_position.y);

								PushDiscOutOfDisc2D(pos, actorA.m_physicsRadius, Vec2(actorB.m_position.x, actorB.m_position.y), actorB.m_physicsRadius);

								actorA.m_position.x = pos.x;
								actorA.m_position.y = pos.y;
							}
							else if (bothPush)
							{
								Vec2 posA = Vec2(actorA.m_position.x, actorA.m_position.y);
								Vec2 posB = Vec2(actorB.m_position.x, actorB.m_position.y);

								PushDiscsOutOfEachOther2D(posA, actorA.m_physicsRadius, posB, actorB.m_physicsRadius);

								actorA.m_position.x = posA.x;
								actorA.m_position.y = posA.y;
								actorB.m_position.x = posB.x;
								actorB.m_position.y = posB.y;
							}
						}
					}
				}
			}
		}
	}
}

void Map::CollideActorsWithMap()
{
	for (size_t index = 0; index < m_actorList.size(); index++)
	{
		if (m_actorList[index] != nullptr)
		{
			CollideActorWithMap(*m_actorList[index]);

			if (m_actorList[index]->m_aiController)
			{
				if (m_actorList[index]->m_position.z > 1.0f - m_actorList[index]->m_physicsHeight)
				{
					m_actorList[index]->m_position.z = 1.0f - m_actorList[index]->m_physicsHeight;
				}

				if (m_actorList[index]->m_position.z < 0.0f)
				{
					m_actorList[index]->m_position.z = 0.0f;
				}

				int x = RoundDownToInt(m_actorList[index]->m_position.x);
				int y = RoundDownToInt(m_actorList[index]->m_position.y);

				int tileIndex1 = x + 1 + (y * m_dimensions.x);
				int tileIndex2 = x - 1 + (y * m_dimensions.x);
				int tileIndex3 = x + ((y + 1) * m_dimensions.x);
				int tileIndex4 = x + ((y - 1) * m_dimensions.x);

				Vec2 pos = Vec2(m_actorList[index]->m_position.x, m_actorList[index]->m_position.y);

				if (m_tiles[tileIndex1].GetDefinition()->m_name == "RockFloor")
				{
					float xCoord = float(x + 1);

					if (xCoord - pos.x <= m_actorList[index]->m_physicsRadius)
					{
						PushDiscOutOfAABB2D(pos, m_actorList[index]->m_physicsRadius, AABB2(float(x + 1), float(y), float(x + 2), float(y + 1)));
						m_actorList[index]->m_position.x = pos.x;
						m_actorList[index]->m_position.y = pos.y;
					}
				}

				if (m_tiles[tileIndex2].GetDefinition()->m_name == "RockFloor")
				{
					float xCoord = float(x);

					if (pos.x - xCoord <= m_actorList[index]->m_physicsRadius)
					{
						PushDiscOutOfAABB2D(pos, m_actorList[index]->m_physicsRadius, AABB2(float(x - 1), float(y), float(x), float(y + 1)));
						m_actorList[index]->m_position.x = pos.x;
						m_actorList[index]->m_position.y = pos.y;
					}
				}

				if (m_tiles[tileIndex3].GetDefinition()->m_name == "RockFloor")
				{
					float yCoord = float(y + 1);

					if (yCoord - pos.y <= m_actorList[index]->m_physicsRadius)
					{
						PushDiscOutOfAABB2D(pos, m_actorList[index]->m_physicsRadius, AABB2(float(x), float(y + 1), float(x + 1), float(y + 2)));
						m_actorList[index]->m_position.x = pos.x;
						m_actorList[index]->m_position.y = pos.y;
					}
				}

				if (m_tiles[tileIndex4].GetDefinition()->m_name == "RockFloor")
				{
					float yCoord = float(y);

					if (pos.y - yCoord <= m_actorList[index]->m_physicsRadius)
					{
						PushDiscOutOfAABB2D(pos, m_actorList[index]->m_physicsRadius, AABB2(float(x), float(y - 1), float(x + 1), float(y)));
						m_actorList[index]->m_position.x = pos.x;
						m_actorList[index]->m_position.y = pos.y;
					}
				}
			}
		}
	}

	for (int i = 0; i < m_game->m_numOfPlayers; i++)
	{
		if (m_game->m_playerController[i] != nullptr && m_game->m_playerController[i]->m_actorUID != ActorUID::INVALID && m_game->m_playerController[i]->m_actorUID.GetActor() != nullptr)
		{
			CollideActorWithMap(*m_game->m_playerController[i]->m_actorUID.GetActor());
		}
	}
}

void Map::CollideActorWithMap(Actor& actor)
{
	if (actor.m_definition.m_collidesWithWorld)
	{
		if (actor.m_definition.m_name == "PlasmaProjectile")
		{
			if (actor.m_position.z > 1.0f - actor.m_physicsHeight)
			{
				actor.m_position.z = 1.0f - actor.m_physicsHeight;

				actor.m_isProjectileDead = true;
			}

			if (actor.m_position.z < 0.0f)
			{
				actor.m_position.z = 0.0f;

				actor.m_isProjectileDead = true;
			}

			int x = RoundDownToInt(actor.m_position.x);
			int y = RoundDownToInt(actor.m_position.y);

			Vec2 pos = Vec2(actor.m_position.x, actor.m_position.y);

			if (AreCoordsInBounds(x + 1, y))
			{
				float xCoord = float(x + 1);

				if (xCoord - pos.x <= actor.m_physicsRadius)
				{
					PushDiscOutOfAABB2D(pos, actor.m_physicsRadius, AABB2(float(x + 1), float(y), float(x + 2), float(y + 1)));
					actor.m_position.x = pos.x;
					actor.m_position.y = pos.y;

					actor.m_isProjectileDead = true;
				}
			}

			if (AreCoordsInBounds(x - 1, y))
			{
				float xCoord = float(x);

				if (pos.x - xCoord <= actor.m_physicsRadius)
				{
					PushDiscOutOfAABB2D(pos, actor.m_physicsRadius, AABB2(float(x - 1), float(y), float(x), float(y + 1)));
					actor.m_position.x = pos.x;
					actor.m_position.y = pos.y;

					actor.m_isProjectileDead = true;
				}
			}

			if (AreCoordsInBounds(x, y + 1))
			{
				float yCoord = float(y + 1);

				if (yCoord - pos.y <= actor.m_physicsRadius)
				{
					PushDiscOutOfAABB2D(pos, actor.m_physicsRadius, AABB2(float(x), float(y + 1), float(x + 1), float(y + 2)));
					actor.m_position.x = pos.x;
					actor.m_position.y = pos.y;

					actor.m_isProjectileDead = true;
				}
			}

			if (AreCoordsInBounds(x, y - 1))
			{
				float yCoord = float(y);

				if (pos.y - yCoord <= actor.m_physicsRadius)
				{
					PushDiscOutOfAABB2D(pos, actor.m_physicsRadius, AABB2(float(x), float(y - 1), float(x + 1), float(y)));
					actor.m_position.x = pos.x;
					actor.m_position.y = pos.y;

					actor.m_isProjectileDead = true;
				}
			}
		}
		else
		{
			if (actor.m_position.z > 1.0f - actor.m_physicsHeight)
			{
				actor.m_position.z = 1.0f - actor.m_physicsHeight;
			}

			if (actor.m_position.z < 0.0f)
			{
				actor.m_position.z = 0.0f;
			}

			int x = RoundDownToInt(actor.m_position.x);
			int y = RoundDownToInt(actor.m_position.y);

			Vec2 pos = Vec2(actor.m_position.x, actor.m_position.y);

			if (AreCoordsInBounds(x + 1, y))
			{
				float xCoord = float(x + 1);

				if (xCoord - pos.x <= actor.m_physicsRadius)
				{
					PushDiscOutOfAABB2D(pos, actor.m_physicsRadius, AABB2(float(x + 1), float(y), float(x + 2), float(y + 1)));
					actor.m_position.x = pos.x;
					actor.m_position.y = pos.y;
				}
			}

			if (AreCoordsInBounds(x - 1, y))
			{
				float xCoord = float(x);

				if (pos.x - xCoord <= actor.m_physicsRadius)
				{
					PushDiscOutOfAABB2D(pos, actor.m_physicsRadius, AABB2(float(x - 1), float(y), float(x), float(y + 1)));
					actor.m_position.x = pos.x;
					actor.m_position.y = pos.y;
				}
			}

			if (AreCoordsInBounds(x, y + 1))
			{
				float yCoord = float(y + 1);

				if (yCoord - pos.y <= actor.m_physicsRadius)
				{
					PushDiscOutOfAABB2D(pos, actor.m_physicsRadius, AABB2(float(x), float(y + 1), float(x + 1), float(y + 2)));
					actor.m_position.x = pos.x;
					actor.m_position.y = pos.y;
				}
			}

			if (AreCoordsInBounds(x, y - 1))
			{
				float yCoord = float(y);

				if (pos.y - yCoord <= actor.m_physicsRadius)
				{
					PushDiscOutOfAABB2D(pos, actor.m_physicsRadius, AABB2(float(x), float(y - 1), float(x + 1), float(y)));
					actor.m_position.x = pos.x;
					actor.m_position.y = pos.y;
				}
			}
		}
	}
}

void Map::DebugPossessNext()
{
	int currentIndex = m_game->m_playerController[0]->m_actorUID.GetIndex();

	if (currentIndex == (int)m_actorList.size() - 1)
	{
		currentIndex = -1;
	}

	size_t nextIndex = 0;

	for(size_t index = (size_t)currentIndex + 1; index < m_actorList.size(); index++)
	{
		if (m_actorList[index])
		{
			if (m_actorList[index]->m_definition.m_canBePossessed)
			{
				nextIndex = index;
				break;
			}
		}
	}

	if (m_actorList[nextIndex])
	{
		m_game->m_playerController[0]->Possess(m_actorList[nextIndex]);
	}
}

void Map::DeleteDestroyedActors()
{
	for (size_t index = 0; index < m_actorList.size(); index++)
	{
		if (m_actorList[index] != nullptr && m_actorList[index]->m_isDead)
		{
			DELETE_PTR(m_actorList[index]);
			m_currentNumOfAI--;
		}
	}
}

RaycastResultDoomenstein Map::RaycastAll(Vec3 const& start, Vec3 const& direction, float distance) const
{
	RaycastResultDoomenstein mapRaycastZ = RaycastWorldZ(start, direction, distance);
	RaycastResultDoomenstein mapRaycastXY = RaycastWorldXY(start, direction, distance);
	RaycastResultDoomenstein mapRaycastActor = RaycastWorldActors(start, direction, distance);

	float rayZDist = GetDistance3D(mapRaycastZ.m_raycast.m_impactPos, start);
	float rayXYDist = GetDistance3D(mapRaycastXY.m_raycast.m_impactPos, start);
	float rayActorDist = GetDistance3D(mapRaycastActor.m_raycast.m_impactPos, start);

	std::vector<float> rayImpactLengths;

	rayImpactLengths.push_back(rayZDist);
	rayImpactLengths.push_back(rayXYDist);
	rayImpactLengths.push_back(rayActorDist);

	std::sort(rayImpactLengths.begin(), rayImpactLengths.end());

	if (rayImpactLengths[0] == rayZDist)
	{
		return mapRaycastZ;
	}
	else if (rayImpactLengths[0] == rayXYDist)
	{
		return mapRaycastXY;
	}
	else if (rayImpactLengths[0] == rayActorDist)
	{
		return mapRaycastActor;
	}

	return RaycastResultDoomenstein();
}

RaycastResultDoomenstein Map::RaycastWorldXY(Vec3 const& start, Vec3 const& direction, float distance) const
{
	RaycastResultDoomenstein ray;

	ray.m_raycast.m_rayStartPos = start;
	ray.m_raycast.m_rayFwdNormal = direction;
	ray.m_raycast.m_rayMaxLength = distance;
	ray.m_raycast.m_impactPos = start + (distance * direction);

	if(start.x < 0.0f || start.y < 0.0f)
		return ray;

	if (IsPositionInBounds(start))
	{
		ray.m_raycast.m_didImpact = true;
		ray.m_raycast.m_impactDist = 0.0f;
		ray.m_raycast.m_impactPos = start;
		ray.m_raycast.m_impactNormal = -1.0f * direction * distance;

		return ray;
	}

	//if ()
	{
		int tileX = RoundDownToInt(start.x);
		int tileY = RoundDownToInt(start.y);

		float fwdDistPerXCrossing = 1.0f / fabsf(direction.x);
		float fwdDistPerYCrossing = 1.0f / fabsf(direction.y);

		int tileStepDirectionX = 0;
		int tileStepDirectionY = 0;

		if (direction.x < 0)
			tileStepDirectionX = -1;
		else
			tileStepDirectionX = 1;

		if (direction.y < 0)
			tileStepDirectionY = -1;
		else
			tileStepDirectionY = 1;

		float xAtFirstXCrossing = float(tileX + ((tileStepDirectionX + 1) / 2));
		float yAtFirstYCrossing = float(tileY + ((tileStepDirectionY + 1) / 2));

		float xDistToFirstXCrossing = xAtFirstXCrossing - start.x;
		float yDistToFirstYCrossing = yAtFirstYCrossing - start.y;

		float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing;
		float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing;

		while (true)
		{
			if (fwdDistAtNextXCrossing < fwdDistAtNextYCrossing)
			{
				if (fwdDistAtNextXCrossing > distance)
					return ray;

				tileX += tileStepDirectionX;

				if (AreCoordsInBounds(tileX, tileY))
				{
					ray.m_raycast.m_didImpact = true;
					ray.m_raycast.m_impactDist = fwdDistAtNextXCrossing;
					ray.m_raycast.m_impactPos = start + (direction * fwdDistAtNextXCrossing);
					ray.m_raycast.m_impactNormal = -(float)tileStepDirectionX * Vec3(1.0f, 0.0f, 0.0f);

					if (ray.m_raycast.m_impactPos.z > 1.0f || ray.m_raycast.m_impactPos.z < 0.0f)
					{
						fwdDistAtNextXCrossing += fwdDistPerXCrossing;
						ray.m_raycast.m_didImpact = false;
						ray.m_raycast.m_rayStartPos = start;
						ray.m_raycast.m_rayFwdNormal = direction;
						ray.m_raycast.m_rayMaxLength = distance;
						ray.m_raycast.m_impactPos = start + (distance * direction);
						continue;
					}

					return ray;
				}
				else
				{
					fwdDistAtNextXCrossing += fwdDistPerXCrossing;
				}
			}
			else if (fwdDistAtNextYCrossing < fwdDistAtNextXCrossing)
			{
				if (fwdDistAtNextYCrossing > distance)
					return ray;

				tileY += tileStepDirectionY;

				if (AreCoordsInBounds(tileX, tileY))
				{
					ray.m_raycast.m_didImpact = true;
					ray.m_raycast.m_impactDist = fwdDistAtNextYCrossing;
					ray.m_raycast.m_impactPos = start + (direction * fwdDistAtNextYCrossing);
					ray.m_raycast.m_impactNormal = -(float)tileStepDirectionY * Vec3(0.0f, 1.0f, 0.0f);

					if (ray.m_raycast.m_impactPos.z > 1.0f || ray.m_raycast.m_impactPos.z < 0.0f)
					{
						fwdDistAtNextYCrossing += fwdDistPerYCrossing;
						ray.m_raycast.m_didImpact = false;
						ray.m_raycast.m_rayStartPos = start;
						ray.m_raycast.m_rayFwdNormal = direction;
						ray.m_raycast.m_rayMaxLength = distance;
						ray.m_raycast.m_impactPos = start + (distance * direction);
						continue;
					}

					return ray;
				}
				else
				{
					fwdDistAtNextYCrossing += fwdDistPerYCrossing;
				}
			}
		}
	}

	return RaycastResultDoomenstein();
}

RaycastResultDoomenstein Map::RaycastWorldZ(Vec3 const& start, Vec3 const& direction, float distance) const
{
	RaycastResultDoomenstein raycast;

	raycast.m_raycast.m_rayStartPos = start;
	raycast.m_raycast.m_rayFwdNormal = direction;
	raycast.m_raycast.m_rayMaxLength = distance;
	raycast.m_raycast.m_impactPos = start + (distance * direction);

	Vec3 ray = direction * distance;

	if (ray.z > 0.0f)
	{
		float t = (1.0f - start.z) / (direction * distance).z;

		if (t > 0.0f && t < 1.0f)
		{
			Vec3 impactPoint = start + (t * direction * distance);

			if(impactPoint.x < 0.0f || impactPoint.y < 0.0f || impactPoint.x > m_dimensions.x || impactPoint.y > m_dimensions.y)
				return RaycastResultDoomenstein();

			raycast.m_raycast.m_didImpact = true;
			raycast.m_raycast.m_impactDist = (t * distance);
			raycast.m_raycast.m_impactPos = impactPoint;
			raycast.m_raycast.m_impactNormal = Vec3(0.0f, 0.0f, -1.0f);
		}
	}
	else if (ray.z <= 1.0f)
	{
		float t = -start.z / (direction * distance).z;

		if (t > 0.0f && t < 1.0f)
		{
			Vec3 impactPoint = start + (t * direction * distance);

			if (impactPoint.x < 0.0f || impactPoint.y < 0.0f || impactPoint.x > m_dimensions.x || impactPoint.y > m_dimensions.y)
				return RaycastResultDoomenstein();

			raycast.m_raycast.m_didImpact = true;
			raycast.m_raycast.m_impactDist = (t * distance);
			raycast.m_raycast.m_impactPos = impactPoint;
			raycast.m_raycast.m_impactNormal = Vec3(0.0f, 0.0f, 1.0f);
		}
	}

	return raycast;
}

RaycastResultDoomenstein Map::RaycastWorldActors(Vec3 const& start, Vec3 const& direction, float distance) const
{
	std::vector<float> rayImpactLengths;

	for (size_t index = 0; index < m_actorList.size(); index++)
	{
		if (m_actorList[index] != nullptr)
		{
			for (int i = 0; i < m_game->m_numOfPlayers; i++)
			{
				if (m_game->m_playerController[i] != nullptr && m_game->m_playerController[i]->m_actorUID != ActorUID::INVALID && m_game->m_playerController[i]->GetActor() != nullptr)
				{
					if (m_actorList[index] != m_game->m_playerController[i]->GetActor())
					{
						RaycastResultDoomenstein mapActorRaycast;
						mapActorRaycast.m_raycast = RaycastVsZCylinder3D(start, direction, distance, m_actorList[index]->m_position, m_actorList[index]->m_physicsHeight, m_actorList[index]->m_physicsRadius);

						float rayLength = GetDistance3D(mapActorRaycast.m_raycast.m_impactPos, start);

						rayImpactLengths.push_back(rayLength);
					}
				}
			}
		}
	}

	if (rayImpactLengths.size() > 0)
	{
		std::sort(rayImpactLengths.begin(), rayImpactLengths.end());

		if (rayImpactLengths[0] == 0.0f)
		{
			rayImpactLengths[0] = rayImpactLengths[1];
		}

		for (size_t index = 0; index < m_actorList.size(); index++)
		{
			if (m_actorList[index] != nullptr)
			{
				for (int i = 0; i < m_game->m_numOfPlayers; i++)
				{
					if (m_game->m_playerController[i] != nullptr && m_game->m_playerController[i]->m_actorUID != ActorUID::INVALID && m_game->m_playerController[i]->GetActor() != nullptr)
					{
						if (m_actorList[index] != m_game->m_playerController[i]->GetActor())
						{
							RaycastResultDoomenstein mapActorRaycast;
							mapActorRaycast.m_raycast = RaycastVsZCylinder3D(start, direction, distance, m_actorList[index]->m_position, m_actorList[index]->m_physicsHeight, m_actorList[index]->m_physicsRadius);
							mapActorRaycast.m_impactedActor = m_actorList[index];

							if (rayImpactLengths[0] == mapActorRaycast.m_raycast.m_impactDist)
								return mapActorRaycast;
						}
					}
				}
			}
		}
	}

	return RaycastResultDoomenstein();
}