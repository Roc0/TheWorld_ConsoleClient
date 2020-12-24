#pragma once

#pragma warning( disable : 4251 )

#include <string>
#include <queue>

#include "TheWorld_ClientApp.h"
#include "TheWorld_ClientDll.h"
#include "TheWorld_ConsoleClient_GL.h"
#include "client_lib/event.h"
#include "SpaceWorld.h"
#include "Entity.h"
#include "Avatar.h"

#include <boost/thread/thread.hpp>

bool caseInSensStringEqual(std::string& str1, std::string& str2);

typedef std::map<KBEngine::ENTITY_ID, std::tr1::shared_ptr<KBEntity> > ENTITIES;
typedef std::map<KBEngine::DBID, std::tr1::shared_ptr<KBAvatar> > AVATARS;

//class TheWorld_ClientAppConsole : public KBEngine::EventHandle
class TheWorld_ClientAppConsole : public TheWorld_ClientApp
{
public:
	TheWorld_ClientAppConsole(void);
	virtual ~TheWorld_ClientAppConsole(void);
	virtual void go(void);
	virtual SpaceWorld* getSpaceWorld(void) { return m_pSpaceWorld; };
	virtual ENTITIES& getEntities(void) { return m_Entities; };
	virtual KBEntity* getPlayerEntity(void) { return m_pPlayerEntity; }
	virtual KBEntity* getTargetEntity(void) { return m_pTargetEntity; }
	virtual KBEntity* getEntity(KBEngine::ENTITY_ID eid)
	{
		ENTITIES::iterator iter = m_Entities.find(eid);
		if (iter == m_Entities.end())
			return NULL;
		else
			return iter->second.get();
	}
	virtual void setTargetEntity(KBEngine::ENTITY_ID eid) { m_pTargetEntity = getEntity(eid); }
	virtual void setTargetEntity(KBEntity* pTarget) { m_pTargetEntity = pTarget; }

private:
	virtual bool init(void);
	virtual void cleanup(void);
	virtual void doInitialMenuAction(int ch);
	virtual void doMainLoop(void);
	virtual void manageInitialMenu(void);
	virtual void manageGraphicRendering(void);
	virtual void client_onEvent(const KBEngine::EventData* lpEventData);

	// config
	virtual void setAppMode(enum TheWorld_ClientApp_GL::_AppMode r, bool bForce = false);
	virtual enum TheWorld_ClientApp_GL::_AppMode getAppMode(void);
	virtual bool getInitAppModeRequired(void);
	virtual void setInitAppModeRequired(bool);
	// config
	virtual void setShutdownRequired(bool b) { m_bShutDown = b; };
	virtual bool getShutdownRequired(void) { return m_bShutDown; };

private:
	bool m_bShutDown;
	bool m_bServerClosed;
	int m_iSelectAvatarPending;
#define SELECT_AVATAR_NOT_PENDING	0
#define SELECT_AVATAR_PENDING		1
#define SELECT_AVATAR_DONE			2
	SpaceWorld* m_pSpaceWorld;

private:
	TheWorld_ClientApp_GL* m_pGLClientApp;

	ENTITIES m_Entities;
	KBEntity* m_pPlayerEntity;
	KBEntity* m_pTargetEntity;
	KBEntity* m_pMouseTarget;
	AVATARS m_Avatars;
};