// TheWorld_ConsoleClient.cpp : Defines the entry point for the console application.
//

/*

	TODO: 
		* Definire in kbengine_demos_assets\res\spaces\TheWorld_ConsoleClient le informazioni riguardo le risorse spaziali (collision information etc.). "TheWorld_ConsoleClient" è è l'identiificativo passato al login.
		* Capire a cosa serve kbe_updateVolatile chiamato in SpaceWorld::frameRenderingQueued in Ogre Demo
		* Capire il significato di Entity, EntityComplex, EntitySimple, Space* in Ogre Demo

*/

#include "stdafx.h"

#include "SDL.h"
#include "SDL_opengl.h"
#include <GL/glu.h>
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl.h"

#include "TheWorld_ConsoleClient.h"
#include "TheWorld_ConsoleClient_GL.h"
#include <conio.h>
#include <conio.h>
#include <json/json.h>
#include <boost/thread/thread.hpp>
#include "PlayerEntity.h"
#include "OtherEntity.h"

HINSTANCE g_hKBEngineDll = NULL;

// Default Account Name
std::string g_defaultAccountName("6529868038458114048");
//------

TheWorld_ClientApp::TheWorld_ClientApp() :
	events_()
{
	m_pGLClientApp = new TheWorld_ClientApp_GL(this);

	m_bDoSleepInMainLoop = true;
	m_iSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
	m_iLogin = LOGIN_NOT_DONE;
	m_bShutDown = false;
	kbe_registerEventHandle(this);
	m_pSpaceWorld = new SpaceWorld;
	m_pPlayer = m_pTarget = m_pMouseTarget = NULL;
	m_bServerClosed = false;
	setAppMode(TheWorld_ClientApp_GL::InitialMenu, true);
}

TheWorld_ClientApp::~TheWorld_ClientApp()
{
	delete m_pGLClientApp;

	kbe_deregisterEventHandle(this);
}

void TheWorld_ClientApp::go(void)
{
	if (!init())
		return;

	doMainLoop();

	cleanup();
}

bool TheWorld_ClientApp::init(void)
{
	if (!m_pGLClientApp->init())
		return false;

	return true;
}

void TheWorld_ClientApp::cleanup(void)
{
	m_pGLClientApp->cleanup();
}

void TheWorld_ClientApp::doMainLoop(void)
{
	while (!m_bShutDown)
	{
		messagePump();

		if (getAppMode() == TheWorld_ClientApp_GL::InitialMenu)
			manageInitialMenu();
		else
			manageGraphicRendering();

		if (m_bDoSleepInMainLoop)
			kbe_sleep(1);
	}
}
	
void TheWorld_ClientApp::manageInitialMenu(void)
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
				m_bShutDown = true;
				m_bDoSleepInMainLoop = false;
				kbe_shutdown();
			}
			else
				doInitialMenuAction(ch);
		}
	}
}

void TheWorld_ClientApp::manageGraphicRendering(void)
{
	bool bLogoutRequired = false;
	if (!m_pGLClientApp->handleGraphicRendering(bLogoutRequired, m_iLogin))
	{
		kbengine_Logout();
		setAppMode(TheWorld_ClientApp_GL::InitialMenu);
		return;
	}

	if (bLogoutRequired)
	{
		kbengine_Logout();
		setAppMode(TheWorld_ClientApp_GL::InitialMenu);
	}
}

void TheWorld_ClientApp::setAppMode(enum TheWorld_ClientApp_GL::_AppMode r, bool bForce)
{
	m_pGLClientApp->setAppMode(r, bForce);
}

enum TheWorld_ClientApp_GL::_AppMode TheWorld_ClientApp::getAppMode(void)
{
	return m_pGLClientApp->getAppMode();
}

bool TheWorld_ClientApp::getInitAppModeRequired(void)
{
	return m_pGLClientApp->getInitAppModeRequired();
}

void TheWorld_ClientApp::setInitAppModeRequired(bool b)
{
	m_pGLClientApp->setInitAppModeRequired(b);
}

