#pragma once

#include <windows.h>
#include <string>
#include <queue>

#include "TheWorld_ClientDll.h"
#include "TheWorld_ConsoleClient_GL.h"
#include "client_lib/event.h"
#include "SpaceWorld.h"
#include "Entity.h"
#include "Avatar.h"

#include <boost/thread/thread.hpp>

typedef std::map<KBEngine::ENTITY_ID, std::tr1::shared_ptr<KBEntity> > ENTITIES;
typedef std::map<KBEngine::DBID, std::tr1::shared_ptr<KBAvatar> > AVATARS;

class TheWorld_ClientApp : public KBEngine::EventHandle
{
public:
	TheWorld_ClientApp(void);
	virtual ~TheWorld_ClientApp(void);
	virtual void go(void);
	virtual bool kbengine_Login(const char* accountname, const char* passwd, const char* datas = NULL, KBEngine::uint32 datasize = 0, const char* ip = NULL, KBEngine::uint32 port = 0);
	virtual bool kbengine_Logout(void);
	virtual void kbengine_Reset(void);
	virtual bool kbengine_CreateAvatar(std::string avatarName);
	virtual bool kbengine_RemoveAvatar(std::string avatarName);
	virtual bool kbengine_SelectAvatarGame(KBEngine::DBID avatarDBID);
	virtual SpaceWorld* getSpaceWorld(void) { return m_pSpaceWorld; };
	virtual ENTITIES& getEntities(void) { return m_Entities; };

private:
	virtual void kbengine_onEvent(const KBEngine::EventData* lpEventData);

private:
	virtual bool init(void);
	virtual void cleanup(void);
	void messagePump(void);
	virtual void onEvent(const KBEngine::EventData* lpEventData);
	virtual void doInitialMenuAction(int ch);
	virtual void printMessage(char *message, bool interline = false, bool console = true, KBEMessageType t = Print);
	virtual void doMainLoop(void);
	virtual void manageInitialMenu(void);
	virtual void manageGraphicRendering(void);
	
	// config
	virtual void setAppMode(enum TheWorld_ClientApp_GL::_AppMode r, bool bForce = false);
	virtual enum TheWorld_ClientApp_GL::_AppMode getAppMode(void);
	virtual bool getInitAppModeRequired(void);
	virtual void setInitAppModeRequired(bool);
	// config

private:
	bool m_bShutDown;

private:
	boost::mutex m_KbeEventsMutex;
	int m_iSelectAvatarPending;
#define SELECT_AVATAR_NOT_PENDING	0
#define SELECT_AVATAR_PENDING		1
#define SELECT_AVATAR_DONE			2
	int m_iLogin;
#define LOGIN_NOT_DONE				0
#define LOGIN_STARTED				1
#define LOGIN_DONE					2
	std::queue< std::tr1::shared_ptr<const KBEngine::EventData> > events_;
	bool m_bDoSleepInMainLoop;
	std::string m_sAccountName;
	SpaceWorld* m_pSpaceWorld;
	bool m_bServerClosed;

private:
	TheWorld_ClientApp_GL* m_pGLClientApp;

	//AVATARS
	ENTITIES m_Entities;
	KBEntity* m_pPlayer;
	KBEntity* m_pTarget;
	KBEntity* m_pMouseTarget;
	//------

	//AVATARS
	AVATARS m_Avatars;
	//------
};