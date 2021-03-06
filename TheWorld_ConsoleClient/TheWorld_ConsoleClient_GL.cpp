// TheWorld_ConsoleClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "SDL.h"
#include "SDL_opengl.h"
#include <GL/glu.h>
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "DetourDebugDraw.h"
#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl.h"

#include "TheWorld_ConsoleClient_GL.h"
#include "TheWorld_ConsoleClient.h"
#include <conio.h>
#include <conio.h>
#include <json/json.h>
#include <boost/thread/thread.hpp>
#include "PlayerEntity.h"
#include "OtherEntity.h"


TheWorld_ClientApp_GL::TheWorld_ClientApp_GL(TheWorld_ClientAppConsole* pClientApp)
{
	m_navQuery = NULL;
	m_navMesh = NULL;
	m_geom = NULL;
	m_triareas = NULL;


	m_pIo = NULL;
	m_window = NULL;
	m_gl_context = NULL;
	m_drawMode = DRAWMODE_NAVMESH;
	
	//m_ctx = NULL;
	
	m_pchf = NULL;
	m_psolid = NULL;
	m_pcset = NULL;
	m_pmesh = NULL;
	m_dmesh = NULL;

	m_pClientApp = pClientApp;
}

TheWorld_ClientApp_GL::~TheWorld_ClientApp_GL()
{
	if (m_navQuery)
	{
		dtFreeNavMeshQuery(m_navQuery);
		m_navQuery = NULL;
	}

	if (m_navMesh)
	{
		dtFreeNavMesh(m_navMesh);
		m_navMesh = NULL;
	}

	if (m_geom)
	{
		delete m_geom;
		m_geom = NULL;
	}
}

void TheWorld_ClientApp_GL::resetCommonSettings()
{
	m_cellSize = 0.3f;
	m_cellHeight = 0.2f;
	m_agentHeight = 2.0f;
	m_agentRadius = 0.6f;
	m_agentMaxClimb = 0.9f;
	m_agentMaxSlope = 45.0f;
	m_regionMinSize = 8;
	m_regionMergeSize = 20;
	m_edgeMaxLen = 12.0f;
	m_edgeMaxError = 1.3f;
	m_vertsPerPoly = 6.0f;
	m_detailSampleDist = 6.0f;
	m_detailSampleMaxError = 1.0f;
	m_partitionType = SAMPLE_PARTITION_WATERSHED;
}

bool TheWorld_ClientApp_GL::init(void)
{
	m_navMeshDrawFlags = DU_DRAWNAVMESH_OFFMESHCONS | DU_DRAWNAVMESH_CLOSEDLIST;
	
	resetCommonSettings();
	
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("Could not initialise SDL.\nError: %s\n", SDL_GetError());
		return false;
	}

	// Enable depth buffer.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Set color channel depth.
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// 4x MSAA.
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(0, &displayMode);

	m_fFieldOfViewAngleY = 50.0f;
	m_fZNear = 1.0f;
	m_fZFar = -1;

	m_bFullScreen = false;
	m_uiFlags = SDL_WINDOW_OPENGL;
	m_uiFlags |= SDL_WINDOW_RESIZABLE;
	if (m_bFullScreen)
	{
		// Create a fullscreen window at the native resolution.
		m_iWidth = displayMode.w;
		m_iHeight = displayMode.h;
		m_uiFlags |= SDL_WINDOW_FULLSCREEN;
	}
	else
	{
		float aspect = 16.0f / 9.0f;
		m_iWidth = rcMin(displayMode.w, (int)(displayMode.h * aspect)) - 80;
		m_iHeight = displayMode.h - 80;
	}

	return true;
}

void TheWorld_ClientApp_GL::cleanup(void)
{
	SDL_Quit();
}

bool TheWorld_ClientApp_GL::handleGraphicRendering(bool& bLogoutRequired, int iLogin)
{
	if (bLogoutRequired)
		return true;
	
	if (m_pClientApp->getReinitAppModeRequired())
	{
		m_pClientApp->setReinitAppModeRequired(false);

		if (!m_pClientApp->getInitAppModeRequired())
		{ 
			cleanupGraphicRendering();
			m_pClientApp->setInitAppModeRequired(true);
		}
	}
	
	if (m_pClientApp->getInitAppModeRequired())
	{
		m_pClientApp->setInitAppModeRequired(false);
		
		if (!initGraphicRendering())
		{
			cleanupGraphicRendering();
			return false;
		}
	}

	float dt = clampFrameRate();

	handleInput(bLogoutRequired, dt);

	setViewport();

	if (!renderMesh(bLogoutRequired, dt))
	{
		cleanupGraphicRendering();
		return false;
	}

	if (!drawEntities(bLogoutRequired, dt))
	{
		cleanupGraphicRendering();
		return false;
	}

	if (!renderImgui(bLogoutRequired))
	{
		cleanupGraphicRendering();
		return false;
	}

	SDL_GL_SwapWindow(m_window);

	updatePlayerToServer();

#define LOGIN_STARTED				1
	if (bLogoutRequired)
	{
		if (iLogin == LOGIN_STARTED)
		{
			m_message = "Login in corso!\n";
			bLogoutRequired = false;
		}
		else
		{
			m_message = "Logout in corso!\n";
		}
	}

	if (bLogoutRequired)
		cleanupGraphicRendering();

	return true;
}

void TheWorld_ClientApp_GL::updatePlayerToServer(void)
{
	m_pClientApp->kbengine_UpdateVolatile();
}

