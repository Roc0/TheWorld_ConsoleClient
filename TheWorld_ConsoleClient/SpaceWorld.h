#pragma once

#include "TheWorld_ClientDll.h"

class Space {
public:
	Space(KBEngine::SPACE_ID spaceID, const std::string& resPath);

protected:
	KBEngine::SPACE_ID mSpaceID;
	std::string mResPath;
};


typedef std::map<KBEngine::SPACE_ID, std::tr1::shared_ptr<Space> > SPACES;


class SpaceWorld
{
public:
	SpaceWorld();
	virtual ~SpaceWorld();
	void addSpace(KBEngine::SPACE_ID spaceID, const std::string& resPath);

protected:
	SPACES mSpaces;
};

