// TheWorld_ConsoleClient.cpp : Defines the entry point for the console application.
//

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

MyClientApp::MyClientApp() :
	events_()
{
	mHasEvent = false;
	mDisplayActions = 0;
	mLoginDone = false;
	mShutDown = false;
	kbe_registerEventHandle(this);
	mSpaceWorldPtr = new SpaceWorld;
	mPlayerPtr = mTargetPtr = mMouseTargetPtr = NULL;
	mServerClosed = false;
}

MyClientApp::~MyClientApp()
{
	kbe_deregisterEventHandle(this);
}

void MyClientApp::go(void)
{
	if (!setup())
		return;

	while (!mShutDown)
	{
		messagePump();

		if (!mLoginDone)
		{
			std::string datas = "TheWorld_ConsoleClient";
			if (!kbengine_Login(NULL, "123456", datas.data(), KBEngine::uint32(datas.size())))
			{
				//MessageBox(NULL, "Login error!", "error!", MB_OK);
				printf("Login error!\n");
			}
			else
				mLoginDone = true;
		}

		if (mDisplayActions == 1)
		{
			mDisplayActions = 2;
			printf("Actions: \n");
			printf("   c - Create Avatar\n");
			printf("   d - Dump Status\n");
			printf("\n");
			for (KBEngine::uint32 i = 0; i < mAvatars.size(); i++)
			{
				printf("   %d - Select Avatar %d - %s\n", i + 1, (int)mAvatarsDBID[i], mAvatars[i].c_str());
			}
			printf("\n");
			printf("   . - Quit\n");
		}

		if (kbhit())
		{
			int ch = getch();
			if (ch == '.') {
				mShutDown = true;
				mHasEvent = true;
				kbe_shutdown();
			}
			else
				doAction(ch);
		}
		
		if (!mHasEvent)
			Sleep(1);
	}
}

void MyClientApp::doAction(int ch)
{
	char str[256];

	switch (ch)
	{
		case 'c':
		{
			kbe_lock();
			kbe_callEntityMethod(kbe_playerID(), "reqCreateAvatar", "[1, \"kbengine\"]");
			kbe_unlock();
			mDisplayActions = 1;
			sprintf(str, "kbe_callEntityMethod: reqCreateAvatar - Player ID: %d", (int)kbe_playerID());
			printf(str);	printf("\n");
			break;
		}
		case 'd':
		{
			printf("\n");
			ENTITIES::iterator iter = mEntities.begin();
			int idx = 0;
			for (; iter != mEntities.end(); iter++)
			{
				idx++;
				KBEntity* pEntity = iter->second.get();
				pEntity->dumpStatus(idx);
			}
			printf("\n");
			break;
		}
		// '1' / '9'
		default:
		{
			int i = ch - 48 - 1;
			if (i >= 0 && i < mAvatarsDBID.size())
			{
				kbe_lock();
				kbe_callEntityMethod(kbe_playerID(), "selectAvatarGame", KBEngine::StringConv::val2str(mAvatarsDBID[i]).c_str());
				kbe_unlock();
				sprintf(str, "kbe_callEntityMethod: selectAvatarGame - Player ID: %d, Avatar DBID %ld\n", (int)kbe_playerID(), (long)mAvatarsDBID[i]);
				printf(str);	printf("\n");
			}
			break;
		}
	}

	return;
};

bool MyClientApp::setup(void)
{
	return true;
};

void MyClientApp::messagePump(void)
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