void TheWorld_ClientApp_GL::setViewport(void)
{
	//m_clear_color = ImVec4(0f, 0f, 0f, 1.00f);
	//m_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	m_clear_color = ImVec4(0.3f, 0.3f, 0.32f, 1.00f);

	SDL_GetWindowSize(m_window, &m_iWidth, &m_iHeight);
	glViewport(0, 0, m_iWidth, m_iHeight);
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	glClearColor(m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_bAgentMousePosDrawn = false;
}

float TheWorld_ClientApp_GL::clampFrameRate(void)
{
	Uint32 time = SDL_GetTicks();
	float dt = (time - m_prevFrameTime) / 1000.0f;
	m_prevFrameTime = time;

	// Clamp the framerate so that we do not hog all the CPU.
	const float MIN_FRAME_TIME = 1.0f / 40.0f;
	if (dt < MIN_FRAME_TIME)
	{
		int ms = (int)((MIN_FRAME_TIME - dt) * 1000.0f);
		if (ms > 10) ms = 10;
		if (ms >= 0) SDL_Delay(ms);
	}

	return dt;
}

void TheWorld_ClientApp_GL::handleInput(bool& bLogoutRequired, float dt)
{
	if (bLogoutRequired)
		return;

	// Da portare fuori per gestire gli scroll del menù (per ora è inutilizzato)
	int mouseScroll = 0;
	m_tickRotateXAxis = 0;

	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);

		switch (event.type)
		{

		case SDL_QUIT:
			bLogoutRequired = true;
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
				if (event.window.windowID == SDL_GetWindowID(m_window))
				{
					bLogoutRequired = true;
				}
				break;
			default:
				break;
			}
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				bLogoutRequired = true;
				break;
			case SDLK_TAB:
				m_showGUI = !m_showGUI;
				break;
			case SDLK_SPACE:
				m_showAgentDetails = !m_showAgentDetails;
				break;
			default:
				break;
			}
			break;

		case SDL_MOUSEWHEEL:
			if (event.wheel.y < 0)
			{
				// wheel down
				if (m_mouseOverMenu)
				{
					mouseScroll++;
				}
				else
				{
					m_scrollZoom += 1.0f;
				}
			}
			else
			{
				if (m_mouseOverMenu)
				{
					mouseScroll--;
				}
				else
				{
					m_scrollZoom -= 1.0f;
				}
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_RIGHT)
			{
				if (!m_mouseOverMenu)
				{
					// Rotate view
					m_bRotate = true;
					//m_movedDuringRotate = false;
					m_origMousePos[0] = m_mousePos[0];
					m_origMousePos[1] = m_mousePos[1];
					m_origCameraEulers[0] = m_cameraEulers[0];
					m_origCameraEulers[1] = m_cameraEulers[1];
				}
			}
			break;

		case SDL_MOUSEBUTTONUP:
			// Handle mouse clicks here.
			if (event.button.button == SDL_BUTTON_RIGHT)
			{
				m_bRotate = false;
				m_message = "";	// DEBUG
				/*if (!m_mouseOverMenu)
				{
					if (!m_movedDuringRotate)
					{
						m_processHitTest = true;
						m_processHitTestShift = true;
					}
				}*/
			}
			/*else if (event.button.button == SDL_BUTTON_LEFT)
			{
				if (!m_mouseOverMenu)
				{
					m_processHitTest = true;
					m_processHitTestShift = (SDL_GetModState() & KMOD_SHIFT) ? true : false;
				}
			}*/
			break;

		case SDL_MOUSEMOTION:
			m_mousePos[0] = event.motion.x;
			m_mousePos[1] = m_iHeight - 1 - event.motion.y;

			if (m_bRotate)
			{
				int dx = m_mousePos[0] - m_origMousePos[0];
				int dy = m_mousePos[1] - m_origMousePos[1];
				m_cameraEulers[0] = m_origCameraEulers[0] - dy * 0.25f;
				m_cameraEulers[1] = m_origCameraEulers[1] + dx * 0.25f;
				/*if (dx * dx + dy * dy > 3 * 3)
				{
					m_movedDuringRotate = true;
				}*/
				// ad ogni tick di rotazione si incrementa o decrementa per spostare la camera in alto e basso in modo da compensare la vista
				if (dy > 0)
					m_tickRotateXAxis -= 1.0f;
				else
					m_tickRotateXAxis += 1.0f;
				char buffer[256];	sprintf(buffer, "dy = %d - m_tickRotateXAxis = %f", dy, m_tickRotateXAxis);	m_message = buffer;	// DEBUG
			}
			break;

		default:
			break;
		}
	}

	// Handle keyboard movement.
	const Uint8* keystate = SDL_GetKeyboardState(NULL);
	// i commenti di cui sotto per invertire W / UP con S / DOWN
	//m_moveFront = rcClamp(m_moveFront + dt * 4 * ((keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) ? 1 : -1), 0.0f, 1.0f);		// si inverte W / UP con S / DOWN
	m_moveFront = rcClamp(m_moveFront + dt * 4 * ((keystate[SDL_SCANCODE_S] /*|| keystate[SDL_SCANCODE_DOWN]*/) ? 1 : -1), 0.0f, 1.0f);
	m_moveLeft = rcClamp(m_moveLeft + dt * 4 * ((keystate[SDL_SCANCODE_A] /*|| keystate[SDL_SCANCODE_LEFT]*/) ? 1 : -1), 0.0f, 1.0f);
	//m_moveBack = rcClamp(m_moveBack + dt * 4 * ((keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) ? 1 : -1), 0.0f, 1.0f);		// si inverte S / DOWN con W / UP
	m_moveBack = rcClamp(m_moveBack + dt * 4 * ((keystate[SDL_SCANCODE_W] /*|| keystate[SDL_SCANCODE_UP]*/) ? 1 : -1), 0.0f, 1.0f);
	m_moveRight = rcClamp(m_moveRight + dt * 4 * ((keystate[SDL_SCANCODE_D] /*|| keystate[SDL_SCANCODE_RIGHT]*/) ? 1 : -1), 0.0f, 1.0f);
	m_moveUp = rcClamp(m_moveUp + dt * 4 * ((keystate[SDL_SCANCODE_Q] || keystate[SDL_SCANCODE_PAGEUP]) ? 1 : -1), 0.0f, 1.0f);
	m_moveDown = rcClamp(m_moveDown + dt * 4 * ((keystate[SDL_SCANCODE_E] || keystate[SDL_SCANCODE_PAGEDOWN]) ? 1 : -1), 0.0f, 1.0f);

	m_keybSpeed = 22.0f;
	if (SDL_GetModState() & KMOD_SHIFT)
	{
		m_keybSpeed *= 4.0f;
	}

	KBEntity* pPlayer = m_pClientApp->getPlayerEntity();
	if (pPlayer != NULL && m_geom)
	{
		const float* bmin = m_geom->getNavMeshBoundsMin();
		const float* bmax = m_geom->getNavMeshBoundsMax();

		float x = 0, y = 0, z = 0;
		//pPlayer->getNewPlayerPosition(x, y, z);
		pPlayer->getForClientPosition(x, y, z);
		//x -= 0.1f;	z -= 0.1f;	//yaw += 1.0f;
		if (keystate[SDL_SCANCODE_UP])
			z += 0.1f;
		if (keystate[SDL_SCANCODE_DOWN])
			z -= 0.1f;
		if (keystate[SDL_SCANCODE_LEFT])
			x -= 0.1f;
		if (keystate[SDL_SCANCODE_RIGHT])
			x += 0.1f;
#define BORDER_SIZE	2
		x = rcClamp(x, bmin[0] + BORDER_SIZE, bmax[0] - BORDER_SIZE);
		z = rcClamp(z, bmin[2] + BORDER_SIZE, bmax[2] - BORDER_SIZE);
		//pPlayer->setNewPlayerPosition(x, y, z);
		pPlayer->setForClientPosition(x, y, z);

		/*float yaw = 0, pitch = 0, roll = 0;
		pPlayer->getDesiderdDirection(yaw, pitch, roll);
		// ...
		pPlayer->setDesideredDirection(yaw, pitch, roll);*/

		if (keystate[SDL_SCANCODE_R] && pPlayer->getState() == ENTITY_STATE_DEAD)
		{
			m_pClientApp->kbengine_Relive();
		}

	}
}

bool TheWorld_ClientApp_GL::drawEntities(bool& bLogoutRequired, float dt)
{
	if (bLogoutRequired)
		return true;

	static const unsigned int monster_free = duRGBA(128, 25, 0, 129);	// red
	static const unsigned int monster_fight = duRGBA(248, 11, 27, 129);	// red
	static const unsigned int monster_dead = duRGBA(255, 255, 255, 129);	// red
	static const unsigned int player_free = duRGBA(51, 102, 0, 129);	// green
	static const unsigned int player_fight = duRGBA(248, 11, 27, 129);	// green
	static const unsigned int player_dead = duRGBA(255, 255, 255, 129);	// green
	static const unsigned int NPC_free = duRGBA(0, 192, 255, 129);		// light blue
	static const unsigned int NPC_fight = duRGBA(248, 11, 27, 129);		// light blue
	static const unsigned int NPC_dead = duRGBA(255, 255, 255, 129);		// light blue

	ENTITIES& entities = m_pClientApp->getEntities();

	ENTITIES::iterator iter = entities.begin();
	for (; iter != entities.end(); iter++)
	{
		KBEntity* pEntity = iter->second.get();
		if (pEntity->getIsInWorld())
		{
			float x, y, z;
			pEntity->getForClientPosition(x, y, z);
			//float pos[3];	pos[0] = x;	pos[1] = y;	pos[2] = z;
			unsigned int agentCol = 0;
			if (pEntity->isPlayer())
			{
				if (pEntity->getState() == ENTITY_STATE_FREE)
					agentCol = player_free;
				else if (pEntity->getState() == ENTITY_STATE_DEAD)
					agentCol = player_dead;
				else if (pEntity->getState() == ENTITY_STATE_FIGHT)
					agentCol = player_fight;
				else
					agentCol = player_free;
			}
			else
			{
				std::string s = pEntity->getClassName();
				if (caseInSensStringEqual(s, std::string("monster")))
				{
					if (pEntity->getState() == ENTITY_STATE_FREE)
						agentCol = monster_free;
					else if (pEntity->getState() == ENTITY_STATE_DEAD)
						agentCol = monster_dead;
					else if (pEntity->getState() == ENTITY_STATE_FIGHT)
						agentCol = monster_fight;
					else
						agentCol = monster_free;
				}
				else
				{
					if (pEntity->getState() == ENTITY_STATE_FREE)
						agentCol = NPC_free;
					else if (pEntity->getState() == ENTITY_STATE_DEAD)
						agentCol = NPC_dead;
					else if (pEntity->getState() == ENTITY_STATE_FIGHT)
						agentCol = NPC_fight;
					else
						agentCol = NPC_free;
				}
			}
			y = drawAgentWorldPos(x, z, agentCol);
			pEntity->setForClientPosition(x, y, z);
			//if (pEntity->isPlayer())
				//pEntity->setNewPlayerPosition(0, y, 0);
				//pEntity->setNewPlayerPosition(x, y, z);
			//pEntity->setIsOnGround(true);
		}
		if (pEntity->getIsOnGround())
		{
			pEntity->setIsOnGround(true);
		}
	}

	return true;
}

