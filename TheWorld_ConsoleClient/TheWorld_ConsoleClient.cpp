// TheWorld_ConsoleClient.cpp : Defines the entry point for the console application.
//

/*

	TODO: 
		* Definire in kbengine_demos_assets\res\spaces\TheWorld_ConsoleClient le informazioni riguardo le risorse spaziali (collision information etc.). "TheWorld_ConsoleClient" è è l'identiificativo passato al login.
		* Capire a cosa serve kbe_updateVolatile chiamato in SpaceWorld::frameRenderingQueued in Ogre Demo
		* Capire il significato di Entity, EntityComplex, EntitySimple, Space* in Ogre Demo

*/

#include "stdafx.h"
#include "TheWorld_ConsoleClient.h"
#include <conio.h>
#include <conio.h>
#include <json/json.h>
#include <boost/thread/thread.hpp>
#include "PlayerEntity.h"
#include "OtherEntity.h"

// Default Account Name
std::string g_defaultAccountName("6529868038458114048");
//------

TheWorld_UIClientApp::TheWorld_UIClientApp() :
	events_()
{
	mHasEvent = false;
	mDisplayActions = 1;
	mSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
	mLogin = LOGIN_NOT_DONE;
	mShutDown = false;
	kbe_registerEventHandle(this);
	mSpaceWorldPtr = new SpaceWorld;
	mPlayerPtr = mTargetPtr = mMouseTargetPtr = NULL;
	mServerClosed = false;
}

TheWorld_UIClientApp::~TheWorld_UIClientApp()
{
	kbe_deregisterEventHandle(this);
}

void TheWorld_UIClientApp::go(void)
{
	if (!setup())
		return;

	while (!mShutDown)
	{
		messagePump();

		if (mDisplayActions == 1)
		{
			mDisplayActions = 2;
			printf("\n");
			printf("Actions: \n");
			printf("\n");
			printf("   l - Login\n");
			printf("   x - Reset\n");
			printf("\n");
			printf("   c - Create Avatar\n");
			printf("   r - Remove Avatar\n");
			printf("\n");
			printf("   d - Dump Status\n");
			printf("   m - Minidump\n");
			printf("\n");
			printf("   z - Display Menu\n");
			printf("\n");
			AVATARS::iterator iter = mAvatars.begin();
			int idx = 0;
			for (; iter != mAvatars.end(); iter++)
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
			if (mSelectAvatarPending == SELECT_AVATAR_PENDING)
			{
				if (ch == '.' || ch == 27)
				{
				}
				else
				{
					int i = ch - 48 - 1;
					if (i >= 0 && i < mAvatars.size())
					{
						doAction(ch);
					}
				}
				mSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
				mDisplayActions = 1;
			}
			else
			{
				if (ch == '.' || ch == 27) {
					mShutDown = true;
					mHasEvent = true;
					kbe_shutdown();
				}
				else
					doAction(ch);
			}
		}
		
		if (!mHasEvent)
			kbe_sleep(1);
	}
}

