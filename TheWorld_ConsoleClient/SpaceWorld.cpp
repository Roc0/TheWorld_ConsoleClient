#include "stdafx.h"
#include "SpaceWorld.h"

Space::Space(KBEngine::SPACE_ID spaceID, const std::string& resPath)
{
	mSpaceID = spaceID;
	mResPath = resPath;
}

void Space::dumpStatus(int idx, bool minidump)
{
	if (minidump)
	{
		printf("SPACE - SpaceID ==> %d, ResPath ==> %s\n", (int)mSpaceID, mResPath.c_str());
	}
	else
	{
		printf("*** ( Space %d) ******************************************************\n", idx);
		printf("   SpaceID ==> %d\n", (int)mSpaceID);
		printf("   ResPath ==> %s\n", mResPath.c_str());
		printf("*** ( Space %d) ******************************************************\n\n", idx);
	}
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
		Space *pSpace = new Space(spaceID, resPath);
		mSpaces[spaceID].reset(pSpace);
	}
	else
	{
		sprintf(str, "Duplicate Space - SpaceID %d", (int)spaceID);
		printf(str);	printf("\n");
	}
}

void SpaceWorld::dumpStatus(bool minidump)
{
	SPACES::iterator iter = mSpaces.begin();
	int idx = 0;
	for (; iter != mSpaces.end(); iter++)
	{
		idx++;
		Space* pSpace = iter->second.get();
		pSpace->dumpStatus(idx, minidump);
	}
}