bool TheWorld_ClientApp_GL::renderMesh(bool& bLogoutRequired, float dt)
{
	/*{
		// P R O V A

		const dtNavMeshParams* navmeshParams = m_navMesh->getParams();
		int tx, ty;
		float worldPos[3] = { 5000, 1, 5000 };
		m_navMesh->calcTileLoc(worldPos, &tx, &ty);
		const dtMeshTile* tile = m_navMesh->getTileAt(tx, ty, 0);
		if (tile)
		{
			// dtNavMeshParams.orig = posizione della grid (x, y, z)
			// TILE (dtMeshTile.header.bmin|bmax) - min vertex bound: x=4999.10596 y=-4.05478001 z=4999.55176 - max vertex bound: x=5098.00000 y=-8.81377602 z=5098.79688 - width=98.8940430 height=99.2451172
			const float cs = 1.0f / tile->header->bvQuantFactor;
			for (int i = 0; i < tile->header->bvNodeCount; ++i)
			{
				const dtBVNode* n = &tile->bvTree[i];
				if (n->i >= 0)
				{
					float worldMinX = tile->header->bmin[0] + n->bmin[0] * cs;
					float worldMinY = tile->header->bmin[1] + n->bmin[1] * cs;
					float worldMinZ = tile->header->bmin[2] + n->bmin[2] * cs;
					float worldMaxX = tile->header->bmax[0] + n->bmax[0] * cs;
					float worldMaxY = tile->header->bmax[1] + n->bmax[1] * cs;
					float worldMaxZ = tile->header->bmax[2] + n->bmax[2] * cs;
				}
			}
		}
	}*/

	if (bLogoutRequired)
		return true;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);

	// Compute the projection matrix.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (m_fZFar < 0)
		m_fZFar = m_fCamr;
	float ratio = (float)m_iWidth / (float)m_iHeight;
	gluPerspective(m_fFieldOfViewAngleY, ratio, m_fZNear, m_fZFar);
	glGetDoublev(GL_PROJECTION_MATRIX, m_projectionMatrix);

	// Compute the modelview matrix.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(m_cameraEulers[0], 1, 0, 0);
	glRotatef(m_cameraEulers[1], 0, 1, 0);
	//m_cameraPos[0] = 5118;	m_cameraPos[1] = 71;	m_cameraPos[2] = 5119;	// DEBUG
	glTranslatef(-m_cameraPos[0], -m_cameraPos[1], -m_cameraPos[2]);
	glGetDoublev(GL_MODELVIEW_MATRIX, m_modelviewMatrix);

	float movex = (m_moveRight - m_moveLeft) * m_keybSpeed * dt;
	//float movey = (m_moveBack - m_moveFront) * m_keybSpeed * dt + m_scrollZoom * 2.0f;	// la rotella del mouse non agisce più come W / UP o S / DOWN a differenza della riga sotto
	float movey = (m_moveBack - m_moveFront) * m_keybSpeed * dt;							// lo spostamento in profondità della camera non è determinato dal movimento della rotella
	//movey += rcClamp(m_tickRotateXAxis, -0.5f, 0.5f);			// ad ogni tick di rotazione si incrementa o decrementa per spostare la camera in alto e basso in modo da compensare la vista

	m_cameraPos[0] += movex * (float)m_modelviewMatrix[0];
	m_cameraPos[1] += movex * (float)m_modelviewMatrix[4];
	m_cameraPos[2] += movex * (float)m_modelviewMatrix[8];

	//m_cameraEulers[0] - INITIAL_ROTATE_X_AXIS

	/*m_cameraPos[0] += movey * (float)m_modelviewMatrix[2];	// i tasti W / UP e S / DOWN non agiscono come traslazione lungo l'asse z (non incrementano / decrementano la profondità)
	m_cameraPos[1] += movey * (float)m_modelviewMatrix[6];
	m_cameraPos[2] += movey * (float)m_modelviewMatrix[10];*/
	m_cameraPos[0] += movey * (float)m_modelviewMatrix[1];		// i tasti W / UP e S / DOWN agiscono come traslazione lungo l'asse y (sposatno la camera in alto e in basso)
	m_cameraPos[1] += movey * (float)m_modelviewMatrix[5];
	m_cameraPos[2] += movey * (float)m_modelviewMatrix[9];

	//m_cameraPos[1] += (m_moveUp - m_moveDown) * keybSpeed * dt;						// a differenza dells riga sotto la rotella NON agisce come Q / PAGEUP o E / PAGEDOWN
	m_cameraPos[1] += (m_moveUp - m_moveDown) * m_keybSpeed * dt + m_scrollZoom * 2.0f;	// la rotella del mouse agisce come Q / PAGEUP o E / PAGEDOWN

	m_scrollZoom = 0;

	glEnable(GL_FOG);

	handleMeshRender();

	//static const unsigned int agentCol = duRGBA(128, 25, 0, 192);
	//drawAgentMousePos(agentCol);

	glDisable(GL_FOG);

	return true;
}

void TheWorld_ClientApp_GL::handleMeshRender(void)
{
	if (!m_geom || !m_geom->getMesh())
		return;

	glEnable(GL_FOG);
	glDepthMask(GL_TRUE);

	const float texScale = 1.0f / (m_cellSize * 10.0f);

	if (m_drawMode != DRAWMODE_NAVMESH_TRANS)
	{
		// Draw mesh
		// ==> Disegna la mesh originale quella su cui sono stati camminati i cammini
		duDebugDrawTriMeshSlope(&m_dd, m_geom->getMesh()->getVerts(), m_geom->getMesh()->getVertCount(),
			m_geom->getMesh()->getTris(), m_geom->getMesh()->getNormals(), m_geom->getMesh()->getTriCount(),
			m_agentMaxSlope, texScale);
		m_geom->drawOffMeshConnections(&m_dd);
	}
	//return;

	glDisable(GL_FOG);
	glDepthMask(GL_FALSE);

	// Draw bounds
	const float* bmin = m_geom->getNavMeshBoundsMin();
	const float* bmax = m_geom->getNavMeshBoundsMax();
	// ==> Disegna il box esterno che contiene la mesh originale
	duDebugDrawBoxWire(&m_dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], duRGBA(255, 255, 255, 128), 1.0f);
	//return;
	m_dd.begin(DU_DRAW_POINTS, 5.0f);
	m_dd.vertex(bmin[0], bmin[1], bmin[2], duRGBA(255, 255, 255, 128));
	m_dd.end();
	//return;

	if (m_navMesh && m_navQuery &&
		(m_drawMode == DRAWMODE_NAVMESH ||
			m_drawMode == DRAWMODE_NAVMESH_TRANS ||
			m_drawMode == DRAWMODE_NAVMESH_BVTREE ||
			m_drawMode == DRAWMODE_NAVMESH_NODES ||
			m_drawMode == DRAWMODE_NAVMESH_INVIS))
	{
		// Colora la mesh di navigazione (navmesh)
		if (m_drawMode != DRAWMODE_NAVMESH_INVIS)
			duDebugDrawNavMeshWithClosedList(&m_dd, *m_navMesh, *m_navQuery, m_navMeshDrawFlags);
		if (m_drawMode == DRAWMODE_NAVMESH_BVTREE)
			duDebugDrawNavMeshBVTree(&m_dd, *m_navMesh);
		if (m_drawMode == DRAWMODE_NAVMESH_NODES)
			duDebugDrawNavMeshNodes(&m_dd, *m_navQuery);
		//return;
		duDebugDrawNavMeshPolysWithFlags(&m_dd, *m_navMesh, SAMPLE_POLYFLAGS_DISABLED, duRGBA(0, 0, 0, 128));
	}

	glDepthMask(GL_TRUE);

	if (m_pchf && m_drawMode == DRAWMODE_COMPACT)
		duDebugDrawCompactHeightfieldSolid(&m_dd, *m_pchf);

	if (m_pchf && m_drawMode == DRAWMODE_COMPACT_DISTANCE)
		duDebugDrawCompactHeightfieldDistance(&m_dd, *m_pchf);
	if (m_pchf && m_drawMode == DRAWMODE_COMPACT_REGIONS)
		duDebugDrawCompactHeightfieldRegions(&m_dd, *m_pchf);
	if (m_psolid && m_drawMode == DRAWMODE_VOXELS)
	{
		glEnable(GL_FOG);
		duDebugDrawHeightfieldSolid(&m_dd, *m_psolid);
		glDisable(GL_FOG);
	}
	if (m_psolid && m_drawMode == DRAWMODE_VOXELS_WALKABLE)
	{
		glEnable(GL_FOG);
		duDebugDrawHeightfieldWalkable(&m_dd, *m_psolid);
		glDisable(GL_FOG);
	}
	if (m_pcset && m_drawMode == DRAWMODE_RAW_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawRawContours(&m_dd, *m_pcset);
		glDepthMask(GL_TRUE);
	}
	if (m_pcset && m_drawMode == DRAWMODE_BOTH_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawRawContours(&m_dd, *m_pcset, 0.5f);
		duDebugDrawContours(&m_dd, *m_pcset);
		glDepthMask(GL_TRUE);
	}
	if (m_pcset && m_drawMode == DRAWMODE_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawContours(&m_dd, *m_pcset);
		glDepthMask(GL_TRUE);
	}
	if (m_pchf && m_pcset && m_drawMode == DRAWMODE_REGION_CONNECTIONS)
	{
		duDebugDrawCompactHeightfieldRegions(&m_dd, *m_pchf);

		glDepthMask(GL_FALSE);
		duDebugDrawRegionConnections(&m_dd, *m_pcset);
		glDepthMask(GL_TRUE);
	}
	if (m_pmesh && m_drawMode == DRAWMODE_POLYMESH)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawPolyMesh(&m_dd, *m_pmesh);
		glDepthMask(GL_TRUE);
	}
	if (m_dmesh && m_drawMode == DRAWMODE_POLYMESH_DETAIL)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawPolyMeshDetail(&m_dd, *m_dmesh);
		glDepthMask(GL_TRUE);
	}

	m_geom->drawConvexVolumes(&m_dd);

	//NavMeshTesterTool::handleRender
	
	//renderToolStates();

	glDepthMask(GL_TRUE);
}