void TheWorld_UIClientApp::doAction(int ch)
{
	char str[256];

	if (mSelectAvatarPending == SELECT_AVATAR_PENDING)
	{
		//mSelectAvatarPending == SELECT_AVATAR_PENDING
		int iAvatar = ch - 48;
		KBAvatar* pAvatar = NULL;
		AVATARS::iterator iter = mAvatars.begin();
		int idx = 1;
		for (; iter != mAvatars.end(); iter++)
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
			printf(str);	printf("\n");
		}
		mSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
		mDisplayActions = 1;
	}
		
	switch (ch)
	{
		case 'x':
		{
			if (mLogin == LOGIN_STARTED)
			{
				printf("Login in corso!\n");
			}
			else
			{
				kbengine_Logout();
				mLogin = LOGIN_NOT_DONE;
				mDisplayActions = 1;
			}
			break;
		}
		case 'l':
		{
			if (mLogin == LOGIN_DONE)
			{
				printf("Login già effettuato!\n");
			}
			else if (mLogin == LOGIN_STARTED)
			{
				printf("Login in corso!\n");
			}
			else
			{
				std::string datas = "TheWorld_ConsoleClient";
				if (kbengine_Login(NULL, "123456", datas.data(), KBEngine::uint32(datas.size())))
					mLogin = LOGIN_STARTED;
				else
					printf("Login error!\n");
			}
			break;
		}
		case 'z':
		{
			mDisplayActions = 1;
			break;
		}
		case 'c':
		{
			if (mLogin == LOGIN_DONE)
			{
				KBEngine::uint64 uuid = kbe_genUUID64();
				std::stringstream ss;
				ss << uuid;
				std::string avatarName = ss.str();
				bool b = kbengine_CreateAvatar(avatarName);
				sprintf(str, "kbe_callEntityMethod: reqCreateAvatar - Player ID: %d", (int)kbe_playerID());
				printf(str);	printf("\n");
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
		case 'r':
		{
			if (mLogin == LOGIN_DONE)
			{
				mSelectAvatarPending = SELECT_AVATAR_PENDING;
				printf("\n");
				printf("Select Avatar\n");
				printf("\n");
				AVATARS::iterator iter = mAvatars.begin();
				int idx = 0;
				for (; iter != mAvatars.end(); iter++)
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
		case 'm':
		{
			bool minidump = (ch == 'm' ? true : false);
			printf("\n");
			printf("*** %s *********************\n", (ch == 'd' ? "DUMP" : "MINIDUMP"));
			printf("\n");
			{
				ENTITIES::iterator iter = mEntities.begin();
				int idx = 0;
				for (; iter != mEntities.end(); iter++)
				{
					idx++;
					KBEntity* pEntity = iter->second.get();
					pEntity->dumpStatus(idx, minidump);
				}
			}
			printf("\n");
			mSpaceWorldPtr->dumpStatus(minidump);
			printf("\n");
			{
				AVATARS::iterator iter = mAvatars.begin();
				int idx = 0;
				for (; iter != mAvatars.end(); iter++)
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
			if (mLogin == LOGIN_DONE)
			{
				int i = ch - 48 - 1;
				if (i >= 0 && i < mAvatars.size())
				{
					KBAvatar* pAvatar = NULL;
					AVATARS::iterator iter = mAvatars.begin();
					int idx = 0;
					for (; iter != mAvatars.end(); iter++)
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
						printf(str);	printf("\n");
					}
				}
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
	}

	return;
};

bool TheWorld_UIClientApp::setup(void)
{
	return true;
};

void TheWorld_UIClientApp::messagePump(void)
{
	while (true)
	{
		mKbeEventsMutex.lock();

		if (events_.empty())
		{
			mKbeEventsMutex.unlock();
			break;
		}

		std::tr1::shared_ptr<const KBEngine::EventData> pEventData = events_.front();
		events_.pop();
		mKbeEventsMutex.unlock();

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

		mHasEvent = false;
	}
}


void TheWorld_UIClientApp::onEvent(const KBEngine::EventData* lpEventData)
{
	char str[256];

	switch (lpEventData->id)
	{
		case CLIENT_EVENT_ENTERWORLD:
		{
			const KBEngine::EventData_EnterWorld* pEventData_EnterWorld = static_cast<const KBEngine::EventData_EnterWorld*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData_EnterWorld->entityID;

			ENTITIES::iterator iter = mEntities.find(eid);
			if (iter == mEntities.end())
			{
				bool bPlayer = false;
				if (kbe_playerID() == eid)
					bPlayer = true;
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERWORLD - %s - EntityID: %d (NOT IN LIST), SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
					(int)pEventData_EnterWorld->entityID, (int)pEventData_EnterWorld->spaceID,
					pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll,
					pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z,
					pEventData_EnterWorld->speed, pEventData_EnterWorld->isOnGround, pEventData_EnterWorld->res.c_str());
				PrintMessage(str, true);
				break;
			}

			KBEntity* pEntity = iter->second.get();

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				mPlayerPtr = pEntity;
			}

			pEntity->setDestPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
			pEntity->setPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
			pEntity->setDestDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
			pEntity->setDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
			pEntity->setMoveSpeed(pEventData_EnterWorld->speed);
			pEntity->setSpaceID(pEventData_EnterWorld->spaceID);
			pEntity->setIsOnGround(pEventData_EnterWorld->isOnGround);
			pEntity->setRes(pEventData_EnterWorld->res);

			pEntity->inWorld(true);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERWORLD - %s - EntityID: %d, SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
				(int)pEventData_EnterWorld->entityID, (int)pEventData_EnterWorld->spaceID,
				pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll,
				pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z,
				pEventData_EnterWorld->speed, pEventData_EnterWorld->isOnGround, pEventData_EnterWorld->res.c_str());
			PrintMessage(str, true);
		}
		break;
	
		case CLIENT_EVENT_LEAVEWORLD:
		{
			KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveWorld*>(lpEventData)->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				mPlayerPtr = NULL;
			}

			if (mTargetPtr && mTargetPtr->id() == eid)
				mTargetPtr = NULL;

			if (mMouseTargetPtr && mMouseTargetPtr->id() == eid)
				mMouseTargetPtr = NULL;

			mEntities.erase(eid);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVEWORLD - %s - EntityID %d\n", bPlayer ? "PLAYER" : "OTHER", (int)eid);
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_ENTERSPACE:
		{
			const KBEngine::EventData_EnterSpace* pEventData_EnterSpace = static_cast<const KBEngine::EventData_EnterSpace*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData_EnterSpace->entityID;

			ENTITIES::iterator iter = mEntities.find(eid);
			if (iter == mEntities.end())
			{
				bool bPlayer = false;
				if (kbe_playerID() == eid)
					bPlayer = true;
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERSPACE - %s - EntityID: %d (NOT IN LIST), SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
					(int)pEventData_EnterSpace->entityID, (int)pEventData_EnterSpace->spaceID,
					pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll,
					pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z,
					pEventData_EnterSpace->speed, pEventData_EnterSpace->isOnGround, pEventData_EnterSpace->res.c_str());
				PrintMessage(str, true);
				break;
			}

			KBEntity* pEntity = iter->second.get();

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				mPlayerPtr = pEntity;
			}

			pEntity->setDestPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
			pEntity->setPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
			pEntity->setDestDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
			pEntity->setDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
			pEntity->setMoveSpeed(pEventData_EnterSpace->speed);
			pEntity->setSpaceID(pEventData_EnterSpace->spaceID);
			pEntity->setIsOnGround(pEventData_EnterSpace->isOnGround);
			pEntity->setRes(pEventData_EnterSpace->res);

			pEntity->inWorld(true);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERSPACE - %s - EntityID: %d, SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
				(int)pEventData_EnterSpace->entityID, (int)pEventData_EnterSpace->spaceID,
				pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll,
				pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z,
				pEventData_EnterSpace->speed, pEventData_EnterSpace->isOnGround, pEventData_EnterSpace->res.c_str());
			PrintMessage(str, true);
		}
		break;

		case CLIENT_EVENT_LEAVESPACE:
		{
			KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveSpace*>(lpEventData)->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				mPlayerPtr = NULL;
			}

			if (mTargetPtr && mTargetPtr->id() == eid)
				mTargetPtr = NULL;

			if (mMouseTargetPtr && mMouseTargetPtr->id() == eid)
				mMouseTargetPtr = NULL;

			mEntities.erase(eid);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVESPACE - %s - EntityID %d\n", bPlayer ? "PLAYER" : "OTHER", (int)eid);
			PrintMessage(str);
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
				pEntity = new PlayerEntity(eid, mSpaceWorldPtr);
				mPlayerPtr = pEntity;
			}
			else
				pEntity = new OtherEntity(eid, mSpaceWorldPtr);
			pEntity->setModelScale(pEventData_createEntity->modelScale);
			mEntities[eid].reset(pEntity);
				
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_CREATEDENTITY - %s - EntityID: %d, ModelScale: %f\n", bPlayer ? "PLAYER" : "OTHER", (int)pEventData_createEntity->entityID, (float)pEventData_createEntity->modelScale);
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_SUCCESS:
		{
			mLogin = LOGIN_DONE;
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_SUCCESS\n");
			PrintMessage(str);
		}
		break;
		
		case CLIENT_EVENT_LOGIN_FAILED:
		{
			mLogin = LOGIN_NOT_DONE;
			const KBEngine::EventData_LoginFailed* info = static_cast<const KBEngine::EventData_LoginFailed*>(lpEventData);

			if (info->failedcode == 20)
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Server is starting, please wait!\n");
			}
			else
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Login is failed (code=%u)!\n", info->failedcode);
			}

			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS:
		{
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS\n");
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_BASEAPP_FAILED:
		{
			const KBEngine::EventData_LoginBaseappFailed* info = static_cast<const KBEngine::EventData_LoginBaseappFailed*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_FAILED (code=%u)!\n", info->failedcode);
			PrintMessage(str);
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
				PrintMessage(str);

				//if (mDisplayActions == 0)
					mDisplayActions = 1;

				mAvatars.clear();
				
				Json::Value::Members mem = root.getMemberNames();
				for (auto iter = mem.begin(); iter != mem.end(); iter++)
				{
					Json::Value& val = root[*iter];
					std::string name = val[1].asString();
					KBEngine::DBID avatarDBID = val[Json::Value::UInt(0)].asUInt();
					KBAvatar *pAvatar = new KBAvatar(avatarDBID, name);
					mAvatars[avatarDBID].reset(pAvatar);
					sprintf(str, "\t\tAvatar DBID: [%ld] , Avatar Name: [%s]\n", (long)avatarDBID, name.c_str());
					PrintMessage(str, true);
				}
			}
			else
			{
				KBEngine::ENTITY_ID eid = root[Json::Value::UInt(0)].asInt();
				ENTITIES::iterator iter = mEntities.find(eid);
				KBEntity* pEntity = NULL;

				if (iter != mEntities.end())
					pEntity = iter->second.get();

				if (peventdata->name == "set_name")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						PrintMessage(str);
						break;
					}

					std::string name = root[1].asString();
					pEntity->setName(name);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, name: %s\n", (int)eid, peventdata->name.c_str(), name.c_str());
					PrintMessage(str);
				}
				else if (peventdata->name == "set_modelScale")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						PrintMessage(str);
						break;
					}

					uint32_t scale = root[1].asUInt();
					pEntity->setModelScale(scale / (float)100.0);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, modelScale: %d\n", (int)eid, peventdata->name.c_str(), (int)scale);
					PrintMessage(str);
				}
				else if (peventdata->name == "set_modelID")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						PrintMessage(str);
						break;
					}

					uint32_t modelID = root[1].asUInt();
					pEntity->setModelID(modelID);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, modelID: %d\n", (int)eid, peventdata->name.c_str(), (int)modelID);
					PrintMessage(str);
				}
				else if (peventdata->name == "set_state")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						PrintMessage(str);
						break;
					}

					int32_t state = root[1].asInt();
					pEntity->setState(state);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, state: %d\n", (int)eid, peventdata->name.c_str(), (int)state);
					PrintMessage(str);
				}
				else if (peventdata->name == "set_HP_Max")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						PrintMessage(str);
						break;
					}

					int32_t v = root[1].asInt();
					pEntity->setHPMax(v);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, HPMax: %d\n", (int)eid, peventdata->name.c_str(), (int)v);
					PrintMessage(str);
				}
				else if (peventdata->name == "set_MP_Max")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						PrintMessage(str);
						break;
					}

					int32_t v = root[1].asInt();
					pEntity->setMPMax(v);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, MPMax: %d\n", (int)eid, peventdata->name.c_str(), (int)v);
					PrintMessage(str);
				}
				else if (peventdata->name == "recvDamage")
				{
					KBEngine::ENTITY_ID attackerID = root[1].asInt();
					uint32_t skillID = root[2].asUInt();
					uint32_t damageType = root[3].asUInt();
					uint32_t damage = root[4].asUInt();

					ENTITIES::iterator iter = mEntities.find(attackerID);

					KBEntity* attacker = NULL;
					KBEntity* receiver = pEntity;
					KBEngine::ENTITY_ID eidAttacker = 0;

					if (iter != mEntities.end())
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
					PrintMessage(str);
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

			ENTITIES::iterator iter = mEntities.find(eid);
			if (iter == mEntities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d (NOT IN LIST), X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
				PrintMessage(str);
				break;
			}

			iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d, X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			PrintMessage(str);
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

			ENTITIES::iterator iter = mEntities.find(eid);
			if (iter == mEntities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d (NOT IN LIST), Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
				PrintMessage(str);
				break;
			}

			iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
			
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			PrintMessage(str);
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

			ENTITIES::iterator iter = mEntities.find(eid);
			if (iter == mEntities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d (NOT IN LIST), MoveSpeed %f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
				PrintMessage(str);
				break;
			}

			iter->second->setMoveSpeed(pEventData->speed);
			
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d, MoveSpeed %f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_SERVER_CLOSED:
		{
			mServerClosed = true;
			mShutDown = true;
			mLogin = LOGIN_NOT_DONE;
			mDisplayActions = 0;
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_SERVER_CLOSED\n");
			PrintMessage(str);
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

			ENTITIES::iterator iter = mEntities.find(eid);
			if (iter == mEntities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d (NOT IN LIST), X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
				PrintMessage(str);
				break;
			}

			iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);
			iter->second->setDestPosition(pEventData->x, pEventData->y, pEventData->z);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d, X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			PrintMessage(str);
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

			ENTITIES::iterator iter = mEntities.find(eid);
			if (iter == mEntities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d (NOT IN LIST), Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
				PrintMessage(str);
				break;
			}

			iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
			iter->second->setDestDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_ADDSPACEGEOMAPPING:
			{
				const KBEngine::EventData_AddSpaceGEOMapping* pEventData = static_cast<const KBEngine::EventData_AddSpaceGEOMapping*>(lpEventData);
				KBEngine::SPACE_ID spaceID = pEventData->spaceID;
				mSpaceWorldPtr->addSpace(spaceID, pEventData->respath);
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_ADDSPACEGEOMAPPING, SpaceID %d, ResPath %s\n", (int)spaceID, pEventData->respath.c_str());
				PrintMessage(str, true);
			}
			break;

		case CLIENT_EVENT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_VersionNotMatch* info = static_cast<const KBEngine::EventData_VersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_VERSION_NOT_MATCH - VerInfo=%s not match(server:%s)\n", info->verInfo.c_str(), info->serVerInfo.c_str());
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_ON_KICKED:
		{
			const KBEngine::EventData_onKicked* info = static_cast<const KBEngine::EventData_onKicked*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ON_KICKED (code=%u)!\n", info->failedcode);
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_LAST_ACCOUNT_INFO:
		{
			const KBEngine::EventData_LastAccountInfo* info = static_cast<const KBEngine::EventData_LastAccountInfo*>(lpEventData);
			g_accountName = info->name;

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LAST_ACCOUNT_INFO - Last account name: %s\n", info->name.c_str());
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_ScriptVersionNotMatch* info = static_cast<const KBEngine::EventData_ScriptVersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH - ScriptVerInfo=%s not match(server:%s)\n", info->verInfo.c_str(), info->serVerInfo.c_str());
			PrintMessage(str);
		}
		break;

		case CLIENT_EVENT_UNKNOWN:
		default:
		{
			//kbe_fireEvent("reset", NULL);
			//kbe_fireEvent("relive", NULL);
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_UNKNOWN\n");
			PrintMessage(str);
		}
		break;
	};
}

