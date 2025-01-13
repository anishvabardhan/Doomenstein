#include "Game/Tile.hpp"

#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/Vec3.hpp"

TileDefinition TileDefinition::s_definitions[13];

void TileDefinition::InitializeDef()
{
	XmlDocument tileDoc;

	XmlError result = tileDoc.LoadFile("Data/Definitions/TileDefinitions.xml");

	if (result != tinyxml2::XML_SUCCESS)
		GUARANTEE_OR_DIE(false, "COULD NOT LOAD XML");

	XmlElement* element = tileDoc.RootElement()->FirstChildElement();

	for (int index = 0; index < 13; index++)
	{
		s_definitions[index].m_name						= ParseXmlAttribute(*element, "name", s_definitions[index].m_name);
		s_definitions[index].m_isSolid					= ParseXmlAttribute(*element, "isSolid", s_definitions[index].m_isSolid);
		s_definitions[index].m_mapImagePixelColor		= ParseXmlAttribute(*element, "mapImagePixelColor", s_definitions[index].m_mapImagePixelColor);
		s_definitions[index].m_floorSpriteCoords		= ParseXmlAttribute(*element, "floorSpriteCoords", s_definitions[index].m_floorSpriteCoords);
		s_definitions[index].m_ceilSpriteCoords			= ParseXmlAttribute(*element, "ceilingSpriteCoords", s_definitions[index].m_ceilSpriteCoords);
		s_definitions[index].m_wallSpriteCoords			= ParseXmlAttribute(*element, "wallSpriteCoords", s_definitions[index].m_wallSpriteCoords);

		element = element->NextSiblingElement();
	}
}

Tile::Tile()
{
}

Tile::Tile(IntVec2 tileCoords, TileDefinition* definition)
	: m_coordinates(tileCoords), m_definition(definition)
{
}

Tile::~Tile()
{
}

TileDefinition* Tile::GetDefinition() const
{
	return m_definition;
}

AABB3 Tile::GetBounds() const
{
	Vec3 bottomLeftDown = Vec3(static_cast<float>(m_coordinates.x), static_cast<float>(m_coordinates.y), 0.0f);
	Vec3 topRightUp = Vec3(static_cast<float>(m_coordinates.x + 1), static_cast<float>(m_coordinates.y + 1), 1.0f);

	AABB3 bounds = AABB3(bottomLeftDown, topRightUp);

	return bounds;
}