bool TheWorld_ClientApp_GL::renderImgui(bool& bLogoutRequired)
{
	if (bLogoutRequired)
		return true;

	if (!m_showGUI)
		return true;
	
	const ImVec4 textCol(255, 255, 255, 128);

	// Render GUI
	glDisable(GL_DEPTH_TEST);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, m_iWidth, 0, m_iHeight);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Al momento non impostato correttamente
	m_mouseOverMenu = false;

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame(m_window);
	ImGui::NewFrame();

	float fLineHeigth = ImGui::GetTextLineHeight();

	//imguiDemoWindow(m_clear_color);

	ImVec2 pos;

	if (m_bAgentMousePosDrawn)
	{
		GLdouble winx, winy, winz;
		if (GL_TRUE == gluProject((GLdouble)m_worldPosFromMousePos[0], (GLdouble)m_worldPosFromMousePos[1], (GLdouble)m_worldPosFromMousePos[2], m_modelviewMatrix, m_projectionMatrix, m_viewport, &winx, &winy, &winz))
		{
			pos.x = (float)winx; pos.y = m_iHeight - (float)winy - m_agentHeight * 20;
			//ImGui::SetCursorScreenPos(textPos);
			ImGui::SetNextWindowPos(pos);
			if (ImGui::Begin("AgentMouse", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize))
			{
				const ImVec4 textCol(255, 255, 255, 220);
				ImGui::TextColored(textCol, "AgentMouse");
			}
			ImGui::End();
		}
	}
	
	ENTITIES& entities = m_pClientApp->getEntities();

	ENTITIES::iterator iter = entities.begin();
	for (; iter != entities.end(); iter++)
	{
		KBEntity* pEntity = iter->second.get();
		if (pEntity->getIsInWorld())
		{
			float x, y, z;	pEntity->getForClientPosition(x, y, z);
			float entityPos[3];	entityPos[0] = x;	entityPos[1] = y;	entityPos[2] = z;
			GLdouble winx, winy, winz;
			if (GL_TRUE == gluProject((GLdouble)entityPos[0], (GLdouble)entityPos[1], (GLdouble)entityPos[2], m_modelviewMatrix, m_projectionMatrix, m_viewport, &winx, &winy, &winz))
			{
				pos.x = (float)winx; pos.y = m_iHeight - (float)winy - m_agentHeight * 20;
				//ImGui::SetCursorScreenPos(textPos);
				ImGui::SetNextWindowPos(pos);
				std::string name = "EntityName_"; name = name + pEntity->getName();
				if (ImGui::Begin(name.c_str(), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::TextColored(textCol, pEntity->getName());
					if (m_showAgentDetails)
					{
						float x, y, z;
						pEntity->getForClientPosition(x, y, z);
						float yaw, pitch, roll;
						pEntity->getForClientDirection(yaw, pitch, roll);
						const ImVec4 textCol(255, 255, 255, 220);
						ImGui::TextColored(textCol, "eid=%d onGroud=%s", pEntity->id(), pEntity->getIsOnGround()?"true":"false");
						ImGui::TextColored(textCol, "x=%f y=%f z=%f vel=%f", x, y, z, pEntity->getMoveSpeed());
						ImGui::TextColored(textCol, "yaw=%f pitch=%f roll=%f", yaw, pitch, roll);
					}
				}
				ImGui::End();
			}
		}
	}

	pos.x = 0; pos.y = 0;
	ImGui::SetNextWindowPos(pos);
	if (ImGui::Begin("ToolBar", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
	{
		if (ImGui::Button("Logout"))
		{
			bLogoutRequired = true;
		}
	}
	ImGui::End();

	pos.x = 0; pos.y = m_iHeight - fLineHeigth * 2;
	ImGui::SetNextWindowPos(pos);
	if (ImGui::Begin("StatusBar", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(textCol, "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::SameLine();
		ImGui::Text("          ");
		ImGui::SameLine();
		ImGui::TextColored(textCol, m_message.c_str());
	}
	ImGui::End();

	ImGui::Render();

	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool TheWorld_ClientApp_GL::initGraphicRendering(void)
{
	m_mouseOverMenu = false;
	m_bRotate = false;
	//m_movedDuringRotate = false;

	SDL_Renderer* renderer;
	int errorCode = SDL_CreateWindowAndRenderer(m_iWidth, m_iHeight, m_uiFlags, &m_window, &renderer);
	if (errorCode != 0 || !m_window || !renderer)
	{
		printf("Could not initialise SDL opengl\nError: %s\n", SDL_GetError());
		return false;
	}

	SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	m_gl_context = SDL_GL_CreateContext(m_window);
	SDL_GL_MakeCurrent(m_window, m_gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Fog.
	float fogColor[4] = { 0.32f, 0.31f, 0.30f, 1.0f };
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, m_fCamr * 0.1f);
	glFogf(GL_FOG_END, m_fCamr * 1.25f);
	glFogfv(GL_FOG_COLOR, fogColor);

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	m_pIo = &io;

	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);
	ImGui_ImplOpenGL2_Init();

	m_bShow_demo_window = true;
	m_bShow_another_window = false;
	m_bShow_helloworld_window = true;

	m_showGUI = true;
	m_showAgentDetails = false;
	//m_processHitTest = false;
	//m_processHitTestShift = false;

	m_fCamr = 1000;
	m_cameraPos[0] = 0;	m_cameraPos[1] = 0;	m_cameraPos[2] = 0;
	m_mousePos[0] = 0;	m_mousePos[1] = 0;
	m_origMousePos[0] = 0;	m_origMousePos[1] = 0;	// Used to compute mouse movement totals across frames.
	m_moveFront = 0.0f;	m_moveBack = 0.0f;	m_moveLeft = 0.0f;	m_moveRight = 0.0f;	m_moveUp = 0.0f;	m_moveDown = 0.0f;
	m_scrollZoom = 0;
#define INITIAL_CAM_ROTATE_X_AXIS	45
#define INITIAL_CAM_ROTATE_Y_AXIS	-45
	m_cameraEulers[0] = INITIAL_CAM_ROTATE_X_AXIS;	m_cameraEulers[1] = INITIAL_CAM_ROTATE_Y_AXIS;
	m_origCameraEulers[0] = 0;	m_origCameraEulers[1] = 0;

	m_keybSpeed = 22.0f;

	Space* pSpace = m_pClientApp->getSpaceWorld()->findSpace();
	if (pSpace)
	{
		std::string sResPath = pSpace->getResPath();

		m_geom = new InputGeom();
		if (!m_geom->load(&m_buildCtx, m_meshPath + sResPath + "\\undulating1.obj"))
			return false;

		//const BuildSettings* buildSettings = m_geom->getBuildSettings();

		m_navMesh = loadAll((m_meshPath + sResPath + "\\undulating1.navmesh").c_str());
		if (!m_navMesh)
			return false;

		m_navQuery = dtAllocNavMeshQuery();
		if (!m_navQuery)
			return false;

		dtStatus status = m_navQuery->init(m_navMesh, 2048);
		if (dtStatusFailed(status))
			return false;

		{
			const float* bmin = m_geom->getNavMeshBoundsMin();
			const float* bmax = m_geom->getNavMeshBoundsMax();
			const float* verts = m_geom->getMesh()->getVerts();
			const int nverts = m_geom->getMesh()->getVertCount();
			const int* tris = m_geom->getMesh()->getTris();
			const int ntris = m_geom->getMesh()->getTriCount();

			//
			// Step 1. Initialize build config.
			//

			// Init build configuration from GUI
			memset(&m_cfg, 0, sizeof(m_cfg));
			m_cfg.cs = m_cellSize;
			m_cfg.ch = m_cellHeight;
			m_cfg.walkableSlopeAngle = m_agentMaxSlope;
			m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
			m_cfg.walkableClimb = (int)floorf(m_agentMaxClimb / m_cfg.ch);
			m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
			m_cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
			m_cfg.maxSimplificationError = m_edgeMaxError;
			m_cfg.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
			m_cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
			m_cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
			m_cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
			m_cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;

			// Set the area where the navigation will be build.
			// Here the bounds of the input mesh are used, but the
			// area could be specified by an user defined box, etc.
			rcVcopy(m_cfg.bmin, bmin);
			rcVcopy(m_cfg.bmax, bmax);
			//float bminUser[3] = { 0, 0, 0 };
			//float bmaxUser[3] = { 50, 5, 50 };
			//rcVcopy(m_cfg.bmin, bminUser);
			//rcVcopy(m_cfg.bmax, bmaxUser);
			rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

			// Reset build times gathering.
			//m_ctx->resetTimers();

			// Start the build process.	
			//m_ctx->startTimer(RC_TIMER_TOTAL);

			//m_ctx->log(RC_LOG_PROGRESS, "Building navigation:");
			//m_ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg.width, m_cfg.height);
			//m_ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts / 1000.0f, ntris / 1000.0f);

			//
			// Step 2. Rasterize input polygon soup.
			//

			// Allocate voxel heightfield where we rasterize our input data to.
			m_psolid = rcAllocHeightfield();
			if (!m_psolid)
			{
				return false;
			}
			if (!rcCreateHeightfield(&m_ctx, *m_psolid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
			{
				return false;
			}

			// Allocate array that can hold triangle area types.
			// If you have multiple meshes you need to process, allocate
			// and array which can hold the max number of triangles you need to process.
			m_triareas = new unsigned char[ntris];
			if (!m_triareas)
			{
				return false;
			}

			// Find triangles which are walkable based on their slope and rasterize them.
			// If your input data is multiple meshes, you can transform them here, calculate
			// the are type for each of the meshes and rasterize them.
			memset(m_triareas, 0, ntris * sizeof(unsigned char));
			rcMarkWalkableTriangles(&m_ctx, m_cfg.walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
			if (!rcRasterizeTriangles(&m_ctx, verts, nverts, tris, m_triareas, ntris, *m_psolid, m_cfg.walkableClimb))
			{
				return false;
			}

			//if (!m_keepInterResults)
			//{
			//	delete[] m_triareas;
			//	m_triareas = 0;
			//}

			//
			// Step 3. Filter walkables surfaces.
			//

			// Once all geoemtry is rasterized, we do initial pass of filtering to
			// remove unwanted overhangs caused by the conservative rasterization
			// as well as filter spans where the character cannot possibly stand.
			//if (m_filterLowHangingObstacles)
			//	rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_psolid);
			//if (m_filterLedgeSpans)
			//	rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_psolid);
			//if (m_filterWalkableLowHeightSpans)
			//	rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_psolid);


			//
			// Step 4. Partition walkable surface to simple regions.
			//

			// Compact the heightfield so that it is faster to handle from now on.
			// This will result more cache coherent data as well as the neighbours
			// between walkable cells will be calculated.
			m_pchf = rcAllocCompactHeightfield();
			if (!m_pchf)
			{
				return false;
			}
			if (!rcBuildCompactHeightfield(&m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_psolid, *m_pchf))
			{
				return false;
			}
		}

	}

	if (m_geom)
	{
		const float* bmin = 0;
		const float* bmax = 0;
		bmin = m_geom->getNavMeshBoundsMin();
		bmax = m_geom->getNavMeshBoundsMax();

		// Reset camera and fog to match the mesh bounds.
		if (bmin && bmax)
		{
			m_fCamr = sqrtf(rcSqr(bmax[0] - bmin[0]) +
				rcSqr(bmax[1] - bmin[1]) +
				rcSqr(bmax[2] - bmin[2])) / 2;
			m_cameraPos[0] = (bmax[0] + bmin[0]) / 2 + m_fCamr;
			m_cameraPos[1] = (bmax[1] + bmin[1]) / 2 + m_fCamr;
			m_cameraPos[2] = (bmax[2] + bmin[2]) / 2 + m_fCamr;
			m_fCamr *= 3;
		}
		glFogf(GL_FOG_START, m_fCamr * 0.1f);
		glFogf(GL_FOG_END, m_fCamr * 1.25f);
	}

	m_message = "";
	
	m_prevFrameTime = SDL_GetTicks();

	return true;
}

void TheWorld_ClientApp_GL::cleanupGraphicRendering(void)
{
	if (m_navQuery)
	{
		dtFreeNavMeshQuery(m_navQuery);
		m_navQuery = NULL;
	}

	if (m_navMesh)
	{
		dtFreeNavMesh(m_navMesh);
		m_navMesh = NULL;
	}
	
	if (m_geom)
	{
		delete m_geom;
		m_geom = NULL;
	}
	
	// Cleanup
	if (m_pIo)
	{
		ImGui_ImplOpenGL2_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
		m_pIo = NULL;
	}

	if (m_gl_context)
	{
		SDL_GL_DeleteContext(m_gl_context);
		m_gl_context = false;
	}
	
	if (m_window)
	{
		SDL_DestroyWindow(m_window);
		m_window = NULL;
	}
}

dtNavMesh* TheWorld_ClientApp_GL::loadAll(const char* path)
{
	FILE* fp = fopen(path, "rb");
	if (!fp) return 0;

	// Read header.
	NavMeshSetHeader header;
	size_t readLen = fread(&header, sizeof(NavMeshSetHeader), 1, fp);
	if (readLen != 1)
	{
		fclose(fp);
		return 0;
	}
	if (header.magic != NAVMESHSET_MAGIC)
	{
		fclose(fp);
		return 0;
	}
	if (header.version != NAVMESHSET_VERSION)
	{
		fclose(fp);
		return 0;
	}

	dtNavMesh* mesh = dtAllocNavMesh();
	if (!mesh)
	{
		fclose(fp);
		return 0;
	}
	dtStatus status = mesh->init(&header.params);
	if (dtStatusFailed(status))
	{
		fclose(fp);
		return 0;
	}

	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		NavMeshTileHeader tileHeader;
		readLen = fread(&tileHeader, sizeof(tileHeader), 1, fp);
		if (readLen != 1)
		{
			fclose(fp);
			return 0;
		}

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data) break;
		memset(data, 0, tileHeader.dataSize);
		readLen = fread(data, tileHeader.dataSize, 1, fp);
		if (readLen != 1)
		{
			dtFree(data);
			fclose(fp);
			return 0;
		}

		mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
	}

	fclose(fp);

	return mesh;
}

void TheWorld_ClientApp_GL::saveAll(const char* path, const dtNavMesh* mesh)
{
	if (!mesh) return;

	FILE* fp = fopen(path, "wb");
	if (!fp)
		return;

	// Store header.
	NavMeshSetHeader header;
	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numTiles = 0;
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		header.numTiles++;
	}
	memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));
	fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

	// Store tiles.
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = mesh->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;
		fwrite(&tileHeader, sizeof(tileHeader), 1, fp);

		fwrite(tile->data, tile->dataSize, 1, fp);
	}

	fclose(fp);
}

// depends by m_mousePos[0], m_mousePos[1], 
void TheWorld_ClientApp_GL::drawAgentMousePos(const unsigned int agentCol)
{
	float rayStart[3];
	float rayEnd[3];

	// Get hit ray position and direction.
	GLdouble x, y, z;
	gluUnProject(m_mousePos[0], m_mousePos[1], 0.0f, m_modelviewMatrix, m_projectionMatrix, m_viewport, &x, &y, &z);
	rayStart[0] = (float)x;
	rayStart[1] = (float)y;
	rayStart[2] = (float)z;
	gluUnProject(m_mousePos[0], m_mousePos[1], 1.0f, m_modelviewMatrix, m_projectionMatrix, m_viewport, &x, &y, &z);
	rayEnd[0] = (float)x;
	rayEnd[1] = (float)y;
	rayEnd[2] = (float)z;

	float hitTime;
	bool hit = m_geom->raycastMesh(rayStart, rayEnd, hitTime);

	if (hit)
	{
		m_worldPosFromMousePos[0] = rayStart[0] + (rayEnd[0] - rayStart[0]) * hitTime;
		m_worldPosFromMousePos[1] = rayStart[1] + (rayEnd[1] - rayStart[1]) * hitTime;
		m_worldPosFromMousePos[2] = rayStart[2] + (rayEnd[2] - rayStart[2]) * hitTime;

		drawAgentWorldPos(m_worldPosFromMousePos, agentCol);

		m_bAgentMousePosDrawn = true;
	}

}

// depends by m_dd, m_agentRadius, m_agentHeight, m_agentMaxClimb
float TheWorld_ClientApp_GL::drawAgentWorldPos(const float x, const float z, const unsigned int agentCol)
{
	float rayStart[3];
	float rayEnd[3];

	if (!m_geom)
		return 0;

	const float* bmin = m_geom->getMeshBoundsMin();
	const float* bmax = m_geom->getMeshBoundsMax();

	rayStart[0] = (float)x;
	rayStart[1] = (float)bmax[1];
	rayStart[2] = (float)z;

	rayEnd[0] = (float)x;
	rayEnd[1] = (float)bmin[1];
	rayEnd[2] = (float)z;

	float worldPos[3];

	float hitTime;
	bool hit = m_geom->raycastMesh(rayStart, rayEnd, hitTime);

	if (hit)
	{
		worldPos[0] = rayStart[0] + (rayEnd[0] - rayStart[0]) * hitTime;
		worldPos[1] = rayStart[1] + (rayEnd[1] - rayStart[1]) * hitTime;
		worldPos[2] = rayStart[2] + (rayEnd[2] - rayStart[2]) * hitTime;
	}

	//float yMesh = 0;

	/*{
		// Where m_pchf is an instance of a rcCompactHeightfield.
		const float cs = m_pchf->cs;
		const float ch = chf.ch;
		for (int y = 0; y < chf.height; ++y)
		{
			for (int x = 0; x < chf.width; ++x)
			{
				// Deriving the minimum corner of the grid location.
				const float fx = chf.bmin[0] + x * cs;
				const float fz = chf.bmin[2] + y * cs;
				// Get the cell for the grid location then iterate
				// up the column.
				const rcCompactCell& c = chf.cells[x + y * chf.width];
				for (unsigned i = c.index, ni = c.index + c.count; i < ni; ++i)
				{
					const rcCompactSpan& s = chf.spans[i];
					//Deriving the minimum(floor) of the span.
						const float fy = chf.bmin[1] + (s.y + 1) * ch;
					// Testing the area assignment of the span.
					if (chf.areas[i] == RC_WALKABLE_AREA)
					{
						// The span is in the default 'walkable area'.
					}
					else if (chf.areas[i] == RC_NULL_AREA)
					{
						// The surface is not considered walkable.
						// E.g. It was filtered out during the build processes.
					}
					else
					{
						// Do something. (Only applicable for custom build
						// build processes.)
					}
					// Iterating the connected axis-neighbor spans.
					for (int dir = 0; dir < 4; ++dir)
					{
						if (rcGetCon(s, dir) != RC_NOT_CONNECTED)
						{
							// There is a neighbor in this direction.
							const int nx = x + rcGetDirOffsetX(dir);
							const int ny = y + rcGetDirOffsetY(dir);
							const int ni = (int)chf.cells[nx + ny * w].index + rcGetCon(s, 0);
							const rcCompactSpan& ns = chf.spans[ni];
							// Do something with the neighbor span.
						}
					}
				}
			}
		}
	}*/
	
	/*{
		// P R O V A

		const dtNavMeshParams* navmeshParams = m_navMesh->getParams();
		int tx, ty;
		float worldPos[3] = { 5000, 1, 5000 };
		m_navMesh->calcTileLoc(worldPos, &tx, &ty);
		const dtMeshTile* tile = m_navMesh->getTileAt(tx, ty, 0);
		if (tile)
		{
			// dtNavMeshParams.orig = posizione della grid (x, y, z)
			// TILE (dtMeshTile.header.bmin|bmax) - min vertex bound: x=4999.10596 y=-4.05478001 z=4999.55176 - max vertex bound: x=5098.00000 y=-8.81377602 z=5098.79688 - width=98.8940430 height=99.2451172
			const float cs = 1.0f / tile->header->bvQuantFactor;
			for (int i = 0; i < tile->header->bvNodeCount; ++i)
			{
				const dtBVNode* n = &tile->bvTree[i];
				if (n->i >= 0)
				{
					float worldMinX = tile->header->bmin[0] + n->bmin[0] * cs;
					float worldMinY = tile->header->bmin[1] + n->bmin[1] * cs;
					float worldMinZ = tile->header->bmin[2] + n->bmin[2] * cs;
					float worldMaxX = tile->header->bmax[0] + n->bmax[0] * cs;
					float worldMaxY = tile->header->bmax[1] + n->bmax[1] * cs;
					float worldMaxZ = tile->header->bmax[2] + n->bmax[2] * cs;
				}
			}
		}
	}*/

	drawAgentWorldPos(worldPos, agentCol);
	
	return worldPos[1];
}

// depends by m_dd, m_agentRadius, m_agentHeight, m_agentMaxClimb
void TheWorld_ClientApp_GL::drawAgentWorldPos(const float* pos, const unsigned int agentCol)
{
	m_dd.depthMask(false);

	// Agent dimensions as a Cylinder with radius = agent radius and height = agent heigth
	duDebugDrawCylinderWire(&m_dd, pos[0] - m_agentRadius, pos[1] + 0.02f, pos[2] - m_agentRadius, pos[0] + m_agentRadius, pos[1] + m_agentHeight, pos[2] + m_agentRadius, agentCol, 2.0f);

	// Agent Radius - Half Cylinder orizontal circle
	duDebugDrawCircle(&m_dd, pos[0], pos[1] + m_agentMaxClimb, pos[2], m_agentRadius, duRGBA(0, 0, 0, 64), 1.0f);

	// avatar as a vertical line
	unsigned int colb = duRGBA(0, 0, 0, 196);
	m_dd.begin(DU_DRAW_LINES);
	m_dd.vertex(pos[0], pos[1] - m_agentMaxClimb, pos[2], colb);
	m_dd.vertex(pos[0], pos[1] + m_agentMaxClimb, pos[2], colb);
	m_dd.vertex(pos[0] - m_agentRadius / 2, pos[1] + 0.02f, pos[2], colb);
	m_dd.vertex(pos[0] + m_agentRadius / 2, pos[1] + 0.02f, pos[2], colb);
	m_dd.vertex(pos[0], pos[1] + 0.02f, pos[2] - m_agentRadius / 2, colb);
	m_dd.vertex(pos[0], pos[1] + 0.02f, pos[2] + m_agentRadius / 2, colb);
	m_dd.end();

	m_dd.depthMask(true);
}

BuildContext::BuildContext()
//	:
//	m_messageCount(0),
//	m_textPoolSize(0)
{
//	memset(m_messages, 0, sizeof(char*) * MAX_MESSAGES);

	resetTimers();
}

void BuildContext::doStartTimer(const rcTimerLabel label)
{
	m_startTime[label] = getPerfTime();
}

void BuildContext::doStopTimer(const rcTimerLabel label)
{
	const TimeVal endTime = getPerfTime();
	const TimeVal deltaTime = endTime - m_startTime[label];
	if (m_accTime[label] == -1)
		m_accTime[label] = deltaTime;
	else
		m_accTime[label] += deltaTime;
}

int BuildContext::doGetAccumulatedTime(const rcTimerLabel label) const
{
	return getPerfTimeUsec(m_accTime[label]);
}

void BuildContext::doResetTimers()
{
	for (int i = 0; i < RC_MAX_TIMERS; ++i)
		m_accTime[i] = -1;
}

class GLCheckerTexture
{
	unsigned int m_texId;
public:
	GLCheckerTexture() : m_texId(0)
	{
	}

	~GLCheckerTexture()
	{
		if (m_texId != 0)
			glDeleteTextures(1, &m_texId);
	}
	void bind()
	{
		if (m_texId == 0)
		{
			// Create checker pattern.
			const unsigned int col0 = duRGBA(215, 215, 215, 255);
			const unsigned int col1 = duRGBA(255, 255, 255, 255);
			static const int TSIZE = 64;
			unsigned int data[TSIZE * TSIZE];

			glGenTextures(1, &m_texId);
			glBindTexture(GL_TEXTURE_2D, m_texId);

			int level = 0;
			int size = TSIZE;
			while (size > 0)
			{
				for (int y = 0; y < size; ++y)
					for (int x = 0; x < size; ++x)
						data[x + y * size] = (x == 0 || y == 0) ? col0 : col1;
				glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
				size /= 2;
				level++;
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_texId);
		}
	}
};
GLCheckerTexture g_tex;

void DebugDrawGL::depthMask(bool state)
{
	glDepthMask(state ? GL_TRUE : GL_FALSE);
}

void DebugDrawGL::texture(bool state)
{
	if (state)
	{
		glEnable(GL_TEXTURE_2D);
		g_tex.bind();
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
	}
}

void DebugDrawGL::begin(duDebugDrawPrimitives prim, float size)
{
	switch (prim)
	{
	case DU_DRAW_POINTS:
		glPointSize(size);
		glBegin(GL_POINTS);
		break;
	case DU_DRAW_LINES:
		glLineWidth(size);
		glBegin(GL_LINES);
		break;
	case DU_DRAW_TRIS:
		glBegin(GL_TRIANGLES);
		break;
	case DU_DRAW_QUADS:
		glBegin(GL_QUADS);
		break;
	};
}

void DebugDrawGL::vertex(const float* pos, unsigned int color)
{
	glColor4ubv((GLubyte*)&color);
	glVertex3fv(pos);
}

void DebugDrawGL::vertex(const float x, const float y, const float z, unsigned int color)
{
	glColor4ubv((GLubyte*)&color);
	glVertex3f(x, y, z);
}

void DebugDrawGL::vertex(const float* pos, unsigned int color, const float* uv)
{
	glColor4ubv((GLubyte*)&color);
	glTexCoord2fv(uv);
	glVertex3fv(pos);
}

void DebugDrawGL::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	glColor4ubv((GLubyte*)&color);
	glTexCoord2f(u, v);
	glVertex3f(x, y, z);
}

