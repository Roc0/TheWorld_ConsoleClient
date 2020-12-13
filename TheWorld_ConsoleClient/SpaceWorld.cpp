#include "stdafx.h"
#include "SpaceWorld.h"

Space::Space(KBEngine::SPACE_ID spaceID, const std::string& resPath)
{
	m_spaceID = spaceID;
	m_resPath = resPath;
}

void Space::dumpStatus(int idx, bool minidump)
{
	if (minidump)
	{
		printf("SPACE - SpaceID ==> %d, ResPath ==> %s\n", (int)m_spaceID, m_resPath.c_str());
	}
	else
	{
		printf("*** ( Space %d) ******************************************************\n", idx);
		printf("   SpaceID ==> %d\n", (int)m_spaceID);
		printf("   ResPath ==> %s\n", m_resPath.c_str());
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
	SPACES::iterator iter = m_Spaces.find(spaceID);
	if (iter == m_Spaces.end())
	{
		Space *pSpace = new Space(spaceID, resPath);
		m_Spaces[spaceID].reset(pSpace);
	}
	else
	{
		sprintf(str, "Duplicate Space - SpaceID %d", (int)spaceID);
		printf(str);	printf("\n");
	}
}

Space* SpaceWorld::findSpace(KBEngine::SPACE_ID spaceID)
{
	SPACES::iterator iter = m_Spaces.find(spaceID);
	if (iter == m_Spaces.end())
	{
		return NULL;
	}
	else
	{
		return iter->second.get();
	}
}

void SpaceWorld::dumpStatus(bool minidump)
{
	SPACES::iterator iter = m_Spaces.begin();
	int idx = 0;
	for (; iter != m_Spaces.end(); iter++)
	{
		idx++;
		Space* pSpace = iter->second.get();
		pSpace->dumpStatus(idx, minidump);
	}
}
