#pragma once

#include <windows.h>
#include <string>
#include <queue>

//#include "TheWorld_ClientDll.h"
//#include "client_lib/event.h"
#include "SpaceWorld.h"
#include "Entity.h"
#include "Avatar.h"

//#include <boost/thread/thread.hpp>
#include "InputGeom.h"
#include "PerfTimer.h"
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include "DebugDraw.h"

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 1;

enum SamplePolyFlags
{
	SAMPLE_POLYFLAGS_WALK = 0x01,		// Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM = 0x02,		// Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR = 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP = 0x08,		// Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED = 0x10,		// Disabled polygon
	SAMPLE_POLYFLAGS_ALL = 0xffff	// All abilities.
};

enum SamplePartitionType
{
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS,
};

enum DrawMode
{
	DRAWMODE_NAVMESH,
	DRAWMODE_NAVMESH_TRANS,
	DRAWMODE_NAVMESH_BVTREE,
	DRAWMODE_NAVMESH_NODES,
	DRAWMODE_NAVMESH_INVIS,
	DRAWMODE_MESH,
	DRAWMODE_VOXELS,
	DRAWMODE_VOXELS_WALKABLE,
	DRAWMODE_COMPACT,
	DRAWMODE_COMPACT_DISTANCE,
	DRAWMODE_COMPACT_REGIONS,
	DRAWMODE_REGION_CONNECTIONS,
	DRAWMODE_RAW_CONTOURS,
	DRAWMODE_BOTH_CONTOURS,
	DRAWMODE_CONTOURS,
	DRAWMODE_POLYMESH,
	DRAWMODE_POLYMESH_DETAIL,
	MAX_DRAWMODE
};

/// These are just sample areas to use consistent values across the samples.
/// The use should specify these base on his needs.
enum PolyAreas
{
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP,
};

/// OpenGL debug draw implementation.
class DebugDrawGL : public duDebugDraw
{
public:
	virtual void depthMask(bool state);
	virtual void texture(bool state);
	virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f);
	virtual void vertex(const float* pos, unsigned int color);
	virtual void vertex(const float x, const float y, const float z, unsigned int color);
	virtual void vertex(const float* pos, unsigned int color, const float* uv);
	virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
	virtual void end();
	virtual unsigned int areaToCol(unsigned int area);
};

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

class TheWorld_UIClientApp_GL
{
public:
	TheWorld_UIClientApp_GL(void);
	virtual ~TheWorld_UIClientApp_GL(void);

	virtual bool init(void);
	virtual void cleanup(void);
	virtual bool initGraphicRendering(void);
	virtual bool manageGraphicRendering(bool& bLogoutRequired, int iLogin);
	virtual void cleanupGraphicRendering(void);
	void resetCommonSettings(void);

	// config
	enum _RenderingMode
	{
		InitialMenu,
		GraphicMode
	};
	virtual void setRenderingMode(enum _RenderingMode r, bool bForce = false);
	virtual enum _RenderingMode getRenderingMode(void);
	virtual bool getInitRenderingMode(void);
	virtual void setInitRenderingMode(bool);
	// config

private:
	virtual dtNavMesh* loadAll(const char* path);
	virtual void saveAll(const char* path, const dtNavMesh* mesh);
	void manageInput(bool& bLogoutRequired);
	float clampFrameRate(void);
	bool imguiRender(bool& bLogoutRequired);
	void imguiDemoWindow(ImVec4& clear_color);
	bool meshRender(bool& bLogoutRequired, float dt);
	bool meshRenderProva(bool& bLogoutRequired, float dt);
	void handleMeshRender(void);
	//bool handleBuild(void);

private:
	// Common Settings
	float m_cellSize;
	float m_cellHeight;
	float m_agentHeight;
	float m_agentRadius;
	float m_agentMaxClimb;
	float m_agentMaxSlope;
	float m_regionMinSize;
	float m_regionMergeSize;
	float m_edgeMaxLen;
	float m_edgeMaxError;
	float m_vertsPerPoly;
	float m_detailSampleDist;
	float m_detailSampleMaxError;
	int m_partitionType;
	// Common Settings

	unsigned char m_navMeshDrawFlags;

	BuildContext* m_ctx;
	rcCompactHeightfield* m_pchf;
	rcHeightfield* m_psolid;
	rcContourSet* m_pcset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;
	rcPolyMeshDetail* m_dmesh;

	float m_fFieldOfViewAngleY;
	float m_fZNear;
	float m_fZFar;
	float m_fCamr;
	int m_mousePos[2];
	int m_origMousePos[2]; // Used to compute mouse movement totals across frames.
	float m_cameraPos[3];
	float m_cameraEulers[2];
	float m_origCameraEulers[2];
	float m_rayStart[3];
	float m_rayEnd[3];
	bool m_mouseOverMenu;
	bool m_bRotate;
	bool m_movedDuringRotate;
	float m_moveFront;
	float m_moveBack;
	float m_moveLeft;
	float m_moveRight;
	float m_moveUp;
	float m_moveDown;
	float m_scrollZoom;
	Uint32 m_prevFrameTime;

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
	DrawMode m_drawMode;
	enum _RenderingMode m_renderingMode;
	InputGeom* m_geom;
	BuildContext m_buildCtx;
	dtNavMesh* m_navMesh;
	dtNavMeshQuery* m_navQuery;
	const std::string m_meshPath = "..\\TheWorld_ConsoleClient\\Meshes\\";
	DebugDrawGL m_dd;
	std::string m_message;
};