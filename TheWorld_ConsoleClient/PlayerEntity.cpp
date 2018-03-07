#include "stdafx.h"
#include "PlayerEntity.h"


PlayerEntity::PlayerEntity(KBEngine::ENTITY_ID eid, SpaceWorld *pSpaceWorld) : KBEntity(eid, pSpaceWorld)
{
	mInWorld = true;
}


PlayerEntity::~PlayerEntity()
{
}
