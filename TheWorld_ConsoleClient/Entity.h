#pragma once

#include "TheWorld_ClientDll.h"

class KBEntity
{
public:
	KBEntity(KBEngine::ENTITY_ID eid);
	virtual ~KBEntity();

	KBEngine::ENTITY_ID id()const { return mID; }

protected:
	KBEngine::ENTITY_ID mID;				// entityID
};