void DebugDrawGL::end()
{
	glEnd();
	glLineWidth(1.0f);
	glPointSize(1.0f);
}

unsigned int DebugDrawGL::areaToCol(unsigned int area)
{
	switch (area)
	{
		// Ground (0) : light blue
	case SAMPLE_POLYAREA_GROUND: return duRGBA(0, 192, 255, 255);
		// Water : blue
	case SAMPLE_POLYAREA_WATER: return duRGBA(0, 0, 255, 255);
		// Road : brown
	case SAMPLE_POLYAREA_ROAD: return duRGBA(50, 20, 12, 255);
		// Door : cyan
	case SAMPLE_POLYAREA_DOOR: return duRGBA(0, 255, 255, 255);
		// Grass : green
	case SAMPLE_POLYAREA_GRASS: return duRGBA(0, 255, 0, 255);
		// Jump : yellow
	case SAMPLE_POLYAREA_JUMP: return duRGBA(255, 255, 0, 255);
		// Unexpected : red
	default: return duRGBA(255, 0, 0, 255);
	}
}

/*bool TheWorld_UIClientApp_GL::handleBuild()
{
	if (!m_geom || !m_geom->getMesh())
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Input mesh is not specified.");
		return false;
	}

	cleanup();

	const float* bmin = m_geom->getNavMeshBoundsMin();
	const float* bmax = m_geom->getNavMeshBoundsMax();
	const float* verts = m_geom->getMesh()->getVerts();
	const int nverts = m_geom->getMesh()->getVertCount();
	const int* tris = m_geom->getMesh()->getTris();
	const int ntris = m_geom->getMesh()->getTriCount();

	//
	// Step 1. Initialize build config.
	//

	// Init build configuration from GUI
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = m_cellSize;
	m_cfg.ch = m_cellHeight;
	m_cfg.walkableSlopeAngle = m_agentMaxSlope;
	m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(m_agentMaxClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
	m_cfg.maxSimplificationError = m_edgeMaxError;
	m_cfg.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
	m_cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
	m_cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;

	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcVcopy(m_cfg.bmin, bmin);
	rcVcopy(m_cfg.bmax, bmax);
	//float bminUser[3] = { 0, 0, 0 };
	//float bmaxUser[3] = { 50, 5, 50 };
	//rcVcopy(m_cfg.bmin, bminUser);
	//rcVcopy(m_cfg.bmax, bmaxUser);
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	// Reset build times gathering.
	m_ctx->resetTimers();

	// Start the build process.	
	m_ctx->startTimer(RC_TIMER_TOTAL);

	m_ctx->log(RC_LOG_PROGRESS, "Building navigation:");
	m_ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg.width, m_cfg.height);
	m_ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts / 1000.0f, ntris / 1000.0f);

	//
	// Step 2. Rasterize input polygon soup.
	//

	// Allocate voxel heightfield where we rasterize our input data to.
	m_psolid = rcAllocHeightfield();
	if (!m_psolid)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		return false;
	}
	if (!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		return false;
	}

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	m_triareas = new unsigned char[ntris];
	if (!m_triareas)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", ntris);
		return false;
	}

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(m_triareas, 0, ntris * sizeof(unsigned char));
	rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
	if (!rcRasterizeTriangles(m_ctx, verts, nverts, tris, m_triareas, ntris, *m_solid, m_cfg.walkableClimb))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not rasterize triangles.");
		return false;
	}

	if (!m_keepInterResults)
	{
		delete[] m_triareas;
		m_triareas = 0;
	}

	//
	// Step 3. Filter walkables surfaces.
	//

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	if (m_filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
	if (m_filterLedgeSpans)
		rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	if (m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);


	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_pchf = rcAllocCompactHeightfield();
	if (!m_pchf)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		return false;
	}
	if (!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		return false;
	}

	if (!m_keepInterResults)
	{
		rcFreeHeightField(m_solid);
		m_solid = 0;
	}

	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		return false;
	}

	// (Optional) Mark areas.
	const ConvexVolume* vols = m_geom->getConvexVolumes();
	for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
		rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);


	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	// There are 3 martitioning methods, each with some pros and cons:
	// 1) Watershed partitioning
	//   - the classic Recast partitioning
	//   - creates the nicest tessellation
	//   - usually slowest
	//   - partitions the heightfield into nice regions without holes or overlaps
	//   - the are some corner cases where this method creates produces holes and overlaps
	//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
	//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
	//   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
	// 2) Monotone partioning
	//   - fastest
	//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
	//   - creates long thin polygons, which sometimes causes paths with detours
	//   * use this if you want fast navmesh generation
	// 3) Layer partitoining
	//   - quite fast
	//   - partitions the heighfield into non-overlapping regions
	//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
	//   - produces better triangles than monotone partitioning
	//   - does not have the corner cases of watershed partitioning
	//   - can be slow and create a bit ugly tessellation (still better than monotone)
	//     if you have large open areas with small obstacles (not a problem if you use tiles)
	//   * good choice to use for tiled navmesh with medium and small sized tiles

	if (m_partitionType == SAMPLE_PARTITION_WATERSHED)
	{
		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if (!rcBuildDistanceField(m_ctx, *m_chf))
		{
			m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
			return false;
		}

		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildRegions(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
			return false;
		}
	}
	else if (m_partitionType == SAMPLE_PARTITION_MONOTONE)
	{
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if (!rcBuildRegionsMonotone(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
			return false;
		}
	}
	else // SAMPLE_PARTITION_LAYERS
	{
		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildLayerRegions(m_ctx, *m_chf, 0, m_cfg.minRegionArea))
		{
			m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
			return false;
		}
	}

	//
	// Step 5. Trace and simplify region contours.
	//

	// Create contours.
	m_pcset = rcAllocContourSet();
	if (!m_pcset)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
		return false;
	}
	if (!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_pcset))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
		return false;
	}

	//
	// Step 6. Build polygons mesh from contours.
	//

	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if (!m_pmesh)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		return false;
	}
	if (!rcBuildPolyMesh(m_ctx, *m_pcset, m_cfg.maxVertsPerPoly, *m_pmesh))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		return false;
	}

	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//

	m_dmesh = rcAllocPolyMeshDetail();
	if (!m_dmesh)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
		return false;
	}

	if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
		return false;
	}

	if (!m_keepInterResults)
	{
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_pcset);
		m_pcset = 0;
	}

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//

	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
	if (m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
	{
		unsigned char* navData = 0;
		int navDataSize = 0;

		// Update poly flags from areas.
		for (int i = 0; i < m_pmesh->npolys; ++i)
		{
			if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
				m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;

			if (m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			}
			else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			}
			else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}


		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = m_pmesh->verts;
		params.vertCount = m_pmesh->nverts;
		params.polys = m_pmesh->polys;
		params.polyAreas = m_pmesh->areas;
		params.polyFlags = m_pmesh->flags;
		params.polyCount = m_pmesh->npolys;
		params.nvp = m_pmesh->nvp;
		params.detailMeshes = m_dmesh->meshes;
		params.detailVerts = m_dmesh->verts;
		params.detailVertsCount = m_dmesh->nverts;
		params.detailTris = m_dmesh->tris;
		params.detailTriCount = m_dmesh->ntris;
		params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
		params.offMeshConRad = m_geom->getOffMeshConnectionRads();
		params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
		params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
		params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
		params.offMeshConUserID = m_geom->getOffMeshConnectionId();
		params.offMeshConCount = m_geom->getOffMeshConnectionCount();
		params.walkableHeight = m_agentHeight;
		params.walkableRadius = m_agentRadius;
		params.walkableClimb = m_agentMaxClimb;
		rcVcopy(params.bmin, m_pmesh->bmin);
		rcVcopy(params.bmax, m_pmesh->bmax);
		params.cs = m_cfg.cs;
		params.ch = m_cfg.ch;
		params.buildBvTree = true;

		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			m_ctx->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
			return false;
		}

		m_navMesh = dtAllocNavMesh();
		if (!m_navMesh)
		{
			dtFree(navData);
			m_ctx->log(RC_LOG_ERROR, "Could not create Detour navmesh");
			return false;
		}

		dtStatus status;

		status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status))
		{
			dtFree(navData);
			m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh");
			return false;
		}

		status = m_navQuery->init(m_navMesh, 2048);
		if (dtStatusFailed(status))
		{
			m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
			return false;
		}
	}

	m_ctx->stopTimer(RC_TIMER_TOTAL);

	// Show performance stats.
	duLogBuildTimes(*m_ctx, m_ctx->getAccumulatedTime(RC_TIMER_TOTAL));
	m_ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);

	m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL) / 1000.0f;

	if (m_tool)
		m_tool->init(this);
	initToolStates(this);

	return true;
}*/

