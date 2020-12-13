#include "stdafx.h"
#include "OtherEntity.h"

OtherEntity::OtherEntity(KBEngine::ENTITY_ID eid, SpaceWorld *pSpaceWorld) : KBEntity(eid, pSpaceWorld)
{
	m_bIsInWorld = false;
	m_bIsPlayer = false;
}


OtherEntity::~OtherEntity()
{
}
