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
#include <json/json.h>
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
	m_pGLClientApp = new TheWorld_ClientApp_GL(this);

	setShutdownRequired(false);;
	m_bServerClosed = false;
	m_iSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
	m_pSpaceWorld = new SpaceWorld;
	m_pPlayerEntity = m_pTargetEntity = m_pMouseTarget = NULL;
	setAppMode(TheWorld_ClientApp_GL::InitialMenu, true);
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

		if (getAppMode() == TheWorld_ClientApp_GL::InitialMenu)
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
		AVATARS::iterator iter = m_Avatars.begin();
		int idx = 0;
		for (; iter != m_Avatars.end(); iter++)
		{
			KBAvatar* pAvatar = iter->second.get();
			printf("   %d - Select Avatar %d - %s\n", idx + 1, (int)pAvatar->getAvatarID(), pAvatar->getAvatarName().c_str());
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
				if (i >= 0 && i < m_Avatars.size())
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
		m_Entities.clear();
		m_Avatars.clear();
		setAppMode(TheWorld_ClientApp_GL::InitialMenu);
		return;
	}

	if (bLogoutRequired)
	{
		kbengine_Logout();
		m_Entities.clear();
		m_Avatars.clear();
		setAppMode(TheWorld_ClientApp_GL::InitialMenu);
	}
}

void TheWorld_ClientAppConsole::setAppMode(enum TheWorld_ClientApp_GL::_AppMode r, bool bForce)
{
	m_pGLClientApp->setAppMode(r, bForce);
}

enum TheWorld_ClientApp_GL::_AppMode TheWorld_ClientAppConsole::getAppMode(void)
{
	return m_pGLClientApp->getAppMode();
}

bool TheWorld_ClientAppConsole::getInitAppModeRequired(void)
{
	return m_pGLClientApp->getInitAppModeRequired();
}

void TheWorld_ClientAppConsole::setInitAppModeRequired(bool b)
{
	m_pGLClientApp->setInitAppModeRequired(b);
}

