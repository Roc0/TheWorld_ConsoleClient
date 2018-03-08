#include "stdafx.h"
#include "Avatar.h"


KBAvatar::KBAvatar(KBEngine::DBID avatarDBID, const std::string & avatarName)
{
	mAvataDBID = avatarDBID;
	mAvatarName = avatarName;
}

KBAvatar::~KBAvatar()
{
}

void KBAvatar::dumpStatus(int idx, bool minidump)
{
	if (minidump)
	{
		printf("AVATAR - AvatarID ==> %d, Avatar Name ==> %s\n", (int)mAvataDBID, mAvatarName.c_str());
	}
	else
	{
		printf("*** ( AVATAR %d) ******************************************************\n", idx);
		printf("   AvatarID    ==> %d\n", (int)mAvataDBID);
		printf("   Avatar Name ==> %s\n", mAvatarName.c_str());
		printf("*** ( AVATAR %d) ******************************************************\n\n", idx);
	}
}
