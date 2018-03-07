#include "stdafx.h"
#include "OtherEntity.h"

OtherEntity::OtherEntity(KBEngine::ENTITY_ID eid, SpaceWorld *pSpaceWorld) : KBEntity(eid, pSpaceWorld)
{
	mInWorld = false;
}


OtherEntity::~OtherEntity()
{
}
