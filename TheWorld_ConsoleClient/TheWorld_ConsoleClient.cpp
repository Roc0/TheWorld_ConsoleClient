// TheWorld_ConsoleClient.cpp : Defines the entry point for the console application.
//

/*

	NOTE:

	* Il client può invocare funzioni python (client) usando kbe_callEntityMethod
	* Il client può mandare venti alla parte scrit usando kbe_fireEvent. L'evento viene raccolto nel metodo kbengine_onEvent di kbemain.py (client). Per esempio eventi "reset" / "relive"
	
	TODO:

	* impostare targetEntity (m_pClientApp->setTargetEntity()): vedi Ogre Demo (SpaceWorld::mousePressed, pickEntity etc.)
	* impostare posizione e direzione del player: vedi Ogre Demo (EntityComplex.cpp line 201)
*/

#include "stdafx.h"

//#include <windows.h>

#include "TheWorld_ConsoleClient.h"
#include "TheWorld_ConsoleClient_GL.h"
#include "PlayerEntity.h"
#include "OtherEntity.h"

#include <conio.h>
#include <conio.h>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>

/*
 * Case Insensitive String Comparision
 */
bool caseInSensStringEqual(std::string& str1, std::string& str2)
{
	return boost::iequals(str1, str2);
}


TheWorld_ClientAppConsole::TheWorld_ClientAppConsole()
{
	//setMain(this);

	m_pGLClientApp = new TheWorld_ClientApp_GL(this);

	//m_bServerClosed = false;
	m_iSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
	setAppMode(TheWorld_ClientApp::InitialMenu, true);
}

TheWorld_ClientAppConsole::~TheWorld_ClientAppConsole()
{
	delete m_pGLClientApp;
}

void TheWorld_ClientAppConsole::go(void)
{
	if (!init())
		return;

	doMainLoop();

	cleanup();
}

bool TheWorld_ClientAppConsole::init(void)
{
	if (!m_pGLClientApp->init())
		return false;

	return true;
}

void TheWorld_ClientAppConsole::cleanup(void)
{
	m_pGLClientApp->cleanup();
}

void TheWorld_ClientAppConsole::doMainLoop(void)
{
	while (!getShutdownRequired())
	{
		kbengine_MessagePump(this);

		if (getAppMode() == TheWorld_ClientApp::InitialMenu)
			manageInitialMenu();
		else
			manageGraphicRendering();

		if (getDoSleepInMainLoop())
			kbengine_Sleep(1);
	}
}
	
void TheWorld_ClientAppConsole::manageInitialMenu(void)
{
	if (getInitAppModeRequired())
	{
		setInitAppModeRequired(false);
		printf("\n");
		printf("Actions: \n");
		printf("\n");
		printf("   l - Login\n");
		printf("   x - Logout\n");
		printf("\n");
		printf("   c - Create Avatar\n");
		printf("   r - Remove Avatar\n");
		printf("\n");
		printf("   d - Dump Status\n");
		printf("   m - Minidump\n");
		printf("\n");
		printf("   z - Display Menu\n");
		printf("\n");
		AVATARS& avatars = getAvatars();
		AVATARS::iterator iter = avatars.begin();
		int idx = 0;
		for (; iter != avatars.end(); iter++)
		{
			KBAvatar* pAvatar = iter->second.get();
			printf("   %d - Select Avatar %d - %s\n", idx + 1, (int)pAvatar->getAvatarID(), pAvatar->getAvatarName());
			idx++;
		}
		printf("\n");
		printf("   . - Quit\n");
	}

	if (kbhit())
	{
		int ch = getch();
		if (m_iSelectAvatarPending == SELECT_AVATAR_PENDING)
		{
			if (ch == '.' || ch == 27)
			{
			}
			else
			{
				int i = ch - 48 - 1;
				if (i >= 0 && i < getAvatars().size())
				{
					doInitialMenuAction(ch);
				}
			}
			m_iSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
			setInitAppModeRequired(true);
		}
		else
		{
			if (ch == '.' || ch == 27) {
				setShutdownRequired(true);
				setDoSleepInMainLoop(false);
				kbengine_Shutdown();
			}
			else
				doInitialMenuAction(ch);
		}
	}
}

