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

class MyClientApp : public KBEngine::EventHandle
{
public:
	MyClientApp(void);
	virtual ~MyClientApp(void);
	virtual void go(void);
	virtual bool kbengine_Login(const char* accountname, const char* passwd, const char* datas = NULL, KBEngine::uint32 datasize = 0, const char* ip = NULL, KBEngine::uint32 port = 0);

protected:
	virtual bool setup();
	void messagePump(void);
	virtual void kbengine_onEvent(const KBEngine::EventData* lpEventData);
	virtual void onEvent(const KBEngine::EventData* lpEventData);
	virtual void doAction(int ch);

private:

protected:
	bool mShutDown;

private:
	boost::mutex mKbeEventsMutex;
	int mDisplayActions;
	bool mLoginDone;
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