void TheWorld_ClientApp::doInitialMenuAction(int ch)
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
			sprintf(str, "kbe_callEntityMethod: reqRemoveAvatar - Player ID: %d, Avatar name %s\n", (int)kbe_playerID(), avatarName.c_str());
			//printf(str);	printf("\n");
			printMessage(str, true);
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
			if (m_iLogin == LOGIN_STARTED)
			{
				printf("Login in corso!\n");
			}
			else
			{
				kbengine_Logout();
				setInitAppModeRequired(true);
			}
			break;
		}
		case 'l':
		case 'L':
		{
			if (m_iLogin == LOGIN_DONE)
			{
				printf("Login già effettuato!\n");
			}
			else if (m_iLogin == LOGIN_STARTED)
			{
				printf("Login in corso!\n");
			}
			else
			{
				std::string datas = "TheWorld_ConsoleClient";
				if (kbengine_Login(NULL, "123456", datas.data(), KBEngine::uint32(datas.size())));
					//m_iLogin = LOGIN_STARTED;
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
			if (m_iLogin == LOGIN_DONE)
			{
				KBEngine::uint64 uuid = kbe_genUUID64();
				std::stringstream ss;
				ss << uuid;
				std::string avatarName = ss.str();
				bool b = kbengine_CreateAvatar(avatarName);
				sprintf(str, "kbe_callEntityMethod: reqCreateAvatar - Player ID: %d\n", (int)kbe_playerID());
				printMessage(str);
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
		case 'r':
		case 'R':
		{
			if (m_iLogin == LOGIN_DONE)
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
			printf("PlayerID: %d\n", (int)kbe_playerID());
			printf("\n");
			printf("*** %s *********************\n", (ch == 'd' ? "DUMP" : "MINIDUMP"));
			printf("\n");
			/*if (minidump)
			{
				kbe_lock();
				char str[256];
				sprintf(str, "[\"Parametro_1\", \"Parametro_2\"]");
				kbe_callEntityMethod(kbe_playerID(), "ping", str);
				kbe_unlock();
			}*/
			break;
		}
		// '1' / '9'
		default:
		{
			if (m_iLogin == LOGIN_DONE)
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
						sprintf(str, "kbe_callEntityMethod: selectAvatarGame - Player ID: %d, Avatar DBID %ld\n", (int)kbe_playerID(), (long)pAvatar->getAvatarID());
						//printf(str);	printf("\n");
						printMessage(str, true);
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

void TheWorld_ClientApp::messagePump(void)
{
	while (true)
	{
		m_KbeEventsMutex.lock();

		if (events_.empty())
		{
			m_KbeEventsMutex.unlock();
			break;
		}

		std::tr1::shared_ptr<const KBEngine::EventData> pEventData = events_.front();
		events_.pop();
		m_KbeEventsMutex.unlock();

		KBEngine::EventID id = pEventData->id;

		if (id == CLIENT_EVENT_SCRIPT)
		{
			kbe_lock();
		}

		onEvent(pEventData.get());

		if (id == CLIENT_EVENT_SCRIPT)
		{
			kbe_unlock();
		}

		m_bDoSleepInMainLoop = true;
	}
}


void TheWorld_ClientApp::onEvent(const KBEngine::EventData* lpEventData)
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
				printMessage(str, true);
				break;
			}

			KBEntity* pEntity = iter->second.get();

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				m_pPlayer = pEntity;
			}

			pEntity->setDestPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
			pEntity->setPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
			pEntity->setDestDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
			pEntity->setDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
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
			printMessage(str, true);

			if (bPlayer)
				setAppMode(TheWorld_ClientApp_GL::GraphicMode);
		}
		break;
	
		case CLIENT_EVENT_LEAVEWORLD:
		{
			KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveWorld*>(lpEventData)->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				m_pPlayer = NULL;
			}

			if (m_pTarget && m_pTarget->id() == eid)
				m_pTarget = NULL;

			if (m_pMouseTarget && m_pMouseTarget->id() == eid)
				m_pMouseTarget = NULL;

			m_Entities.erase(eid);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVEWORLD - %s - EntityID %d\n", bPlayer ? "PLAYER" : "OTHER", (int)eid);
			printMessage(str);
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
				printMessage(str, true);
				break;
			}

			KBEntity* pEntity = iter->second.get();

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				m_pPlayer = pEntity;
			}

			pEntity->setDestPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
			pEntity->setPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
			pEntity->setDestDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
			pEntity->setDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
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
			printMessage(str, true);

			if (bPlayer)
			{
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
				m_pPlayer = NULL;
			}

			if (m_pTarget && m_pTarget->id() == eid)
				m_pTarget = NULL;

			if (m_pMouseTarget && m_pMouseTarget->id() == eid)
				m_pMouseTarget = NULL;

			m_Entities.erase(eid);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVESPACE - %s - EntityID %d, SpaceID: %d\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, (int)spaceId);
			printMessage(str);

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
				m_pPlayer = pEntity;
			}
			else
				pEntity = new OtherEntity(eid, m_pSpaceWorld);
			pEntity->setModelScale(pEventData_createEntity->modelScale);
			m_Entities[eid].reset(pEntity);
				
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_CREATEDENTITY - %s - EntityID: %d, ModelScale: %f\n", bPlayer ? "PLAYER" : "OTHER", (int)pEventData_createEntity->entityID, (float)pEventData_createEntity->modelScale);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_SUCCESS:
		{
			m_iLogin = LOGIN_DONE;
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_SUCCESS\n");
			printMessage(str);
		}
		break;
		
		case CLIENT_EVENT_LOGIN_FAILED:
		{
			m_iLogin = LOGIN_NOT_DONE;
			const KBEngine::EventData_LoginFailed* info = static_cast<const KBEngine::EventData_LoginFailed*>(lpEventData);

			if (info->failedcode == 20)
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Server is starting, please wait!\n");
			}
			else
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Login is failed (code=%u)!\n", info->failedcode);
			}

			printMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS:
		{
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS\n");
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_BASEAPP_FAILED:
		{
			const KBEngine::EventData_LoginBaseappFailed* info = static_cast<const KBEngine::EventData_LoginBaseappFailed*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_FAILED (code=%u)!\n", info->failedcode);
			printMessage(str);
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
				strcat(str,"\n");
				printMessage(str);

				setInitAppModeRequired(true);

				m_Avatars.clear();
				
				Json::Value::Members mem = root.getMemberNames();
				for (auto iter = mem.begin(); iter != mem.end(); iter++)
				{
					Json::Value& val = root[*iter];
					std::string name = val[1].asString();
					KBEngine::DBID avatarDBID = val[Json::Value::UInt(0)].asUInt();
					KBAvatar *pAvatar = new KBAvatar(avatarDBID, name);
					m_Avatars[avatarDBID].reset(pAvatar);
					sprintf(str, "\t\tAvatar DBID: [%ld] , Avatar Name: [%s]\n", (long)avatarDBID, name.c_str());
					printMessage(str, true);
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
						printMessage(str);
						break;
					}

					std::string name = root[1].asString();
					pEntity->setName(name);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, name: %s\n", (int)eid, peventdata->name.c_str(), name.c_str());
					printMessage(str);
				}
				else if (peventdata->name == "set_modelScale")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					uint32_t scale = root[1].asUInt();
					pEntity->setModelScale(scale / (float)100.0);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, modelScale: %d\n", (int)eid, peventdata->name.c_str(), (int)scale);
					printMessage(str);
				}
				else if (peventdata->name == "set_modelID")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					uint32_t modelID = root[1].asUInt();
					pEntity->setModelID(modelID);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, modelID: %d\n", (int)eid, peventdata->name.c_str(), (int)modelID);
					printMessage(str);
				}
				else if (peventdata->name == "set_state")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					int32_t state = root[1].asInt();
					pEntity->setState(state);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, state: %d\n", (int)eid, peventdata->name.c_str(), (int)state);
					printMessage(str);
				}
				else if (peventdata->name == "set_HP_Max")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					int32_t v = root[1].asInt();
					pEntity->setHPMax(v);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, HPMax: %d\n", (int)eid, peventdata->name.c_str(), (int)v);
					printMessage(str);
				}
				else if (peventdata->name == "set_MP_Max")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					int32_t v = root[1].asInt();
					pEntity->setMPMax(v);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, MPMax: %d\n", (int)eid, peventdata->name.c_str(), (int)v);
					printMessage(str);
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
					printMessage(str);
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
				printMessage(str);
				break;
			}

			iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d, X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			printMessage(str);
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
				printMessage(str);
				break;
			}

			iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
			
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			printMessage(str);
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
				printMessage(str);
				break;
			}

			iter->second->setMoveSpeed(pEventData->speed);
			
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d, MoveSpeed %f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_SERVER_CLOSED:
		{
			m_bServerClosed = true;
			m_bShutDown = true;
			m_iLogin = LOGIN_NOT_DONE;
			setInitAppModeRequired(false);
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_SERVER_CLOSED\n");
			printMessage(str);
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
				printMessage(str);
				break;
			}

			iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);
			iter->second->setDestPosition(pEventData->x, pEventData->y, pEventData->z);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d, X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			printMessage(str);
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
				printMessage(str);
				break;
			}

			iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
			iter->second->setDestDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_ADDSPACEGEOMAPPING:
			{
				const KBEngine::EventData_AddSpaceGEOMapping* pEventData = static_cast<const KBEngine::EventData_AddSpaceGEOMapping*>(lpEventData);
				KBEngine::SPACE_ID spaceID = pEventData->spaceID;
				m_pSpaceWorld->addSpace(spaceID, pEventData->respath);
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_ADDSPACEGEOMAPPING, SpaceID %d, ResPath %s\n", (int)spaceID, pEventData->respath.c_str());
				printMessage(str, true);
			}
			break;

		case CLIENT_EVENT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_VersionNotMatch* info = static_cast<const KBEngine::EventData_VersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_VERSION_NOT_MATCH - VerInfo=%s not match(server:%s)\n", info->verInfo.c_str(), info->serVerInfo.c_str());
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_ON_KICKED:
		{
			const KBEngine::EventData_onKicked* info = static_cast<const KBEngine::EventData_onKicked*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ON_KICKED (code=%u)!\n", info->failedcode);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_LAST_ACCOUNT_INFO:
		{
			const KBEngine::EventData_LastAccountInfo* info = static_cast<const KBEngine::EventData_LastAccountInfo*>(lpEventData);
			m_sAccountName = info->name;

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LAST_ACCOUNT_INFO - Last account name: %s\n", info->name.c_str());
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_ScriptVersionNotMatch* info = static_cast<const KBEngine::EventData_ScriptVersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH - ScriptVerInfo=%s not match(server:%s)\n", info->verInfo.c_str(), info->serVerInfo.c_str());
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_UNKNOWN:
		default:
		{
			//kbe_fireEvent("reset", NULL);
			//kbe_fireEvent("relive", NULL);
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_UNKNOWN\n");
			printMessage(str);
		}
		break;
	};
}

