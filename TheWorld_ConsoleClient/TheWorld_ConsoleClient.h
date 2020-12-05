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
#include "InputGeom.h"
#include "PerfTimer.h"
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

HINSTANCE g_hKBEngineDll = NULL;
typedef std::map<KBEngine::ENTITY_ID, std::tr1::shared_ptr<KBEntity> > ENTITIES;
typedef std::map<KBEngine::DBID, std::tr1::shared_ptr<KBAvatar> > AVATARS;

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 1;

struct NavMeshSetHeader
{
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams params;
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int dataSize;
};

/// Recast build context.
class BuildContext : public rcContext
{
	TimeVal m_startTime[RC_MAX_TIMERS];
	TimeVal m_accTime[RC_MAX_TIMERS];

	//static const int MAX_MESSAGES = 1000;
	//const char* m_messages[MAX_MESSAGES];
	//int m_messageCount;
	//static const int TEXT_POOL_SIZE = 8000;
	//char m_textPool[TEXT_POOL_SIZE];
	//int m_textPoolSize;

public:
	BuildContext();

	/// Dumps the log to stdout.
	//void dumpLog(const char* format, ...);
	/// Returns number of log messages.
	//int getLogCount() const;
	/// Returns log message text.
	//const char* getLogText(const int i) const;

protected:
	/// Virtual functions for custom implementations.
	///@{
	//virtual void doResetLog();
	//virtual void doLog(const rcLogCategory category, const char* msg, const int len);
	virtual void doResetTimers();
	virtual void doStartTimer(const rcTimerLabel label);
	virtual void doStopTimer(const rcTimerLabel label);
	virtual int doGetAccumulatedTime(const rcTimerLabel label) const;
	///@}
};

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
	virtual bool init(void);
	virtual void cleanup(void);
	void messagePump(void);
	virtual void onEvent(const KBEngine::EventData* lpEventData);
	virtual void doInitialMenuAction(int ch);
	virtual void printMessage(char *message, bool interline = false, bool console = true, KBEMessageType t = Print);
	virtual void doMainLoop(void);
	virtual void manageInitialMenu(void);
	virtual void manageGraphicRendering(void);
	virtual bool initGraphicRendering(void);
	virtual void cleanupGraphicRendering(void);
	enum _RenderingMode
	{
		InitialMenu,
		GraphicMode
	};
	virtual void setRenderingMode(enum _RenderingMode r, bool bForce = false);
	virtual _RenderingMode getRenderingMode(void);
	virtual dtNavMesh* loadAll(const char* path);
	virtual void saveAll(const char* path, const dtNavMesh* mesh);

private:
	void imguiDemoWindow(ImVec4& clear_color);
	bool imguiRender(bool& bLogoutRequired);
	std::string m_message;

protected:
	bool m_bShutDown;

protected:
	boost::mutex m_KbeEventsMutex;
	//int m_iDisplayActions;
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

protected:
	float m_fFieldOfViewAngleY;
	float m_fZNear;
	float m_fZFar;
	float m_fCamr;
	float m_cameraPos[3];
	float m_cameraEulers[2];
	float m_bFullScreen;
	SDL_Window* m_window;
	SDL_GLContext m_gl_context;
	Uint32 m_uiFlags;
	int m_iWidth;
	int m_iHeight;
	ImGuiIO *m_pIo;
	bool m_bShow_demo_window;
	bool m_bShow_another_window;
	bool m_bShow_helloworld_window;
	bool m_bInitRenderingMode;
	enum _RenderingMode renderingMode;
	InputGeom* m_geom;
	BuildContext m_buildCtx;
	dtNavMesh* m_navMesh;
	dtNavMeshQuery* m_navQuery;
	const std::string m_meshPath = "..\\TheWorld_ConsoleClient\\Meshes\\";

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