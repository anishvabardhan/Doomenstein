#include "Game/Controller.hpp"

#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"

Controller::Controller()
{
	
}

Controller::~Controller()
{
	//GetActor()->OnUnpossessed(this);
}

void Controller::Update(float deltaseconds)
{
	UNUSED(deltaseconds);
}

void Controller::Possess(Actor* actor)
{
	if (actor)
	{
		if (m_actorUID != ActorUID::INVALID)
		{
			GetActor()->OnUnpossessed(this);
		}

		m_actorUID = actor->m_UID;

		if (m_actorUID != ActorUID::INVALID)
		{
			GetActor()->OnPossessed(this);
		}
	}
}

Actor* Controller::GetActor() const
{
	if(m_actorUID != ActorUID::INVALID)
		return m_actorUID.GetActor();

	return nullptr;
}

bool Controller::IsPlayer()
{
	return false;
}

void Controller::DamagedBy(Actor* actor)
{
	UNUSED(actor);
}

void Controller::KilledBy(Actor* actor)
{
	UNUSED(actor);
}

void Controller::Killed(Actor* actor)
{
	UNUSED(actor);
}
