#pragma once

#include <windows.h>
#include <string>
#include <queue>

#include "TheWorld_ClientDll.h"
#include "client_lib/event.h"
#include "SpaceWorld.h"
#include "Entity.h"
#include "Avatar.h"
#include <boost/thread/thread.hpp>

HINSTANCE g_hKBEngineDll = NULL;
typedef std::map<KBEngine::ENTITY_ID, std::tr1::shared_ptr<KBEntity> > ENTITIES;
typedef std::map<KBEngine::DBID, std::tr1::shared_ptr<KBAvatar> > AVATARS;

class TheWorld_UIClientApp : public KBEngine::EventHandle
{
public:
	TheWorld_UIClientApp(void);
	virtual ~TheWorld_UIClientApp(void);
	virtual void go(void);
	virtual bool kbengine_Login(const char* accountname, const char* passwd, const char* datas = NULL, KBEngine::uint32 datasize = 0, const char* ip = NULL, KBEngine::uint32 port = 0);
	virtual bool kbengine_Logout(void);
	virtual void kbengine_Reset(void);
	virtual bool kbengine_CreateAvatar(std::string avatarName);
	virtual bool kbengine_RemoveAvatar(std::string avatarName);
	virtual bool kbengine_SelectAvatarGame(KBEngine::DBID avatarDBID);

protected:
	virtual void kbengine_onEvent(const KBEngine::EventData* lpEventData);

protected:
	virtual bool setup();
	void messagePump(void);
	virtual void onEvent(const KBEngine::EventData* lpEventData);
	virtual void doAction(int ch);
	virtual void PrintMessage(char *message, bool interline = false, bool console = true, KBEMessageType t = Print);

protected:
	bool mShutDown;

private:
	boost::mutex mKbeEventsMutex;
	int mDisplayActions;
	int mSelectAvatarPending;
#define SELECT_AVATAR_NOT_PENDING	0
#define SELECT_AVATAR_PENDING		1
#define SELECT_AVATAR_DONE			2
	int mLogin;
#define LOGIN_NOT_DONE				0
#define LOGIN_STARTED				1
#define LOGIN_DONE					2
	std::queue< std::tr1::shared_ptr<const KBEngine::EventData> > events_;
	bool mHasEvent;
	std::string g_accountName;
	SpaceWorld* mSpaceWorldPtr;
	bool mServerClosed;

	//AVATARS
	ENTITIES mEntities;
	KBEntity* mPlayerPtr;
	KBEntity* mTargetPtr;
	KBEntity* mMouseTargetPtr;
	//------

	//AVATARS
	AVATARS mAvatars;
	//------
};