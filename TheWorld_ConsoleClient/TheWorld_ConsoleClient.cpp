// TheWorld_ConsoleClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TheWorld_ConsoleClient.h"
#include <conio.h>
#include <conio.h>
#include <json/json.h>
#include <boost/thread/thread.hpp>

//AVATAR
//KBEngine::DBID g_selAvatarDBID = 0;
//std::string g_avatar;
std::vector<KBEngine::DBID> g_avatarsDBID;
std::vector<std::string> g_avatars;
//------

//Account
std::string g_defaultAccountName("6529868038458114048");
//------

int g_displayActions = 0;
int g_loginDone = false;
bool MyClientApp::g_hasEvent = false;
std::string MyClientApp::g_accountName;

boost::mutex g_kbeEventsMutex;

MyClientApp::MyClientApp() :
	events_()
{
	mShutDown = false;
	kbe_registerEventHandle(this);
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

		if (!g_loginDone)
		{
			std::string datas = "TheWorld_ConsoleClient";
			if (!kbengine_Login(NULL, "123456", datas.data(), KBEngine::uint32(datas.size())))
			{
				MessageBox(NULL, "Login error!", "error!", MB_OK);
			}
			g_loginDone = true;
		}

		if (g_displayActions == 1)
		{
			g_displayActions = 2;
			printf("Actions: \n");
			printf("   1 - Create Avatar\n");
			printf("\n");
			for (KBEngine::uint32 i = 0; i < g_avatars.size(); i++)
			{
				printf("   %d - Select Avatar %d - %s\n", i + 2, (int)g_avatarsDBID[i], g_avatars[i].c_str());
			}
			printf("\n");
			printf("   . - Quit\n");
		}

		if (kbhit())
		{
			int ch = getch();
			if (ch == '.') {
				mShutDown = true;
				g_hasEvent = true;
				kbe_shutdown();
			}
			else
				doAction(ch);
		}
		
		if (!g_hasEvent)
			Sleep(1);
	}
}

void MyClientApp::doAction(int ch)
{
	char str[256];

	switch (ch)
	{
		// '1'
		case '1':
		{
			kbe_lock();
			kbe_callEntityMethod(kbe_playerID(), "reqCreateAvatar", "[1, \"kbengine\"]");
			kbe_unlock();
			g_displayActions = 1;
			strcpy(str, "kbe_callEntityMethod: reqCreateAvatar");
			printf(str);	printf("\n");
			break;
		}
		default:
		{
			int i = ch - 48 - 2;
			if (i >= 0 && i < g_avatarsDBID.size())
			{
				kbe_lock();
				kbe_callEntityMethod(kbe_playerID(), "selectAvatarGame", KBEngine::StringConv::val2str(g_avatarsDBID[i]).c_str());
				kbe_unlock();
				sprintf(str, "kbe_callEntityMethod: selectAvatarGame, Avatar DBID %ld\n", (long)g_avatarsDBID[i]);
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
		g_kbeEventsMutex.lock();

		if (events_.empty())
		{
			g_kbeEventsMutex.unlock();
			break;
		}

		std::tr1::shared_ptr<const KBEngine::EventData> pEventData = events_.front();
		events_.pop();
		g_kbeEventsMutex.unlock();

		KBEngine::EventID id = pEventData->id;

		if (id == CLIENT_EVENT_SERVER_CLOSED)
		{
			//OgreApplication::getSingleton().changeSpace(new SpaceAvatarSelect(mRoot, mWindow, mInputManager, mTrayMgr));
			//break;
		}

		if (id == CLIENT_EVENT_SCRIPT)
		{
			kbe_lock();
		}

		onEvent(pEventData.get());

		if (id == CLIENT_EVENT_SCRIPT)
		{
			kbe_unlock();
		}

		g_hasEvent = false;
	}
}


void MyClientApp::onEvent(const KBEngine::EventData* lpEventData)
{
	char str[256];

	switch (lpEventData->id)
	{
		case CLIENT_EVENT_ENTERWORLD:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_ENTERWORLD");
			printf(str);	printf("\n");
			break;
	
		case CLIENT_EVENT_LEAVEWORLD:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LEAVEWORLD");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_ENTERSPACE:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_ENTERSPACE");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_LEAVESPACE:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LEAVESPACE");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_CREATEDENTITY:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_CREATEDENTITY");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_LOGIN_SUCCESS:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_SUCCESS");
			printf(str);	printf("\n");
			break;
		
		case CLIENT_EVENT_LOGIN_FAILED:
		{
			g_loginDone = false;
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
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS");
			printf(str);	printf("\n");
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
				if (g_displayActions == 0)
					g_displayActions = 1;

				g_avatars.clear();
				g_avatarsDBID.clear();
				
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
						g_avatarsDBID.push_back(avatarDBID);
						g_avatars.push_back(name);

						sprintf(str, "Avatar DBID: %ld , %s\n", (long)avatarDBID, name.c_str());
						printf(str);	printf("\n");
					}
				}
			}
		}
		break;

		case CLIENT_EVENT_POSITION_CHANGED:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_DIRECTION_CHANGED:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_MOVESPEED_CHANGED:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_SERVER_CLOSED:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_SERVER_CLOSED");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_POSITION_FORCE:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_DIRECTION_FORCE:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_ADDSPACEGEOMAPPING:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_ADDSPACEGEOMAPPING");
			printf(str);	printf("\n");
			break;

		case CLIENT_EVENT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_VersionNotMatch* info = static_cast<const KBEngine::EventData_VersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_VERSION_NOT_MATCH - VerInfo=%s not match(server:%s)", info->verInfo.c_str(), info->serVerInfo.c_str());
			printf(str);	printf("\n");
		}
		break;

		case CLIENT_EVENT_ON_KICKED:
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_ON_KICKED");
			printf(str);	printf("\n");
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
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_UNKNOWN");
			printf(str);	printf("\n");
			break;
	};
}

void MyClientApp::kbengine_onEvent(const KBEngine::EventData* lpEventData)
{
	KBEngine::EventData* peventdata = KBEngine::copyKBEngineEvent(lpEventData);

	if (peventdata)
	{
		boost::mutex::scoped_lock lock(g_kbeEventsMutex);
		events_.push(std::tr1::shared_ptr<const KBEngine::EventData>(peventdata));
		g_hasEvent = true;
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