void TheWorld_UIClientApp::kbengine_onEvent(const KBEngine::EventData* lpEventData)
{
	KBEngine::EventData* peventdata = KBEngine::copyKBEngineEvent(lpEventData);

	if (peventdata)
	{
		boost::mutex::scoped_lock lock(mKbeEventsMutex);
		events_.push(std::tr1::shared_ptr<const KBEngine::EventData>(peventdata));
		mHasEvent = true;
	}
}

bool TheWorld_UIClientApp::kbengine_Login(const char* accountname, const char* passwd, const char* datas, KBEngine::uint32 datasize,	const char* ip, KBEngine::uint32 port)
{
	std::string accountName;
	if (accountname == NULL)
	{
		g_accountName = kbe_getLastAccountName();
		if (g_accountName.size() == 0)
		{
			/*KBEngine::uint64 uuid = kbe_genUUID64();
			std::stringstream ss;
			ss << uuid;
			g_accountName = ss.str();
			//g_accountName = KBEngine::StringConv::val2str(KBEngine::genUUID64());*/
			g_accountName = g_defaultAccountName;
		}
		accountName = g_accountName;
	}
	else
		accountName = accountname;

	char str[256];
	sprintf(str, "KBE Login - AccountName: [%s], Password: [%s], Client Name: [%s]\n", accountName.c_str(), passwd, datas);
	PrintMessage(str);
	if (kbe_login(accountName.c_str(), passwd, datas, datasize, ip, port))
	{
		return true;
	}
	else
		return false;
}