/*// Draw start and end point labels
if (m_sposSet && gluProject((GLdouble)m_spos[0], (GLdouble)m_spos[1], (GLdouble)m_spos[2],
	model, proj, view, &x, &y, &z))
{
	imguiDrawText((int)x, (int)(y - 25), IMGUI_ALIGN_CENTER, "Start", imguiRGBA(0, 0, 0, 220));
}*/

/*void TheWorld_UIClientApp_GL::imguiDemoWindow(ImVec4& clear_color)
{
	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (m_bShow_demo_window)
		ImGui::ShowDemoWindow(&m_bShow_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	if (m_bShow_helloworld_window)
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &m_bShow_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &m_bShow_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (m_bShow_another_window)
	{
		ImGui::Begin("Another Window", &m_bShow_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			m_bShow_another_window = false;
		ImGui::End();
	}

}*/

/*bool TheWorld_UIClientApp_GL::meshRenderProva(bool& bLogoutRequired, float dt)
{
	static GLboolean should_rotate = GL_TRUE;
	// Our angle of rotation.
	static float angle = 0.0f;
	if (should_rotate) {

		if (++angle > 360.0f) {
			angle = 0.0f;
		}

	}

	float ratio = (float)m_iWidth / (float)m_iHeight;

	// Our shading model--Gouraud (smooth).
	glShadeModel(GL_SMOOTH);

	// Culling.
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	//
	// Change to the projection matrix and set
	// our viewing volume.
	//
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, ratio, 1.0, 1024.0);

	// We don't want to modify the projection matrix.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Move down the z-axis.
	glTranslatef(0.0, 0.0, -5.0);

	// Rotate.
	glRotatef(angle, 1.0, 1.0, 1.0);

	meshRenderProva1();

	return true;
}*/

