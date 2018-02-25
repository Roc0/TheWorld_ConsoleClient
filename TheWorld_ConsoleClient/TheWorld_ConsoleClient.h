#pragma once

#include <windows.h>
#include <string>
#include <queue>

#include "TheWorld_ClientDll.h"
#include "client_lib/event.h"

HINSTANCE g_hKBEngineDll = NULL;

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
	std::queue< std::tr1::shared_ptr<const KBEngine::EventData> > events_;
	static bool g_hasEvent;
	static std::string g_accountName;
};