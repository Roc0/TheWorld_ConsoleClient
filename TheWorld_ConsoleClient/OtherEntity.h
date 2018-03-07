#pragma once
#include "Entity.h"
class OtherEntity :
	public KBEntity
{
public:
	OtherEntity(KBEngine::ENTITY_ID eid, SpaceWorld *pSpaceWorld);
	virtual ~OtherEntity();
};

