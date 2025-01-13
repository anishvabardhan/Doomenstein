#include "Game/ActorUID.hpp"

#include "Game/Map.hpp"
#include "Game/Actor.hpp"

extern Map* g_currentMap;

const ActorUID ActorUID::INVALID = ActorUID(0x0000ffffu, 0x0000ffffu);

ActorUID::ActorUID()
{
	m_data = 0x00000000u;
}

ActorUID::ActorUID(unsigned int salt, unsigned int index)
{
	m_data = (salt << 16) | index & 0xFFFF;
}

bool ActorUID::IsValid() const
{
	return m_data != INVALID.m_data;
}

unsigned int ActorUID::GetIndex() const
{
	return m_data & 0xFFFF;
}

bool ActorUID::operator==(ActorUID other) const
{
	return m_data == other.m_data;
}

bool ActorUID::operator!=(ActorUID other) const
{
	return m_data != other.m_data;
}

Actor* ActorUID::operator->() const
{
	return GetActor();
}

Actor* ActorUID::GetActor() const
{
	return g_currentMap->m_actorList[GetIndex()];
}