void TheWorld_ClientApp::kbengine_onEvent(const KBEngine::EventData* lpEventData)
{
	KBEngine::EventData* peventdata = KBEngine::copyKBEngineEvent(lpEventData);

	if (peventdata)
	{
		boost::mutex::scoped_lock lock(m_KbeEventsMutex);
		events_.push(std::tr1::shared_ptr<const KBEngine::EventData>(peventdata));
		m_bDoSleepInMainLoop = false;
	}
}

bool TheWorld_ClientApp::kbengine_Login(const char* accountname, const char* passwd, const char* datas, KBEngine::uint32 datasize,	const char* ip, KBEngine::uint32 port)
{
	std::string accountName;
	if (accountname == NULL)
	{
		m_sAccountName = kbe_getLastAccountName();
		if (m_sAccountName.size() == 0)
		{
			/*KBEngine::uint64 uuid = kbe_genUUID64();
			std::stringstream ss;
			ss << uuid;
			g_accountName = ss.str();
			//g_accountName = KBEngine::StringConv::val2str(KBEngine::genUUID64());*/
			m_sAccountName = g_defaultAccountName;
		}
		accountName = m_sAccountName;
	}
	else
		accountName = accountname;

	char str[256];
	sprintf(str, "KBE Login - AccountName: [%s], Password: [%s], Client Name: [%s]\n", accountName.c_str(), passwd, datas);
	printMessage(str);
	if (kbe_login(accountName.c_str(), passwd, datas, datasize, ip, port))
	{
		m_iLogin = LOGIN_STARTED;
		return true;
	}
	else
		return false;
}

