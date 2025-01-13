#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB3.hpp"

#include <string>

struct TileDefinition
{
	std::string					m_name						= "";
	bool						m_isSolid					= false;
	Rgba8						m_mapImagePixelColor		= Rgba8::BLACK;
	IntVec2						m_floorSpriteCoords			= IntVec2::ZERO;
	IntVec2						m_ceilSpriteCoords			= IntVec2::ZERO;
	IntVec2						m_wallSpriteCoords			= IntVec2::ZERO;

	static TileDefinition		s_definitions[13];

	static void					InitializeDef();
};

class Tile
{
public:
	TileDefinition*				m_definition				= nullptr;
	IntVec2						m_coordinates				= IntVec2::ZERO;
public:
								Tile();
								Tile(IntVec2 tileCoords, TileDefinition* definition);
								~Tile();

	TileDefinition*				GetDefinition() const;
	AABB3						GetBounds() const;
};