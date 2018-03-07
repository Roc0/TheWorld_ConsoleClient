#include "stdafx.h"
#include "SpaceWorld.h"

Space::Space(KBEngine::SPACE_ID spaceID, const std::string& resPath)
{
	mSpaceID = spaceID;
	mResPath = resPath;
}

SpaceWorld::SpaceWorld()
{
}


SpaceWorld::~SpaceWorld()
{
}

void SpaceWorld::addSpace(KBEngine::SPACE_ID spaceID, const std::string& resPath)
{
	char str[256];
	SPACES::iterator iter = mSpaces.find(spaceID);
	if (iter == mSpaces.end())
	{
		sprintf(str, "Duplicate Space - SpaceID %d", (int)spaceID);
		printf(str);	printf("\n");
	}
	else
	{
		Space *pSpace = new Space(spaceID, resPath);
		mSpaces[spaceID].reset(pSpace);
	}
}