/*void DrawSphere(float a_Radius, int a_Slices, int a_Stacks)
{
	float X, Y, Z;
	float X1, Y1, Z1;
	float M = (float)a_Slices;
	float N = (float)a_Stacks;
	for (int i = 0; i <= a_Slices + 1; i++)
	{
		glBegin(GL_LINE_STRIP);
		for (int j = 1; j <= a_Stacks + 1; j++)
		{
			X = a_Radius * sin((i * 3.1415926535897932385f) / (M + 1)) * cos((j * 2 * 3.1415926535897932385f) / N);
			Y = a_Radius * sin((i * 3.1415926535897932385f) / (M + 1)) * sin((j * 2 * 3.1415926535897932385f) / N);
			Z = -a_Radius * cos((i * 3.1415926535897932385f) / (M + 1));
			glVertex3f(X, Y, Z);
		}
		glEnd();
		glBegin(GL_LINES);
		for (int j = 1; j <= a_Stacks + 1; j++)
		{
			X = a_Radius * sin((i * 3.1415926535897932385f) / (M + 1)) * cos((j * 2 * 3.1415926535897932385f) / N);
			Y = a_Radius * sin((i * 3.1415926535897932385f) / (M + 1)) * sin((j * 2 * 3.1415926535897932385f) / N);
			Z = -a_Radius * cos((i * 3.1415926535897932385f) / (M + 1));
			X1 = a_Radius * sin(((i + 1) * 3.1415926535897932385f) / (M + 1)) * cos(((j) * 2 * 3.1415926535897932385f) / N);
			Y1 = a_Radius * sin(((i + 1) * 3.1415926535897932385f) / (M + 1)) * sin(((j) * 2 * 3.1415926535897932385f) / N);
			Z1 = -a_Radius * cos(((i + 1) * 3.1415926535897932385f) / (M + 1));
			glVertex3f(X, Y, Z);
			glVertex3f(X1, Y1, Z1);
		}
		glEnd();
	}
}*/