bool TheWorld_ClientApp::kbengine_Logout()
{
	printMessage("KBE Logout\n");
	kbengine_Reset();
	m_Avatars.clear();
	m_iLogin = LOGIN_NOT_DONE;

	return true;
}
	
bool TheWorld_ClientApp::kbengine_CreateAvatar(std::string avatarName)
{
	char str[256];
	sprintf(str, "KBE Create Avatar - Name: [%s]\n", avatarName.c_str());
	printMessage(str);

	kbe_lock();
	sprintf(str, "[1, \"kbeconsole_%s\"]", avatarName.c_str());
	kbe_callEntityMethod(kbe_playerID(), "reqCreateAvatar", str);
	kbe_unlock();

	return true;
}

bool TheWorld_ClientApp::kbengine_RemoveAvatar(std::string avatarName)
{
	char str[256];
	sprintf(str, "KBE Remove Avatar - Name: [%s]\n", avatarName.c_str());
	printMessage(str);

	kbe_lock();
	kbe_callEntityMethod(kbe_playerID(), "reqRemoveAvatar", avatarName.c_str());
	kbe_unlock();

	return true;
}

bool TheWorld_ClientApp::kbengine_SelectAvatarGame(KBEngine::DBID avatarDBID)
{
	char str[256];
	sprintf(str, "KBE Select Avatar Game - DBID: [%d]\n", (int)avatarDBID);
	printMessage(str);

	kbe_lock();
	kbe_callEntityMethod(kbe_playerID(), "selectAvatarGame", KBEngine::StringConv::val2str(avatarDBID).c_str());
	kbe_unlock();

	return true;
}