void TheWorld_ClientAppConsole::manageGraphicRendering(void)
{
	bool bLogoutRequired = false;
	if (!m_pGLClientApp->handleGraphicRendering(bLogoutRequired, getLoginStatus()))
	{
		kbengine_Logout();
		clearEntities();
		clearAvatars();
		setAppMode(TheWorld_ClientApp::InitialMenu);
		return;
	}

	if (bLogoutRequired)
	{
		kbengine_Logout();
		clearEntities();
		clearAvatars();
		setAppMode(TheWorld_ClientApp::InitialMenu);
	}
}

void TheWorld_ClientAppConsole::doInitialMenuAction(int ch)
{
	char str[256];

	if (m_iSelectAvatarPending == SELECT_AVATAR_PENDING)
	{
		int iAvatar = ch - 48;
		KBAvatar* pAvatar = NULL;
		AVATARS& avatars = getAvatars();
		AVATARS::iterator iter = avatars.begin();
		int idx = 1;
		for (; iter != avatars.end(); iter++)
		{
			if (idx == iAvatar)
			{
				pAvatar = iter->second.get();
				break;
			}
			idx++;
		}
		if (pAvatar)
		{
			std::string avatarName = pAvatar->getAvatarName();
			bool b = kbengine_RemoveAvatar(avatarName);
			sprintf(str, "kbengine_RemoveAvatar - Player ID: %d, Avatar name %s\n", (int)kbengine_PlayerID(), avatarName.c_str());
			//printf(str);	printf("\n");
			kbengine_PrintMessage(str, true);
		}
		m_iSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
		setInitAppModeRequired(true);
	}
		
	switch (ch)
	{
		case 's':
		case 'S':
		{
			setAppMode(TheWorld_ClientApp::WorldMode);
			break;
		}
		case 'x':
		case 'X':
		{
			if (getLoginStatus() == LOGIN_STARTED)
			{
				printf("Login in corso!\n");
			}
			else
			{
				kbengine_Logout();
				clearEntities();
				clearAvatars();
				setInitAppModeRequired(true);
			}
			break;
		}
		case 'l':
		case 'L':
		{
			if (getLoginStatus() == LOGIN_DONE)
			{
				printf("Login già effettuato!\n");
			}
			else if (getLoginStatus() == LOGIN_STARTED)
			{
				printf("Login in corso!\n");
			}
			else
			{
				//std::string datas = "TheWorld_ConsoleClient";
				std::string datas = "TheWorld";
				//if (kbengine_Login(NULL, "123456", datas.data(), KBEngine::uint32(datas.size())));
				if (kbengine_Login("KBEngine", "123456", datas.data(), KBEngine::uint32(datas.size())));
				//if (kbengine_Login("KBEngine1", "123456", datas.data(), KBEngine::uint32(datas.size())));
				//setLoginStatus(LOGIN_STARTED);
				else
					printf("Login error!\n");
			}
			break;
		}
		case 'z':
		case 'Z':
		{
			setInitAppModeRequired(true);
			break;
		}
		case 'c':
		case 'C':
		{
			if (getLoginStatus() == LOGIN_DONE)
			{
				UINT64 uuid = kbengine_GenUUID64();
				std::stringstream ss;
				ss << uuid;
				std::string avatarName = ss.str();
				bool b = kbengine_CreateAvatar(avatarName);
				sprintf(str, "kbengine_CreateAvatar - Player ID: %d\n", (int)kbengine_PlayerID());
				kbengine_PrintMessage(str);
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
		case 'r':
		case 'R':
		{
			if (getLoginStatus() == LOGIN_DONE)
			{
				m_iSelectAvatarPending = SELECT_AVATAR_PENDING;
				printf("\n");
				printf("Select Avatar\n");
				printf("\n");
				AVATARS& avatars = getAvatars();
				AVATARS::iterator iter = avatars.begin();
				int idx = 0;
				for (; iter != avatars.end(); iter++)
				{
					KBAvatar* pAvatar = iter->second.get();
					printf("   %d - Select Avatar %d - %s\n", idx + 1, (int)pAvatar->getAvatarID(), pAvatar->getAvatarName());
					idx++;
				}
				printf("\n");
				printf("   . - Return\n");
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
		case 'd':
		case 'D':
		case 'm':
		case 'M':
		{
			bool minidump = (ch == 'm' ? true : false);
			printf("\n");
			printf("*** %s *********************\n", (ch == 'd' ? "DUMP" : "MINIDUMP"));
			printf("\n");
			{
				ENTITIES& entities = getEntities();
				ENTITIES::iterator iter = entities.begin();
				int idx = 0;
				for (; iter != entities.end(); iter++)
				{
					idx++;
					KBEntity* pEntity = iter->second.get();
					pEntity->dumpStatus(idx, minidump);
				}
			}
			printf("\n");
			getSpaceWorld()->dumpStatus(minidump);
			printf("\n");
			{
				AVATARS& avatars = getAvatars();
				AVATARS::iterator iter = avatars.begin();
				int idx = 0;
				for (; iter != avatars.end(); iter++)
				{
					idx++;
					KBAvatar* pAvatar = iter->second.get();
					pAvatar->dumpStatus(idx, minidump);
				}
			}
			printf("\n");
			printf("PlayerID: %d\n", (int)kbengine_PlayerID());
			printf("\n");
			printf("*** %s *********************\n", (ch == 'd' ? "DUMP" : "MINIDUMP"));
			printf("\n");
			break;
		}
		// '1' / '9'
		default:
		{
			if (getLoginStatus() == LOGIN_DONE)
			{
				int i = ch - 48 - 1;
				if (i >= 0 && i < getAvatars().size())
				{
					KBAvatar* pAvatar = NULL;
					AVATARS& avatars = getAvatars();
					AVATARS::iterator iter = avatars.begin();
					int idx = 0;
					for (; iter != avatars.end(); iter++)
					{
						if (idx == i)
						{
							pAvatar = iter->second.get();
							break;
						}
						idx++;
					}
					if (pAvatar)
					{
						bool b = kbengine_SelectAvatarGame(pAvatar->getAvatarID());
						sprintf(str, "kbengine_SelectAvatarGame - Player ID: %d, Avatar DBID %ld\n", (int)kbengine_PlayerID(), (long)pAvatar->getAvatarID());
						//printf(str);	printf("\n");
						kbengine_PrintMessage(str, true);
					}
				}
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
	}

	return;
}

static bool compareWithPrecision(float num1, float num2, int precision)
{
	float diff = abs(num1 - num2);

	float value = 0;
	if (precision == 7)
		value = 0.0000001;
	else if (precision == 6)
		value = 0.000001;
	else if (precision == 5)
		value = 0.00001;
	else if (precision == 4)
		value = 0.0001;
	else if (precision == 3)
		value = 0.001;
	else if (precision == 2)
		value = 0.01;
	else if (precision == 1)
		value = 0.1;

	if (diff < value)
		return true;
	
	return false;
	
	/*float value = (int)(num * pow(10.0, digit));
	float v = (float)value / (int)pow(10.0, digit);

	int i = 3141592;
	int inum = i / 100;
	float fnum = float(i) / 100;

	//v = std::setprecision(6) << num

	v = floorf(num * 100) / 100;

	//return v;

	char str[40];
	sprintf(str, "%.6f", num);
	sscanf(str, "%f", &num);
	
	return num;*/
}

int main(int argc, char* argv[])
{
	float f1 = 3.14159265358979323846f;
	float f2 = 3.141592;
	bool b = compareWithPrecision(f1, f2, 7);

	TheWorld_ClientAppConsole* app = new TheWorld_ClientAppConsole();

	if (!app->kbengine_Init())
	{
		std::string s = "kbengine_Init is failed!";
		MessageBox(NULL, s.c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return 0;
	}

	// TODO: login
	try
	{
		app->go();
	}
	catch (...)
	{
		MessageBox(NULL, "An exception has occured!", "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}

	app->kbengine_Destroy();

	delete app;

	return 0;
}
