#pragma once

#pragma warning( disable : 4251 )

#include <string>
#include <queue>

#include "TheWorld_ClientApp.h"
#include "TheWorld_ClientDll.h"
#include "TheWorld_ConsoleClient_GL.h"
#include "client_lib/event.h"

#include <boost/thread/thread.hpp>

bool caseInSensStringEqual(std::string& str1, std::string& str2);

//class TheWorld_ClientAppConsole : public KBEngine::EventHandle
class TheWorld_ClientAppConsole : public TheWorld_ClientApp
{
public:
	TheWorld_ClientAppConsole(void);
	virtual ~TheWorld_ClientAppConsole(void);
	virtual void go(void);

private:
	virtual bool init(void);
	virtual void cleanup(void);
	virtual void doInitialMenuAction(int ch);
	virtual void doMainLoop(void);
	virtual void manageInitialMenu(void);
	virtual void manageGraphicRendering(void);
	
	virtual void onLoginSuccess(void) {};
	virtual void onLoginFailed(int failCode) {};
	virtual void onServerClosed(void) {};
	virtual void onKicked(int failCode) {};
	virtual void onClearEntities(void) {};
	virtual void onCreatedEntity(KBEngine::ENTITY_ID eid, bool bPlayer) {};
	virtual void onEraseEntity(KBEngine::ENTITY_ID eid) {};
	virtual void onClearAvatars(void) {};
	virtual void onEraseAvatar(KBEngine::DBID dbid) {};
	virtual void onUpdateAvatars(void) {};
	virtual void onEntityEnterWorld(KBEngine::ENTITY_ID eid) {};
	virtual void onEntityLeaveWorld(KBEngine::ENTITY_ID eid) {};
	virtual void onEntityEnterSpace(KBEngine::ENTITY_ID eid, KBEngine::SPACE_ID spaceId) {};
	virtual void onEntityLeaveSpace(KBEngine::ENTITY_ID eid, KBEngine::SPACE_ID spaceId) {};
	virtual void onPlayerEnterSpace(KBEngine::ENTITY_ID eid, KBEngine::SPACE_ID spaceId) {};
	virtual void onPlayerLeaveSpace(KBEngine::ENTITY_ID eid, KBEngine::SPACE_ID spaceId) {};
	virtual void onAddSpaceGeoMapping(KBEngine::SPACE_ID, const char* resPath) {};
	virtual void onMaxHPChanged(KBEngine::ENTITY_ID eid, int MaxHP) {};
	virtual void onMaxMPChanged(KBEngine::ENTITY_ID eid, int MaxMP) {};
	virtual void onHPChanged(KBEngine::ENTITY_ID eid, int HP) {};
	virtual void onMPChanged(KBEngine::ENTITY_ID eid, int MP) {};
	virtual void onRecvDamage(KBEngine::ENTITY_ID eid, KBEngine::ENTITY_ID attacker, int skillID, int damageType, int damage) {};
	virtual void onAttackDamage(KBEngine::ENTITY_ID eid, KBEngine::ENTITY_ID receiver, int skillID, int damageType, int damage) {};
	virtual bool isDebugEnabled(void) { return true; };

private:
	//bool m_bServerClosed;
	int m_iSelectAvatarPending;
#define SELECT_AVATAR_NOT_PENDING	0
#define SELECT_AVATAR_PENDING		1
#define SELECT_AVATAR_DONE			2

	TheWorld_ClientApp_GL* m_pGLClientApp;
};