void TheWorld_ClientApp::kbengine_Reset(void)
{
	kbe_reset();
}

void TheWorld_ClientApp::printMessage(char *message, bool interline, bool console, KBEMessageType t)
{
	if (message[strlen(message) - 1] != '\n')
		kbe_printMessage("Next message without trailing New Line", t);
	kbe_printMessage(message, t);
	if (console)
	{
		printf(message);
		if (interline)
			printf("\n");
	}
}

int main(int argc, char *argv[])
{
#ifdef _DEBUG
	std::string kbenginedll_name = "TheWorld_ClientDll_d.dll";
#else
	std::string kbenginedll_name = "TheWorld_ClientDll.dll";
#endif

	g_hKBEngineDll = LoadLibrary(kbenginedll_name.c_str());
	if (g_hKBEngineDll == NULL)
	{
		std::string kbenginedll_name_failed = "load " + kbenginedll_name + " is failed!";
		MessageBox(NULL, kbenginedll_name_failed.c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return 0;
	}

	if (!kbe_init())
	{
		MessageBox(NULL, "kbengine_init() is failed!", "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return 0;
	}
	
	TheWorld_ClientApp *app = new TheWorld_ClientApp();

	// TODO: login
	try
	{
		app->go();
	}
	catch (...)
	{
		MessageBox(NULL, "An exception has occured!", "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	
	delete app;
	kbe_destroy();

	FreeLibrary(g_hKBEngineDll);

	return 0;
}