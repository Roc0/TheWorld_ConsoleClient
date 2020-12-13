#pragma once

#include "TheWorld_ClientDll.h"

class KBAvatar
{
public:
	KBAvatar(KBEngine::DBID avatarDBID, const std::string& avatarName);
	~KBAvatar();

	KBEngine::DBID getAvatarID(void) { return m_avatarDBID; }
	const std::string& getAvatarName(void) { return m_avatarName; }
	
	void dumpStatus(int idx, bool minidump);

protected:
	KBEngine::DBID m_avatarDBID;
	std::string m_avatarName;
};

