#pragma once

#include "TheWorld_ClientDll.h"

class Space {
public:
	Space(KBEngine::SPACE_ID spaceID, const std::string& resPath);
	void dumpStatus(int idx, bool minidump);

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
	void dumpStatus(bool minidump);

protected:
	SPACES mSpaces;
};

