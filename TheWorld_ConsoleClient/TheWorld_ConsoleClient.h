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
	virtual void onUpdateAvatars(void) {};

private:
	//bool m_bServerClosed;
	int m_iSelectAvatarPending;
#define SELECT_AVATAR_NOT_PENDING	0
#define SELECT_AVATAR_PENDING		1
#define SELECT_AVATAR_DONE			2

	TheWorld_ClientApp_GL* m_pGLClientApp;
};