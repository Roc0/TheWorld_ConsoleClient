#include "stdafx.h"
#include "Avatar.h"


KBAvatar::KBAvatar(KBEngine::DBID avatarDBID, const std::string & avatarName)
{
	m_avatarDBID = avatarDBID;
	m_avatarName = avatarName;
}

KBAvatar::~KBAvatar()
{
}

void KBAvatar::dumpStatus(int idx, bool minidump)
{
	if (minidump)
	{
		printf("AVATAR - AvatarID ==> %d, Avatar Name ==> %s\n", (int)m_avatarDBID, m_avatarName.c_str());
	}
	else
	{
		printf("*** ( AVATAR %d) ******************************************************\n", idx);
		printf("   AvatarID    ==> %d\n", (int)m_avatarDBID);
		printf("   Avatar Name ==> %s\n", m_avatarName.c_str());
		printf("*** ( AVATAR %d) ******************************************************\n\n", idx);
	}
}
