#pragma once

#include "TheWorld_ClientDll.h"

class Space {
public:
	Space(KBEngine::SPACE_ID spaceID, const std::string& resPath);
	void dumpStatus(int idx, bool minidump);
	std::string getResPath() { return m_resPath; }

protected:
	KBEngine::SPACE_ID m_spaceID;
	std::string m_resPath;
};


typedef std::map<KBEngine::SPACE_ID, std::tr1::shared_ptr<Space> > SPACES;


class SpaceWorld
{
public:
	SpaceWorld();
	virtual ~SpaceWorld();
	void addSpace(KBEngine::SPACE_ID spaceID, const std::string& resPath);
	Space* findSpace(KBEngine::SPACE_ID spaceID);
	void dumpStatus(bool minidump);

protected:
	SPACES m_Spaces;
};

