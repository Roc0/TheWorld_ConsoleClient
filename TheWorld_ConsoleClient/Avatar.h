#pragma once

#include "TheWorld_ClientDll.h"

class KBAvatar
{
public:
	KBAvatar(KBEngine::DBID avatarDBID, const std::string& avatarName);
	~KBAvatar();

	KBEngine::DBID getAvatarID(void) { return mAvataDBID; }
	const std::string& getAvatarName(void) { return mAvatarName; }
	
	void dumpStatus(int idx, bool minidump);

protected:
	KBEngine::DBID mAvataDBID;
	std::string mAvatarName;
};