bool TheWorld_UIClientApp::kbengine_Logout()
{
	PrintMessage("KBE Logout\n");
	kbengine_Reset();
	mAvatars.clear();

	return true;
}
	
bool TheWorld_UIClientApp::kbengine_CreateAvatar(std::string avatarName)
{
	char str[256];
	sprintf(str, "KBE Create Avatar - Name: [%s]\n", avatarName);
	PrintMessage(str);

	kbe_lock();
	sprintf(str, "[1, \"kbeconsole_%s\"]", avatarName.c_str());
	kbe_callEntityMethod(kbe_playerID(), "reqCreateAvatar", str);
	kbe_unlock();

	return true;
}

bool TheWorld_UIClientApp::kbengine_RemoveAvatar(std::string avatarName)
{
	char str[256];
	sprintf(str, "KBE Remove Avatar - Name: [%s]\n", avatarName);
	PrintMessage(str);

	kbe_lock();
	kbe_callEntityMethod(kbe_playerID(), "reqRemoveAvatar", avatarName.c_str());
	kbe_unlock();

	return true;
}

bool TheWorld_UIClientApp::kbengine_SelectAvatarGame(KBEngine::DBID avatarDBID)
{
	char str[256];
	sprintf(str, "KBE Select Avatar Game - DBID: [%d]\n", (int)avatarDBID);
	PrintMessage(str);

	kbe_lock();
	kbe_callEntityMethod(kbe_playerID(), "selectAvatarGame", KBEngine::StringConv::val2str(avatarDBID).c_str());
	kbe_unlock();

	return true;
}

void TheWorld_UIClientApp::kbengine_Reset(void)
{
	kbe_reset();
}

void TheWorld_UIClientApp::PrintMessage(char *message, bool interline, bool console, KBEMessageType t)
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

int main()
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
	
	TheWorld_UIClientApp *app = new TheWorld_UIClientApp();

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