void TheWorld_ClientAppConsole::doInitialMenuAction(int ch)
{
	char str[256];

	if (m_iSelectAvatarPending == SELECT_AVATAR_PENDING)
	{
		int iAvatar = ch - 48;
		KBAvatar* pAvatar = NULL;
		AVATARS::iterator iter = m_Avatars.begin();
		int idx = 1;
		for (; iter != m_Avatars.end(); iter++)
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
			setAppMode(TheWorld_ClientApp_GL::GraphicMode);
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
				m_Entities.clear();
				m_Avatars.clear();
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
				std::string datas = "TheWorld_ConsoleClient";
				if (kbengine_Login(NULL, "123456", datas.data(), KBEngine::uint32(datas.size())));
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
				AVATARS::iterator iter = m_Avatars.begin();
				int idx = 0;
				for (; iter != m_Avatars.end(); iter++)
				{
					KBAvatar* pAvatar = iter->second.get();
					printf("   %d - Select Avatar %d - %s\n", idx + 1, (int)pAvatar->getAvatarID(), pAvatar->getAvatarName().c_str());
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
				ENTITIES::iterator iter = m_Entities.begin();
				int idx = 0;
				for (; iter != m_Entities.end(); iter++)
				{
					idx++;
					KBEntity* pEntity = iter->second.get();
					pEntity->dumpStatus(idx, minidump);
				}
			}
			printf("\n");
			m_pSpaceWorld->dumpStatus(minidump);
			printf("\n");
			{
				AVATARS::iterator iter = m_Avatars.begin();
				int idx = 0;
				for (; iter != m_Avatars.end(); iter++)
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
				if (i >= 0 && i < m_Avatars.size())
				{
					KBAvatar* pAvatar = NULL;
					AVATARS::iterator iter = m_Avatars.begin();
					int idx = 0;
					for (; iter != m_Avatars.end(); iter++)
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

void TheWorld_ClientAppConsole::client_onEvent(const KBEngine::EventData* lpEventData)
{
	char str[256];

	switch (lpEventData->id)
	{
	case CLIENT_EVENT_ENTERWORLD:
	{
		const KBEngine::EventData_EnterWorld* pEventData_EnterWorld = static_cast<const KBEngine::EventData_EnterWorld*>(lpEventData);
		KBEngine::ENTITY_ID eid = pEventData_EnterWorld->entityID;

		ENTITIES::iterator iter = m_Entities.find(eid);
		if (iter == m_Entities.end())
		{
			bool bPlayer = false;
			if (kbe_playerID() == eid)
				bPlayer = true;
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERWORLD - %s - EntityID: %d (NOT IN LIST), SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
				(int)pEventData_EnterWorld->entityID, (int)pEventData_EnterWorld->spaceID,
				pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll,
				pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z,
				pEventData_EnterWorld->speed, pEventData_EnterWorld->isOnGround, pEventData_EnterWorld->res.c_str());
			kbengine_PrintMessage(str, true);
			break;
		}

		KBEntity* pEntity = iter->second.get();

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
			m_pPlayerEntity = pEntity;
		}

		pEntity->setPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
		if (bPlayer)
			pEntity->setDesideredPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
		pEntity->setDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
		if (bPlayer)
			pEntity->setDesideredDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
		pEntity->setMoveSpeed(pEventData_EnterWorld->speed);
		pEntity->setSpaceID(pEventData_EnterWorld->spaceID);
		pEntity->setIsOnGround(pEventData_EnterWorld->isOnGround);
		pEntity->setRes(pEventData_EnterWorld->res);

		pEntity->setIsInWorld(true);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERWORLD - %s - EntityID: %d, SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
			(int)pEventData_EnterWorld->entityID, (int)pEventData_EnterWorld->spaceID,
			pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll,
			pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z,
			pEventData_EnterWorld->speed, pEventData_EnterWorld->isOnGround, pEventData_EnterWorld->res.c_str());
		kbengine_PrintMessage(str, true);

		//if (bPlayer)
		//	setAppMode(TheWorld_ClientApp_GL::GraphicMode);
	}
	break;

	case CLIENT_EVENT_LEAVEWORLD:
	{
		KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveWorld*>(lpEventData)->entityID;

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
			m_pPlayerEntity = NULL;
		}

		if (m_pTargetEntity && m_pTargetEntity->id() == eid)
			m_pTargetEntity = NULL;

		if (m_pMouseTarget && m_pMouseTarget->id() == eid)
			m_pMouseTarget = NULL;

		m_Entities.erase(eid);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVEWORLD - %s - EntityID %d\n", bPlayer ? "PLAYER" : "OTHER", (int)eid);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_ENTERSPACE:
	{
		const KBEngine::EventData_EnterSpace* pEventData_EnterSpace = static_cast<const KBEngine::EventData_EnterSpace*>(lpEventData);
		KBEngine::ENTITY_ID eid = pEventData_EnterSpace->entityID;

		ENTITIES::iterator iter = m_Entities.find(eid);
		if (iter == m_Entities.end())
		{
			bool bPlayer = false;
			if (kbe_playerID() == eid)
				bPlayer = true;
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERSPACE - %s - EntityID: %d (NOT IN LIST), SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
				(int)pEventData_EnterSpace->entityID, (int)pEventData_EnterSpace->spaceID,
				pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll,
				pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z,
				pEventData_EnterSpace->speed, pEventData_EnterSpace->isOnGround, pEventData_EnterSpace->res.c_str());
			kbengine_PrintMessage(str, true);
			break;
		}

		KBEntity* pEntity = iter->second.get();

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
			//m_pPlayer = pEntity;
		}

		pEntity->setPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
		if (bPlayer)
			pEntity->setDesideredPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
		pEntity->setDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
		if (bPlayer)
			pEntity->setDesideredDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
		pEntity->setMoveSpeed(pEventData_EnterSpace->speed);
		pEntity->setSpaceID(pEventData_EnterSpace->spaceID);
		pEntity->setIsOnGround(pEventData_EnterSpace->isOnGround);
		pEntity->setRes(pEventData_EnterSpace->res);

		pEntity->setIsInWorld(true);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERSPACE - %s - EntityID: %d, SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
			(int)pEventData_EnterSpace->entityID, (int)pEventData_EnterSpace->spaceID,
			pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll,
			pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z,
			pEventData_EnterSpace->speed, pEventData_EnterSpace->isOnGround, pEventData_EnterSpace->res.c_str());
		kbengine_PrintMessage(str, true);

		if (bPlayer)
		{
			setAppMode(TheWorld_ClientApp_GL::GraphicMode);
			m_pGLClientApp->playerEnterSpace(pEventData_EnterSpace->spaceID);
		}
	}
	break;

	case CLIENT_EVENT_LEAVESPACE:
	{
		KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveSpace*>(lpEventData)->entityID;
		KBEngine::SPACE_ID spaceId = static_cast<const KBEngine::EventData_LeaveSpace*>(lpEventData)->spaceID;

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
			//m_pPlayer = NULL;
		}

		if (m_pTargetEntity && m_pTargetEntity->id() == eid)
			m_pTargetEntity = NULL;

		if (m_pMouseTarget && m_pMouseTarget->id() == eid)
			m_pMouseTarget = NULL;

		m_Entities.erase(eid);
		sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVESPACE - %s - EntityID %d, SpaceID: %d\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, (int)spaceId);
		kbengine_PrintMessage(str);

		if (bPlayer)
		{
			m_pGLClientApp->playerLeaveSpace(spaceId);
		}
	}
	break;

	case CLIENT_EVENT_CREATEDENTITY:
	{
		const KBEngine::EventData_CreatedEntity* pEventData_createEntity = static_cast<const KBEngine::EventData_CreatedEntity*>(lpEventData);
		KBEngine::ENTITY_ID eid = pEventData_createEntity->entityID;
		KBEntity* pEntity = NULL;

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
			pEntity = new PlayerEntity(eid, m_pSpaceWorld);
			m_pPlayerEntity = pEntity;
		}
		else
			pEntity = new OtherEntity(eid, m_pSpaceWorld);
		pEntity->setModelScale(pEventData_createEntity->modelScale);
		m_Entities[eid].reset(pEntity);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_CREATEDENTITY - %s - EntityID: %d, ModelScale: %f\n", bPlayer ? "PLAYER" : "OTHER", (int)pEventData_createEntity->entityID, (float)pEventData_createEntity->modelScale);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_LOGIN_SUCCESS:
	{
		setLoginStatus(LOGIN_DONE);
		strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_SUCCESS\n");
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_LOGIN_FAILED:
	{
		setLoginStatus(LOGIN_NOT_DONE);
		const KBEngine::EventData_LoginFailed* info = static_cast<const KBEngine::EventData_LoginFailed*>(lpEventData);

		if (info->failedcode == 20)
		{
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Server is starting, please wait!\n");
		}
		else
		{
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Login is failed (code=%u)!\n", info->failedcode);
		}

		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS:
	{
		strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS\n");
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_LOGIN_BASEAPP_FAILED:
	{
		const KBEngine::EventData_LoginBaseappFailed* info = static_cast<const KBEngine::EventData_LoginBaseappFailed*>(lpEventData);
		sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_FAILED (code=%u)!\n", info->failedcode);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_SCRIPT:
	{
		const KBEngine::EventData_Script* peventdata = static_cast<const KBEngine::EventData_Script*>(lpEventData);

		Json::Reader reader;
		Json::Value root;

		if (!reader.parse(peventdata->datas.c_str(), root))
		{
			assert(false);
		}

		if (peventdata->name == "update_avatars")
		{
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, peventdata->name: ");
			strcat(str, peventdata->name.c_str());
			strcat(str, "\n");
			kbengine_PrintMessage(str);

			setInitAppModeRequired(true);

			m_Avatars.clear();

			Json::Value::Members mem = root.getMemberNames();
			for (auto iter = mem.begin(); iter != mem.end(); iter++)
			{
				Json::Value& val = root[*iter];
				std::string name = val[1].asString();
				KBEngine::DBID avatarDBID = val[Json::Value::UInt(0)].asUInt();
				KBAvatar* pAvatar = new KBAvatar(avatarDBID, name);
				m_Avatars[avatarDBID].reset(pAvatar);
				sprintf(str, "\t\tAvatar DBID: [%ld] , Avatar Name: [%s]\n", (long)avatarDBID, name.c_str());
				kbengine_PrintMessage(str, true);
			}
		}
		else
		{
			KBEngine::ENTITY_ID eid = root[Json::Value::UInt(0)].asInt();
			ENTITIES::iterator iter = m_Entities.find(eid);
			KBEntity* pEntity = NULL;

			if (iter != m_Entities.end())
				pEntity = iter->second.get();

			if (peventdata->name == "set_name")
			{
				if (pEntity == NULL)
				{
					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
					kbengine_PrintMessage(str);
					break;
				}

				std::string name = root[1].asString();
				pEntity->setName(name);

				sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, name: %s\n", (int)eid, peventdata->name.c_str(), name.c_str());
				kbengine_PrintMessage(str);
			}
			else if (peventdata->name == "set_class_name")
			{
				if (pEntity == NULL)
				{
					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
					kbengine_PrintMessage(str);
					break;
				}

				std::string className = root[1].asString();
				pEntity->setClassName(className);

				sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, name: %s\n", (int)eid, peventdata->name.c_str(), className.c_str());
				kbengine_PrintMessage(str);
			}
			else if (peventdata->name == "set_modelScale")
			{
				if (pEntity == NULL)
				{
					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
					kbengine_PrintMessage(str);
					break;
				}

				uint32_t scale = root[1].asUInt();
				pEntity->setModelScale(scale / (float)100.0);

				sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, modelScale: %d\n", (int)eid, peventdata->name.c_str(), (int)scale);
				kbengine_PrintMessage(str);
			}
			else if (peventdata->name == "set_modelID")
			{
				if (pEntity == NULL)
				{
					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
					kbengine_PrintMessage(str);
					break;
				}

				uint32_t modelID = root[1].asUInt();
				pEntity->setModelID(modelID);

				sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, modelID: %d\n", (int)eid, peventdata->name.c_str(), (int)modelID);
				kbengine_PrintMessage(str);
			}
			else if (peventdata->name == "set_state")
			{
				if (pEntity == NULL)
				{
					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
					kbengine_PrintMessage(str);
					break;
				}

				int32_t state = root[1].asInt();
				pEntity->setState(state);

				sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, state: %d\n", (int)eid, peventdata->name.c_str(), (int)state);
				kbengine_PrintMessage(str);
			}
			else if (peventdata->name == "set_HP_Max")
			{
				if (pEntity == NULL)
				{
					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
					kbengine_PrintMessage(str);
					break;
				}

				int32_t v = root[1].asInt();
				pEntity->setHPMax(v);

				sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, HPMax: %d\n", (int)eid, peventdata->name.c_str(), (int)v);
				kbengine_PrintMessage(str);
			}
			else if (peventdata->name == "set_MP_Max")
			{
				if (pEntity == NULL)
				{
					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
					kbengine_PrintMessage(str);
					break;
				}

				int32_t v = root[1].asInt();
				pEntity->setMPMax(v);

				sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, MPMax: %d\n", (int)eid, peventdata->name.c_str(), (int)v);
				kbengine_PrintMessage(str);
			}
			else if (peventdata->name == "recvDamage")
			{
				KBEngine::ENTITY_ID attackerID = root[1].asInt();
				uint32_t skillID = root[2].asUInt();
				uint32_t damageType = root[3].asUInt();
				uint32_t damage = root[4].asUInt();

				ENTITIES::iterator iter = m_Entities.find(attackerID);

				KBEntity* attacker = NULL;
				KBEntity* receiver = pEntity;
				KBEngine::ENTITY_ID eidAttacker = 0;

				if (iter != m_Entities.end())
				{
					attacker = iter->second.get();
				}

				if (attacker)
				{
					attacker->attack(receiver, skillID, damageType, damage);
				}

				if (receiver)
				{
					receiver->recvDamage(attacker, skillID, damageType, damage);
				}

				sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, EID Attacker: %d, skillId: %d, damageType: %d, damage: %d\n", (int)eid, peventdata->name.c_str(), (int)eidAttacker, (int)skillID, (int)damageType, (int)damage);
				kbengine_PrintMessage(str);
			}
		}
		// TODO
	}
	break;

	case CLIENT_EVENT_POSITION_CHANGED:
	{
		const KBEngine::EventData_PositionChanged* pEventData = static_cast<const KBEngine::EventData_PositionChanged*>(lpEventData);
		KBEngine::ENTITY_ID eid = pEventData->entityID;

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
		}

		ENTITIES::iterator iter = m_Entities.find(eid);
		if (iter == m_Entities.end())
		{
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d (NOT IN LIST), X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			kbengine_PrintMessage(str);
			break;
		}

		iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);
		if (bPlayer)
			iter->second->setDesideredPosition(pEventData->x, pEventData->y, pEventData->z);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d, X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_DIRECTION_CHANGED:
	{
		const KBEngine::EventData_DirectionChanged* pEventData = static_cast<const KBEngine::EventData_DirectionChanged*>(lpEventData);
		KBEngine::ENTITY_ID eid = pEventData->entityID;

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
		}

		ENTITIES::iterator iter = m_Entities.find(eid);
		if (iter == m_Entities.end())
		{
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d (NOT IN LIST), Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			kbengine_PrintMessage(str);
			break;
		}

		iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
		if (bPlayer)
			iter->second->setDesideredDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_MOVESPEED_CHANGED:
	{
		const KBEngine::EventData_MoveSpeedChanged* pEventData = static_cast<const KBEngine::EventData_MoveSpeedChanged*>(lpEventData);
		KBEngine::ENTITY_ID eid = pEventData->entityID;

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
		}

		ENTITIES::iterator iter = m_Entities.find(eid);
		if (iter == m_Entities.end())
		{
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d (NOT IN LIST), MoveSpeed %f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
			kbengine_PrintMessage(str);
			break;
		}

		iter->second->setMoveSpeed(pEventData->speed);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d, MoveSpeed %f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_SERVER_CLOSED:
	{
		m_bServerClosed = true;
		setShutdownRequired(true);
		setLoginStatus(LOGIN_NOT_DONE);
		setInitAppModeRequired(false);
		strcpy(str, "KBE Event received ==> CLIENT_EVENT_SERVER_CLOSED\n");
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_POSITION_FORCE:
	{
		const KBEngine::EventData_PositionForce* pEventData = static_cast<const KBEngine::EventData_PositionForce*>(lpEventData);
		KBEngine::ENTITY_ID eid = pEventData->entityID;

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
		}

		ENTITIES::iterator iter = m_Entities.find(eid);
		if (iter == m_Entities.end())
		{
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d (NOT IN LIST), X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			kbengine_PrintMessage(str);
			break;
		}

		iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);
		if (bPlayer)
			iter->second->setDesideredPosition(pEventData->x, pEventData->y, pEventData->z);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d, X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_DIRECTION_FORCE:
	{
		const KBEngine::EventData_DirectionForce* pEventData = static_cast<const KBEngine::EventData_DirectionForce*>(lpEventData);
		KBEngine::ENTITY_ID eid = pEventData->entityID;

		bool bPlayer = false;
		if (kbe_playerID() == eid)
		{
			bPlayer = true;
		}

		ENTITIES::iterator iter = m_Entities.find(eid);
		if (iter == m_Entities.end())
		{
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d (NOT IN LIST), Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			kbengine_PrintMessage(str);
			break;
		}

		iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
		if (bPlayer)
			iter->second->setDesideredDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_ADDSPACEGEOMAPPING:
	{
		const KBEngine::EventData_AddSpaceGEOMapping* pEventData = static_cast<const KBEngine::EventData_AddSpaceGEOMapping*>(lpEventData);
		KBEngine::SPACE_ID spaceID = pEventData->spaceID;
		m_pSpaceWorld->addSpace(spaceID, pEventData->respath);
		sprintf(str, "KBE Event received ==> CLIENT_EVENT_ADDSPACEGEOMAPPING, SpaceID %d, ResPath %s\n", (int)spaceID, pEventData->respath.c_str());
		kbengine_PrintMessage(str, true);
	}
	break;

	case CLIENT_EVENT_VERSION_NOT_MATCH:
	{
		const KBEngine::EventData_VersionNotMatch* info = static_cast<const KBEngine::EventData_VersionNotMatch*>(lpEventData);
		sprintf(str, "KBE Event received ==> CLIENT_EVENT_VERSION_NOT_MATCH - VerInfo=%s not match(server:%s)\n", info->verInfo.c_str(), info->serVerInfo.c_str());
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_ON_KICKED:
	{
		const KBEngine::EventData_onKicked* info = static_cast<const KBEngine::EventData_onKicked*>(lpEventData);
		sprintf(str, "KBE Event received ==> CLIENT_EVENT_ON_KICKED (code=%u)!\n", info->failedcode);
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_LAST_ACCOUNT_INFO:
	{
		const KBEngine::EventData_LastAccountInfo* info = static_cast<const KBEngine::EventData_LastAccountInfo*>(lpEventData);
		setAccountName(info->name);

		sprintf(str, "KBE Event received ==> CLIENT_EVENT_LAST_ACCOUNT_INFO - Last account name: %s\n", info->name.c_str());
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH:
	{
		const KBEngine::EventData_ScriptVersionNotMatch* info = static_cast<const KBEngine::EventData_ScriptVersionNotMatch*>(lpEventData);
		sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH - ScriptVerInfo=%s not match(server:%s)\n", info->verInfo.c_str(), info->serVerInfo.c_str());
		kbengine_PrintMessage(str);
	}
	break;

	case CLIENT_EVENT_UNKNOWN:
	default:
	{
		//kbe_fireEvent("reset", NULL);
		//kbe_fireEvent("relive", NULL);
		strcpy(str, "KBE Event received ==> CLIENT_EVENT_UNKNOWN\n");
		kbengine_PrintMessage(str);
	}
	break;
	};
}

int main(int argc, char* argv[])
{
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