void MyClientApp::onEvent(const KBEngine::EventData* lpEventData)
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
				printf(str);	printf("\n");
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
			printf(str);	printf("\n");
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

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVEWORLD - %s - EntityID %d", bPlayer ? "PLAYER" : "OTHER", (int)eid);
			printf(str);	printf("\n");
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
				printf(str);	printf("\n");
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
			printf(str);	printf("\n");
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
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVESPACE - %s - EntityID %d", bPlayer ? "PLAYER" : "OTHER", (int)eid);
			printf(str);	printf("\n");
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
				
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_CREATEDENTITY - %s - EntityID: %d, ModelScale: %f", bPlayer ? "PLAYER" : "OTHER", (int)pEventData_createEntity->entityID, (float)pEventData_createEntity->modelScale);
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_LOGIN_SUCCESS:
		{
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_SUCCESS");
			printf(str);	printf("\n");
		}
		break;
		
		case CLIENT_EVENT_LOGIN_FAILED:
		{
			mLoginDone = false;
			const KBEngine::EventData_LoginFailed* info = static_cast<const KBEngine::EventData_LoginFailed*>(lpEventData);

			if (info->failedcode == 20)
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Server is starting, please wait!");
			}
			else
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Login is failed (code=%u)!", info->failedcode);
			}

			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS:
		{
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS");
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_LOGIN_BASEAPP_FAILED:
		{
			const KBEngine::EventData_LoginBaseappFailed* info = static_cast<const KBEngine::EventData_LoginBaseappFailed*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_FAILED (code=%u)!", info->failedcode);
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_SCRIPT:
		{
			const KBEngine::EventData_Script* peventdata = static_cast<const KBEngine::EventData_Script*>(lpEventData);
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, peventdata->name: ");
			strcat(str, peventdata->name.c_str());
			printf(str);	printf("\n");
			if (peventdata->name == "update_avatars")
			{
				if (mDisplayActions == 0)
					mDisplayActions = 1;

				mAvatars.clear();
				mAvatarsDBID.clear();
				
				Json::Reader reader;
				Json::Value root;

				if (reader.parse(peventdata->datas.c_str(), root))
				{
					Json::Value::Members mem = root.getMemberNames();
					for (auto iter = mem.begin(); iter != mem.end(); iter++)
					{
						Json::Value& val = root[*iter];
						std::string name = val[1].asString();
						KBEngine::DBID avatarDBID = val[Json::Value::UInt(0)].asUInt();
						mAvatarsDBID.push_back(avatarDBID);
						mAvatars.push_back(name);

						sprintf(str, "Avatar DBID: %ld , %s\n", (long)avatarDBID, name.c_str());
						printf(str);	printf("\n");
					}
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
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d (NOT IN LIST), X/Y/Z: %f/%f/%f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
				printf(str);	printf("\n");
				break;
			}

			iter->second->setDestPosition(pEventData->x, pEventData->y, pEventData->z);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d, X/Y/Z: %f/%f/%f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			printf(str);	printf("\n");
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
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d (NOT IN LIST), Yaw/Pitch/Roll: %f/%f/%f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
				break;
			}

			iter->second->setDestDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
			
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			printf(str);	printf("\n");
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
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d (NOT IN LIST), MoveSpeed %f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
				printf(str);	printf("\n");
				break;
			}

			iter->second->setMoveSpeed(pEventData->speed);
			
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d, MoveSpeed %f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_SERVER_CLOSED:
		{
			mServerClosed = true;
			mShutDown = true;
			mLoginDone = false;
			mDisplayActions = 0;
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_SERVER_CLOSED");
			printf(str);	printf("\n");
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
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d (NOT IN LIST), X/Y/Z: %f/%f/%f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
				break;
			}

			iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);
			iter->second->setDestPosition(pEventData->x, pEventData->y, pEventData->z);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d, X/Y/Z: %f/%f/%f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			printf(str);	printf("\n");
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
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d (NOT IN LIST), Yaw/Pitch/Roll: %f/%f/%f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
				printf(str);	printf("\n");
				break;
			}

			iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
			iter->second->setDestDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_ADDSPACEGEOMAPPING:
			{
				const KBEngine::EventData_AddSpaceGEOMapping* pEventData = static_cast<const KBEngine::EventData_AddSpaceGEOMapping*>(lpEventData);
				KBEngine::SPACE_ID spaceID = pEventData->spaceID;
				mSpaceWorldPtr->addSpace(spaceID, pEventData->respath);
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_ADDSPACEGEOMAPPING, SpaceID %d, ResPath %s\n", (int)spaceID, pEventData->respath.c_str());
				printf(str);	printf("\n");
			}
			break;

		case CLIENT_EVENT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_VersionNotMatch* info = static_cast<const KBEngine::EventData_VersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_VERSION_NOT_MATCH - VerInfo=%s not match(server:%s)", info->verInfo.c_str(), info->serVerInfo.c_str());
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_ON_KICKED:
		{
			const KBEngine::EventData_onKicked* info = static_cast<const KBEngine::EventData_onKicked*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ON_KICKED (code=%u)!", info->failedcode);
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_LAST_ACCOUNT_INFO:
		{
			const KBEngine::EventData_LastAccountInfo* info = static_cast<const KBEngine::EventData_LastAccountInfo*>(lpEventData);
			g_accountName = info->name;

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LAST_ACCOUNT_INFO - Last account name: %s", info->name.c_str());
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_ScriptVersionNotMatch* info = static_cast<const KBEngine::EventData_ScriptVersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH - ScriptVerInfo=%s not match(server:%s)", info->verInfo.c_str(), info->serVerInfo.c_str());
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_UNKNOWN:
		default:
		{
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_UNKNOWN");
			printf(str);	printf("\n");
		}
		break;
	};
}

void MyClientApp::kbengine_onEvent(const KBEngine::EventData* lpEventData)
{
	KBEngine::EventData* peventdata = KBEngine::copyKBEngineEvent(lpEventData);

	if (peventdata)
	{
		boost::mutex::scoped_lock lock(mKbeEventsMutex);
		events_.push(std::tr1::shared_ptr<const KBEngine::EventData>(peventdata));
		mHasEvent = true;
	}
}

bool MyClientApp::kbengine_Login(const char* accountname, const char* passwd, const char* datas, KBEngine::uint32 datasize,	const char* ip, KBEngine::uint32 port)
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

	if (!kbe_login(accountName.c_str(), passwd, datas, datasize, ip, port))
	{
		return false;
	}

	return true;
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
	
	MyClientApp *app = new MyClientApp();

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

