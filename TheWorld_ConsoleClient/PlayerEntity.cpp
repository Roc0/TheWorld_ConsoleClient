#include "stdafx.h"
#include "PlayerEntity.h"


PlayerEntity::PlayerEntity(KBEngine::ENTITY_ID eid, SpaceWorld *pSpaceWorld) : KBEntity(eid, pSpaceWorld)
{
	m_bIsInWorld = false;
	m_bIsPlayer = true;
}


PlayerEntity::~PlayerEntity()
{
}