/*void TheWorld_UIClientApp_GL::meshRenderProva1(void)
{
	// Angle of revolution around the nucleus
	static float fElect1 = 0.0f;
	static const int slices = 100;
	static const int stacks = 100;

	// Clear the window with current clearing color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Reset the modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Translate the whole scene out and into vi ew
	// This is the initial viewing transformation
	glTranslatef(0.0f, 0.0f, -200.0f);
	
	glRotatef(45, 1, 0, 0);
	//glRotatef(-45, 0, 1, 0);

	// Red Nucleus
	//glRGB(255, 0, 0);
	glColor3f(255, 0, 0);
	DrawSphere(10.0f, slices, stacks);
	//auxSolidSphere(10.0f);
	
	// Yellow Electrons
	//glRGB(255, 255, 0);
	glColor3f(255, 255, 0);
	// First Electron Orbit
	// Save viewing transformation
	glPushMatrix();
	// Rotate by angle of revolution
	glRotatef(fElect1, 0.0f, 1.0f, 0.0f);
	// Translate out from origin to orbit distance
	glTranslatef(90.0f, 0.0f, 0.0f);
	// Draw the electron
	DrawSphere(6.0f, slices, stacks);
	//auxSolidSphere(6.0f);
	
	// Restore the viewing transformation
	glPopMatrix();
	
	// Second Electron Orbit
	glPushMatrix();
	glRotatef(45.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(fElect1, 0.0f, 1.0f, 0.0f);
	glTranslatef(-70.0f, 0.0f, 0.0f);
	DrawSphere(6.0f, slices, stacks);
	//auxSolidSphere(6.0f);
	
	// Restore the viewing transformation
	glPopMatrix();

	// Third Electron Orbit
	glPushMatrix();
	glRotatef(-45.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(fElect1, 0.0f, 1.0f, 0.0f);
	glTranslatef(0.0f, 0.0f, 60.0f);
	DrawSphere(6.0f, slices, stacks);
	//auxSolidSphere(6.0f);

	glPopMatrix();

	// Increment the angle of revolution
	fElect1 += 3.0f;
	if (fElect1 > 360.0f)
		fElect1 = 0.0f;
}*/

/*void TheWorld_UIClientApp_GL::meshRenderProva1(void)
{
	static GLfloat v0[] = { -1.0f, -1.0f,  1.0f };
	static GLfloat v1[] = { 1.0f, -1.0f,  1.0f };
	static GLfloat v2[] = { 1.0f,  1.0f,  1.0f };
	static GLfloat v3[] = { -1.0f,  1.0f,  1.0f };
	static GLfloat v4[] = { -1.0f, -1.0f, -1.0f };
	static GLfloat v5[] = { 1.0f, -1.0f, -1.0f };
	static GLfloat v6[] = { 1.0f,  1.0f, -1.0f };
	static GLfloat v7[] = { -1.0f,  1.0f, -1.0f };
	static GLubyte red[] = { 255,   0,   0, 255 };
	static GLubyte green[] = { 0, 255,   0, 255 };
	static GLubyte blue[] = { 0,   0, 255, 255 };
	static GLubyte white[] = { 255, 255, 255, 255 };
	static GLubyte yellow[] = { 0, 255, 255, 255 };
	static GLubyte black[] = { 0,   0,   0, 255 };
	static GLubyte orange[] = { 255, 255,   0, 255 };
	static GLubyte purple[] = { 255,   0, 255,   0 };

	// Send our triangle data to the pipeline.
	glBegin(GL_TRIANGLES);

	glColor4ubv(red);
	glVertex3fv(v0);
	glColor4ubv(green);
	glVertex3fv(v1);
	glColor4ubv(blue);
	glVertex3fv(v2);

	glColor4ubv(red);
	glVertex3fv(v0);
	glColor4ubv(blue);
	glVertex3fv(v2);
	glColor4ubv(white);
	glVertex3fv(v3);

	glColor4ubv(green);
	glVertex3fv(v1);
	glColor4ubv(black);
	glVertex3fv(v5);
	glColor4ubv(orange);
	glVertex3fv(v6);

	glColor4ubv(green);
	glVertex3fv(v1);
	glColor4ubv(orange);
	glVertex3fv(v6);
	glColor4ubv(blue);
	glVertex3fv(v2);

	glColor4ubv(black);
	glVertex3fv(v5);
	glColor4ubv(yellow);
	glVertex3fv(v4);
	glColor4ubv(purple);
	glVertex3fv(v7);

	glColor4ubv(black);
	glVertex3fv(v5);
	glColor4ubv(purple);
	glVertex3fv(v7);
	glColor4ubv(orange);
	glVertex3fv(v6);

	glColor4ubv(yellow);
	glVertex3fv(v4);
	glColor4ubv(red);
	glVertex3fv(v0);
	glColor4ubv(white);
	glVertex3fv(v3);

	glColor4ubv(yellow);
	glVertex3fv(v4);
	glColor4ubv(white);
	glVertex3fv(v3);
	glColor4ubv(purple);
	glVertex3fv(v7);

	glColor4ubv(white);
	glVertex3fv(v3);
	glColor4ubv(blue);
	glVertex3fv(v2);
	glColor4ubv(orange);
	glVertex3fv(v6);

	glColor4ubv(white);
	glVertex3fv(v3);
	glColor4ubv(orange);
	glVertex3fv(v6);
	glColor4ubv(purple);
	glVertex3fv(v7);

	glColor4ubv(green);
	glVertex3fv(v1);
	glColor4ubv(red);
	glVertex3fv(v0);
	glColor4ubv(yellow);
	glVertex3fv(v4);

	glColor4ubv(green);
	glVertex3fv(v1);
	glColor4ubv(yellow);
	glVertex3fv(v4);
	glColor4ubv(black);
	glVertex3fv(v5);

	glEnd();
}*/

