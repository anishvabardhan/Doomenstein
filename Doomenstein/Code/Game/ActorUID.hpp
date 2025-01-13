#pragma once

class Actor;

struct ActorUID
{
	ActorUID();
	ActorUID(unsigned int salt, unsigned int index);

	bool IsValid() const;
	unsigned int GetIndex() const;
	bool operator==(ActorUID other) const;
	bool operator!= (ActorUID other) const;
	Actor* operator->() const;
	Actor* GetActor() const;

	static const ActorUID INVALID;
public:
	unsigned int m_data;
};