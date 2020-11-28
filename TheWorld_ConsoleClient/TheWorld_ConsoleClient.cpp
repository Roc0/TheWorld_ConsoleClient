// TheWorld_ConsoleClient.cpp : Defines the entry point for the console application.
//

/*

	TODO: 
		* Definire in kbengine_demos_assets\res\spaces\TheWorld_ConsoleClient le informazioni riguardo le risorse spaziali (collision information etc.). "TheWorld_ConsoleClient" è è l'identiificativo passato al login.
		* Capire a cosa serve kbe_updateVolatile chiamato in SpaceWorld::frameRenderingQueued in Ogre Demo
		* Capire il significato di Entity, EntityComplex, EntitySimple, Space* in Ogre Demo

*/

#include "stdafx.h"

#include "SDL.h"
#include "SDL_opengl.h"
#include <GL/glu.h>
#include "Recast.h"
#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl.h"

#include "TheWorld_ConsoleClient.h"
#include <conio.h>
#include <conio.h>
#include <json/json.h>
#include <boost/thread/thread.hpp>
#include "PlayerEntity.h"
#include "OtherEntity.h"


// Default Account Name
std::string g_defaultAccountName("6529868038458114048");
//------

TheWorld_UIClientApp::TheWorld_UIClientApp() :
	events_()
{
	m_pIo = NULL;
	m_bDoSleepInMainLoop = true;
	m_iDisplayActions = 1;
	m_iSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
	m_iLogin = LOGIN_NOT_DONE;
	m_bShutDown = false;
	kbe_registerEventHandle(this);
	m_pSpaceWorld = new SpaceWorld;
	m_pPlayer = m_pTarget = m_pMouseTarget = NULL;
	m_bServerClosed = false;
	setRenderingMode(InitialMenu, true);
	m_bInitRenderingMode = true;
}

TheWorld_UIClientApp::~TheWorld_UIClientApp()
{
	kbe_deregisterEventHandle(this);
}

void TheWorld_UIClientApp::go(void)
{
	if (!setup())
		return;

	doMainLoop();

	reset();
}

bool TheWorld_UIClientApp::setup(void)
{
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

	float t = 0.0f;
	float timeAcc = 0.0f;
	Uint32 prevFrameTime = SDL_GetTicks();
	int mousePos[2] = { 0, 0 };
	int origMousePos[2] = { 0, 0 }; // Used to compute mouse movement totals across frames.

	float cameraEulers[] = { 45, -45 };
	float cameraPos[] = { 0, 0, 0 };
	float camr = 1000;
	float origCameraEulers[] = { 0, 0 }; // Used to compute rotational changes across frames.

	float moveFront = 0.0f, moveBack = 0.0f, moveLeft = 0.0f, moveRight = 0.0f, moveUp = 0.0f, moveDown = 0.0f;

	float scrollZoom = 0;
	bool rotate = false;
	bool movedDuringRotate = false;
	float rayStart[3];
	float rayEnd[3];
	bool mouseOverMenu = false;

	float markerPosition[3] = { 0, 0, 0 };
	bool markerPositionSet = false;

	// Fog.
	float fogColor[4] = { 0.32f, 0.31f, 0.30f, 1.0f };
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, camr * 0.1f);
	glFogf(GL_FOG_END, camr * 1.25f);
	glFogfv(GL_FOG_COLOR, fogColor);

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	return true;
}

void TheWorld_UIClientApp::reset(void)
{
	SDL_Quit();
}

void TheWorld_UIClientApp::doMainLoop(void)
{
	while (!m_bShutDown)
	{
		messagePump();

		if (getRenderingMode() == InitialMenu)
			manageInitialMenu();
		else
			manageGraphicRendering();

		if (m_bDoSleepInMainLoop)
			kbe_sleep(1);
	}
}
	
void TheWorld_UIClientApp::manageInitialMenu(void)
{
	if (m_iDisplayActions == 1 || m_bInitRenderingMode)
	{
		m_bInitRenderingMode = false;
		m_iDisplayActions = 2;
		printf("\n");
		printf("Actions: \n");
		printf("\n");
		printf("   l - Login\n");
		printf("   x - Reset\n");
		printf("\n");
		printf("   c - Create Avatar\n");
		printf("   r - Remove Avatar\n");
		printf("\n");
		printf("   d - Dump Status\n");
		printf("   m - Minidump\n");
		printf("\n");
		printf("   z - Display Menu\n");
		printf("\n");
		AVATARS::iterator iter = m_Avatars.begin();
		int idx = 0;
		for (; iter != m_Avatars.end(); iter++)
		{
			KBAvatar* pAvatar = iter->second.get();
			printf("   %d - Select Avatar %d - %s\n", idx + 1, (int)pAvatar->getAvatarID(), pAvatar->getAvatarName().c_str());
			idx++;
		}
		printf("\n");
		printf("   . - Quit\n");
	}

	if (kbhit())
	{
		int ch = getch();
		if (m_iSelectAvatarPending == SELECT_AVATAR_PENDING)
		{
			if (ch == '.' || ch == 27)
			{
			}
			else
			{
				int i = ch - 48 - 1;
				if (i >= 0 && i < m_Avatars.size())
				{
					doInitialMenuAction(ch);
				}
			}
			m_iSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
			m_iDisplayActions = 1;
		}
		else
		{
			if (ch == '.' || ch == 27) {
				m_bShutDown = true;
				m_bDoSleepInMainLoop = false;
				kbe_shutdown();
			}
			else
				doInitialMenuAction(ch);
		}
	}
}

void TheWorld_UIClientApp::manageGraphicRendering(void)
{
	if (m_bInitRenderingMode)
	{
		m_bInitRenderingMode = false;
		
		SDL_Renderer* renderer;
		int errorCode = SDL_CreateWindowAndRenderer(m_iWidth, m_iHeight, m_uiFlags, &m_window, &renderer);
		if (errorCode != 0 || !m_window || !renderer)
		{
			printf("Could not initialise SDL opengl\nError: %s\n", SDL_GetError());
			setRenderingMode(InitialMenu);
			return;
		}

		SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		m_gl_context = SDL_GL_CreateContext(m_window);
		SDL_GL_MakeCurrent(m_window, m_gl_context);
		SDL_GL_SetSwapInterval(1); // Enable vsync
	
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
	}

	ImGuiIO& io = *m_pIo;
	
	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT)
			setRenderingMode(InitialMenu);
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_window))
			setRenderingMode(InitialMenu);
	}

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame(m_window);
	ImGui::NewFrame();

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (m_bShow_demo_window)
		ImGui::ShowDemoWindow(&m_bShow_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
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

	// Rendering
	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(m_window);
	
	
	if (getRenderingMode() == InitialMenu)
	{
		// Cleanup
		ImGui_ImplOpenGL2_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		SDL_GL_DeleteContext(m_gl_context);
		SDL_DestroyWindow(m_window);
	}

}

void TheWorld_UIClientApp::setRenderingMode(_RenderingMode r, bool bForce)
{
	if (renderingMode != r || bForce)
	{
		renderingMode = r;
		m_bInitRenderingMode = true;
	}

}

TheWorld_UIClientApp::_RenderingMode TheWorld_UIClientApp::getRenderingMode(void)
{
	return renderingMode;
}

void TheWorld_UIClientApp::doInitialMenuAction(int ch)
{
	char str[256];

	if (m_iSelectAvatarPending == SELECT_AVATAR_PENDING)
	{
		//mSelectAvatarPending == SELECT_AVATAR_PENDING
		int iAvatar = ch - 48;
		KBAvatar* pAvatar = NULL;
		AVATARS::iterator iter = m_Avatars.begin();
		int idx = 1;
		for (; iter != m_Avatars.end(); iter++)
		{
			if (idx == iAvatar)
			{
				pAvatar = iter->second.get();
				break;
			}
			idx++;
		}
		if (pAvatar)
		{
			std::string avatarName = pAvatar->getAvatarName();
			bool b = kbengine_RemoveAvatar(avatarName);
			sprintf(str, "kbe_callEntityMethod: reqRemoveAvatar - Player ID: %d, Avatar name %s\n", (int)kbe_playerID(), avatarName.c_str());
			printf(str);	printf("\n");
		}
		m_iSelectAvatarPending = SELECT_AVATAR_NOT_PENDING;
		m_iDisplayActions = 1;
	}
		
	switch (ch)
	{
		case 's':
		{
			setRenderingMode(GraphicMode);
			break;
		}
		case 'x':
		{
			if (m_iLogin == LOGIN_STARTED)
			{
				printf("Login in corso!\n");
			}
			else
			{
				kbengine_Logout();
				m_iLogin = LOGIN_NOT_DONE;
				m_iDisplayActions = 1;
			}
			break;
		}
		case 'l':
		{
			if (m_iLogin == LOGIN_DONE)
			{
				printf("Login già effettuato!\n");
			}
			else if (m_iLogin == LOGIN_STARTED)
			{
				printf("Login in corso!\n");
			}
			else
			{
				std::string datas = "TheWorld_ConsoleClient";
				if (kbengine_Login(NULL, "123456", datas.data(), KBEngine::uint32(datas.size())))
					m_iLogin = LOGIN_STARTED;
				else
					printf("Login error!\n");
			}
			break;
		}
		case 'z':
		{
			m_iDisplayActions = 1;
			break;
		}
		case 'c':
		{
			if (m_iLogin == LOGIN_DONE)
			{
				KBEngine::uint64 uuid = kbe_genUUID64();
				std::stringstream ss;
				ss << uuid;
				std::string avatarName = ss.str();
				bool b = kbengine_CreateAvatar(avatarName);
				sprintf(str, "kbe_callEntityMethod: reqCreateAvatar - Player ID: %d", (int)kbe_playerID());
				printf(str);	printf("\n");
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
		case 'r':
		{
			if (m_iLogin == LOGIN_DONE)
			{
				m_iSelectAvatarPending = SELECT_AVATAR_PENDING;
				printf("\n");
				printf("Select Avatar\n");
				printf("\n");
				AVATARS::iterator iter = m_Avatars.begin();
				int idx = 0;
				for (; iter != m_Avatars.end(); iter++)
				{
					KBAvatar* pAvatar = iter->second.get();
					printf("   %d - Select Avatar %d - %s\n", idx + 1, (int)pAvatar->getAvatarID(), pAvatar->getAvatarName().c_str());
					idx++;
				}
				printf("\n");
				printf("   . - Return\n");
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
		case 'd':
		case 'm':
		{
			bool minidump = (ch == 'm' ? true : false);
			printf("\n");
			printf("*** %s *********************\n", (ch == 'd' ? "DUMP" : "MINIDUMP"));
			printf("\n");
			{
				ENTITIES::iterator iter = m_Entities.begin();
				int idx = 0;
				for (; iter != m_Entities.end(); iter++)
				{
					idx++;
					KBEntity* pEntity = iter->second.get();
					pEntity->dumpStatus(idx, minidump);
				}
			}
			printf("\n");
			m_pSpaceWorld->dumpStatus(minidump);
			printf("\n");
			{
				AVATARS::iterator iter = m_Avatars.begin();
				int idx = 0;
				for (; iter != m_Avatars.end(); iter++)
				{
					idx++;
					KBAvatar* pAvatar = iter->second.get();
					pAvatar->dumpStatus(idx, minidump);
				}
			}
			printf("\n");
			printf("PlayerID: %d\n", (int)kbe_playerID());
			printf("\n");
			printf("*** %s *********************\n", (ch == 'd' ? "DUMP" : "MINIDUMP"));
			printf("\n");
			/*if (minidump)
			{
				kbe_lock();
				char str[256];
				sprintf(str, "[\"Parametro_1\", \"Parametro_2\"]");
				kbe_callEntityMethod(kbe_playerID(), "ping", str);
				kbe_unlock();
			}*/
			break;
		}
		// '1' / '9'
		default:
		{
			if (m_iLogin == LOGIN_DONE)
			{
				int i = ch - 48 - 1;
				if (i >= 0 && i < m_Avatars.size())
				{
					KBAvatar* pAvatar = NULL;
					AVATARS::iterator iter = m_Avatars.begin();
					int idx = 0;
					for (; iter != m_Avatars.end(); iter++)
					{
						if (idx == i)
						{
							pAvatar = iter->second.get();
							break;
						}
						idx++;
					}
					if (pAvatar)
					{
						bool b = kbengine_SelectAvatarGame(pAvatar->getAvatarID());
						sprintf(str, "kbe_callEntityMethod: selectAvatarGame - Player ID: %d, Avatar DBID %ld\n", (int)kbe_playerID(), (long)pAvatar->getAvatarID());
						printf(str);	printf("\n");
					}
				}
			}
			else
				printf("Login non effettuato!\n");
			break;
		}
	}

	return;
}

void TheWorld_UIClientApp::messagePump(void)
{
	while (true)
	{
		m_KbeEventsMutex.lock();

		if (events_.empty())
		{
			m_KbeEventsMutex.unlock();
			break;
		}

		std::tr1::shared_ptr<const KBEngine::EventData> pEventData = events_.front();
		events_.pop();
		m_KbeEventsMutex.unlock();

		KBEngine::EventID id = pEventData->id;

		if (id == CLIENT_EVENT_SCRIPT)
		{
			kbe_lock();
		}

		onEvent(pEventData.get());

		if (id == CLIENT_EVENT_SCRIPT)
		{
			kbe_unlock();
		}

		m_bDoSleepInMainLoop = true;
	}
}


void TheWorld_UIClientApp::onEvent(const KBEngine::EventData* lpEventData)
{
	char str[256];

	switch (lpEventData->id)
	{
		case CLIENT_EVENT_ENTERWORLD:
		{
			const KBEngine::EventData_EnterWorld* pEventData_EnterWorld = static_cast<const KBEngine::EventData_EnterWorld*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData_EnterWorld->entityID;

			ENTITIES::iterator iter = m_Entities.find(eid);
			if (iter == m_Entities.end())
			{
				bool bPlayer = false;
				if (kbe_playerID() == eid)
					bPlayer = true;
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERWORLD - %s - EntityID: %d (NOT IN LIST), SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
					(int)pEventData_EnterWorld->entityID, (int)pEventData_EnterWorld->spaceID,
					pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll,
					pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z,
					pEventData_EnterWorld->speed, pEventData_EnterWorld->isOnGround, pEventData_EnterWorld->res.c_str());
				printMessage(str, true);
				break;
			}

			KBEntity* pEntity = iter->second.get();

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				m_pPlayer = pEntity;
			}

			pEntity->setDestPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
			pEntity->setPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
			pEntity->setDestDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
			pEntity->setDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
			pEntity->setMoveSpeed(pEventData_EnterWorld->speed);
			pEntity->setSpaceID(pEventData_EnterWorld->spaceID);
			pEntity->setIsOnGround(pEventData_EnterWorld->isOnGround);
			pEntity->setRes(pEventData_EnterWorld->res);

			pEntity->inWorld(true);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERWORLD - %s - EntityID: %d, SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
				(int)pEventData_EnterWorld->entityID, (int)pEventData_EnterWorld->spaceID,
				pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll,
				pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z,
				pEventData_EnterWorld->speed, pEventData_EnterWorld->isOnGround, pEventData_EnterWorld->res.c_str());
			printMessage(str, true);
		}
		break;
	
		case CLIENT_EVENT_LEAVEWORLD:
		{
			KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveWorld*>(lpEventData)->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				m_pPlayer = NULL;
			}

			if (m_pTarget && m_pTarget->id() == eid)
				m_pTarget = NULL;

			if (m_pMouseTarget && m_pMouseTarget->id() == eid)
				m_pMouseTarget = NULL;

			m_Entities.erase(eid);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVEWORLD - %s - EntityID %d\n", bPlayer ? "PLAYER" : "OTHER", (int)eid);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_ENTERSPACE:
		{
			const KBEngine::EventData_EnterSpace* pEventData_EnterSpace = static_cast<const KBEngine::EventData_EnterSpace*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData_EnterSpace->entityID;

			ENTITIES::iterator iter = m_Entities.find(eid);
			if (iter == m_Entities.end())
			{
				bool bPlayer = false;
				if (kbe_playerID() == eid)
					bPlayer = true;
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERSPACE - %s - EntityID: %d (NOT IN LIST), SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
					(int)pEventData_EnterSpace->entityID, (int)pEventData_EnterSpace->spaceID,
					pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll,
					pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z,
					pEventData_EnterSpace->speed, pEventData_EnterSpace->isOnGround, pEventData_EnterSpace->res.c_str());
				printMessage(str, true);
				break;
			}

			KBEntity* pEntity = iter->second.get();

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				m_pPlayer = pEntity;
			}

			pEntity->setDestPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
			pEntity->setPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
			pEntity->setDestDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
			pEntity->setDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
			pEntity->setMoveSpeed(pEventData_EnterSpace->speed);
			pEntity->setSpaceID(pEventData_EnterSpace->spaceID);
			pEntity->setIsOnGround(pEventData_EnterSpace->isOnGround);
			pEntity->setRes(pEventData_EnterSpace->res);

			pEntity->inWorld(true);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ENTERSPACE - %s - EntityID: %d, SpaceID: %d, Yaw/Pitch/Roll: %f/%f/%f, X/Y/Z: %f/%f/%f, Speed: %f, IsOnGround: %d, Res: %s\n", bPlayer ? "PLAYER" : "OTHER",
				(int)pEventData_EnterSpace->entityID, (int)pEventData_EnterSpace->spaceID,
				pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll,
				pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z,
				pEventData_EnterSpace->speed, pEventData_EnterSpace->isOnGround, pEventData_EnterSpace->res.c_str());
			printMessage(str, true);
		}
		break;

		case CLIENT_EVENT_LEAVESPACE:
		{
			KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveSpace*>(lpEventData)->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				m_pPlayer = NULL;
			}

			if (m_pTarget && m_pTarget->id() == eid)
				m_pTarget = NULL;

			if (m_pMouseTarget && m_pMouseTarget->id() == eid)
				m_pMouseTarget = NULL;

			m_Entities.erase(eid);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LEAVESPACE - %s - EntityID %d\n", bPlayer ? "PLAYER" : "OTHER", (int)eid);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_CREATEDENTITY:
		{
			const KBEngine::EventData_CreatedEntity* pEventData_createEntity = static_cast<const KBEngine::EventData_CreatedEntity*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData_createEntity->entityID;
			KBEntity* pEntity = NULL;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
				pEntity = new PlayerEntity(eid, m_pSpaceWorld);
				m_pPlayer = pEntity;
			}
			else
				pEntity = new OtherEntity(eid, m_pSpaceWorld);
			pEntity->setModelScale(pEventData_createEntity->modelScale);
			m_Entities[eid].reset(pEntity);
				
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_CREATEDENTITY - %s - EntityID: %d, ModelScale: %f\n", bPlayer ? "PLAYER" : "OTHER", (int)pEventData_createEntity->entityID, (float)pEventData_createEntity->modelScale);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_SUCCESS:
		{
			m_iLogin = LOGIN_DONE;
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_SUCCESS\n");
			printMessage(str);
		}
		break;
		
		case CLIENT_EVENT_LOGIN_FAILED:
		{
			m_iLogin = LOGIN_NOT_DONE;
			const KBEngine::EventData_LoginFailed* info = static_cast<const KBEngine::EventData_LoginFailed*>(lpEventData);

			if (info->failedcode == 20)
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Server is starting, please wait!\n");
			}
			else
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_FAILED - Login is failed (code=%u)!\n", info->failedcode);
			}

			printMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS:
		{
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_SUCCESS\n");
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_LOGIN_BASEAPP_FAILED:
		{
			const KBEngine::EventData_LoginBaseappFailed* info = static_cast<const KBEngine::EventData_LoginBaseappFailed*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LOGIN_BASEAPP_FAILED (code=%u)!\n", info->failedcode);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_SCRIPT:
		{
			const KBEngine::EventData_Script* peventdata = static_cast<const KBEngine::EventData_Script*>(lpEventData);
			
			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(peventdata->datas.c_str(), root))
			{
				assert(false);
			}
			
			if (peventdata->name == "update_avatars")
			{
				strcpy(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, peventdata->name: ");
				strcat(str, peventdata->name.c_str());
				strcat(str,"\n");
				printMessage(str);

				//if (mDisplayActions == 0)
					m_iDisplayActions = 1;

				m_Avatars.clear();
				
				Json::Value::Members mem = root.getMemberNames();
				for (auto iter = mem.begin(); iter != mem.end(); iter++)
				{
					Json::Value& val = root[*iter];
					std::string name = val[1].asString();
					KBEngine::DBID avatarDBID = val[Json::Value::UInt(0)].asUInt();
					KBAvatar *pAvatar = new KBAvatar(avatarDBID, name);
					m_Avatars[avatarDBID].reset(pAvatar);
					sprintf(str, "\t\tAvatar DBID: [%ld] , Avatar Name: [%s]\n", (long)avatarDBID, name.c_str());
					printMessage(str, true);
				}
			}
			else
			{
				KBEngine::ENTITY_ID eid = root[Json::Value::UInt(0)].asInt();
				ENTITIES::iterator iter = m_Entities.find(eid);
				KBEntity* pEntity = NULL;

				if (iter != m_Entities.end())
					pEntity = iter->second.get();

				if (peventdata->name == "set_name")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					std::string name = root[1].asString();
					pEntity->setName(name);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, name: %s\n", (int)eid, peventdata->name.c_str(), name.c_str());
					printMessage(str);
				}
				else if (peventdata->name == "set_modelScale")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					uint32_t scale = root[1].asUInt();
					pEntity->setModelScale(scale / (float)100.0);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, modelScale: %d\n", (int)eid, peventdata->name.c_str(), (int)scale);
					printMessage(str);
				}
				else if (peventdata->name == "set_modelID")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					uint32_t modelID = root[1].asUInt();
					pEntity->setModelID(modelID);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, modelID: %d\n", (int)eid, peventdata->name.c_str(), (int)modelID);
					printMessage(str);
				}
				else if (peventdata->name == "set_state")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					int32_t state = root[1].asInt();
					pEntity->setState(state);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, state: %d\n", (int)eid, peventdata->name.c_str(), (int)state);
					printMessage(str);
				}
				else if (peventdata->name == "set_HP_Max")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					int32_t v = root[1].asInt();
					pEntity->setHPMax(v);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, HPMax: %d\n", (int)eid, peventdata->name.c_str(), (int)v);
					printMessage(str);
				}
				else if (peventdata->name == "set_MP_Max")
				{
					if (pEntity == NULL)
					{
						sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT (ENTITY NOT FOUND), EntityID: %d, peventdata->name: %s\n", (int)eid, peventdata->name.c_str());
						printMessage(str);
						break;
					}

					int32_t v = root[1].asInt();
					pEntity->setMPMax(v);

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, MPMax: %d\n", (int)eid, peventdata->name.c_str(), (int)v);
					printMessage(str);
				}
				else if (peventdata->name == "recvDamage")
				{
					KBEngine::ENTITY_ID attackerID = root[1].asInt();
					uint32_t skillID = root[2].asUInt();
					uint32_t damageType = root[3].asUInt();
					uint32_t damage = root[4].asUInt();

					ENTITIES::iterator iter = m_Entities.find(attackerID);

					KBEntity* attacker = NULL;
					KBEntity* receiver = pEntity;
					KBEngine::ENTITY_ID eidAttacker = 0;

					if (iter != m_Entities.end())
					{
						attacker = iter->second.get();
					}

					if (attacker)
					{
						attacker->attack(receiver, skillID, damageType, damage);
					}

					if (receiver)
					{
						receiver->recvDamage(attacker, skillID, damageType, damage);
					}

					sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT, EntityID: %d, peventdata->name: %s, EID Attacker: %d, skillId: %d, damageType: %d, damage: %d\n", (int)eid, peventdata->name.c_str(), (int)eidAttacker, (int)skillID, (int)damageType, (int)damage);
					printMessage(str);
				}
			}
			// TODO
		}
		break;

		case CLIENT_EVENT_POSITION_CHANGED:
		{
			const KBEngine::EventData_PositionChanged* pEventData = static_cast<const KBEngine::EventData_PositionChanged*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
			}

			ENTITIES::iterator iter = m_Entities.find(eid);
			if (iter == m_Entities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d (NOT IN LIST), X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
				printMessage(str);
				break;
			}

			iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_CHANGED - %s - EntityID %d, X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_DIRECTION_CHANGED:
		{
			const KBEngine::EventData_DirectionChanged* pEventData = static_cast<const KBEngine::EventData_DirectionChanged*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
			}

			ENTITIES::iterator iter = m_Entities.find(eid);
			if (iter == m_Entities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d (NOT IN LIST), Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
				printMessage(str);
				break;
			}

			iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
			
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_CHANGED - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_MOVESPEED_CHANGED:
		{
			const KBEngine::EventData_MoveSpeedChanged* pEventData = static_cast<const KBEngine::EventData_MoveSpeedChanged*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
			}

			ENTITIES::iterator iter = m_Entities.find(eid);
			if (iter == m_Entities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d (NOT IN LIST), MoveSpeed %f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
				printMessage(str);
				break;
			}

			iter->second->setMoveSpeed(pEventData->speed);
			
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_MOVESPEED_CHANGED - %s - EntityID %d, MoveSpeed %f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->speed);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_SERVER_CLOSED:
		{
			m_bServerClosed = true;
			m_bShutDown = true;
			m_iLogin = LOGIN_NOT_DONE;
			m_iDisplayActions = 0;
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_SERVER_CLOSED\n");
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_POSITION_FORCE:
		{
			const KBEngine::EventData_PositionForce* pEventData = static_cast<const KBEngine::EventData_PositionForce*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
			}

			ENTITIES::iterator iter = m_Entities.find(eid);
			if (iter == m_Entities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d (NOT IN LIST), X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
				printMessage(str);
				break;
			}

			iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);
			iter->second->setDestPosition(pEventData->x, pEventData->y, pEventData->z);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_POSITION_FORCE - %s - EntityID %d, X/Y/Z: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->x, pEventData->y, pEventData->z);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_DIRECTION_FORCE:
		{
			const KBEngine::EventData_DirectionForce* pEventData = static_cast<const KBEngine::EventData_DirectionForce*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			bool bPlayer = false;
			if (kbe_playerID() == eid)
			{
				bPlayer = true;
			}

			ENTITIES::iterator iter = m_Entities.find(eid);
			if (iter == m_Entities.end())
			{
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d (NOT IN LIST), Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
				printMessage(str);
				break;
			}

			iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
			iter->second->setDestDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_DIRECTION_FORCE - %s - EntityID %d, Yaw/Pitch/Roll: %f/%f/%f\n", bPlayer ? "PLAYER" : "OTHER", (int)eid, pEventData->yaw, pEventData->pitch, pEventData->roll);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_ADDSPACEGEOMAPPING:
			{
				const KBEngine::EventData_AddSpaceGEOMapping* pEventData = static_cast<const KBEngine::EventData_AddSpaceGEOMapping*>(lpEventData);
				KBEngine::SPACE_ID spaceID = pEventData->spaceID;
				m_pSpaceWorld->addSpace(spaceID, pEventData->respath);
				sprintf(str, "KBE Event received ==> CLIENT_EVENT_ADDSPACEGEOMAPPING, SpaceID %d, ResPath %s\n", (int)spaceID, pEventData->respath.c_str());
				printMessage(str, true);
			}
			break;

		case CLIENT_EVENT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_VersionNotMatch* info = static_cast<const KBEngine::EventData_VersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_VERSION_NOT_MATCH - VerInfo=%s not match(server:%s)\n", info->verInfo.c_str(), info->serVerInfo.c_str());
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_ON_KICKED:
		{
			const KBEngine::EventData_onKicked* info = static_cast<const KBEngine::EventData_onKicked*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_ON_KICKED (code=%u)!\n", info->failedcode);
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_LAST_ACCOUNT_INFO:
		{
			const KBEngine::EventData_LastAccountInfo* info = static_cast<const KBEngine::EventData_LastAccountInfo*>(lpEventData);
			m_sAccountName = info->name;

			sprintf(str, "KBE Event received ==> CLIENT_EVENT_LAST_ACCOUNT_INFO - Last account name: %s\n", info->name.c_str());
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH:
		{
			const KBEngine::EventData_ScriptVersionNotMatch* info = static_cast<const KBEngine::EventData_ScriptVersionNotMatch*>(lpEventData);
			sprintf(str, "KBE Event received ==> CLIENT_EVENT_SCRIPT_VERSION_NOT_MATCH - ScriptVerInfo=%s not match(server:%s)\n", info->verInfo.c_str(), info->serVerInfo.c_str());
			printMessage(str);
		}
		break;

		case CLIENT_EVENT_UNKNOWN:
		default:
		{
			//kbe_fireEvent("reset", NULL);
			//kbe_fireEvent("relive", NULL);
			strcpy(str, "KBE Event received ==> CLIENT_EVENT_UNKNOWN\n");
			printMessage(str);
		}
		break;
	};
}

void TheWorld_UIClientApp::kbengine_onEvent(const KBEngine::EventData* lpEventData)
{
	KBEngine::EventData* peventdata = KBEngine::copyKBEngineEvent(lpEventData);

	if (peventdata)
	{
		boost::mutex::scoped_lock lock(m_KbeEventsMutex);
		events_.push(std::tr1::shared_ptr<const KBEngine::EventData>(peventdata));
		m_bDoSleepInMainLoop = false;
	}
}

bool TheWorld_UIClientApp::kbengine_Login(const char* accountname, const char* passwd, const char* datas, KBEngine::uint32 datasize,	const char* ip, KBEngine::uint32 port)
{
	std::string accountName;
	if (accountname == NULL)
	{
		m_sAccountName = kbe_getLastAccountName();
		if (m_sAccountName.size() == 0)
		{
			/*KBEngine::uint64 uuid = kbe_genUUID64();
			std::stringstream ss;
			ss << uuid;
			g_accountName = ss.str();
			//g_accountName = KBEngine::StringConv::val2str(KBEngine::genUUID64());*/
			m_sAccountName = g_defaultAccountName;
		}
		accountName = m_sAccountName;
	}
	else
		accountName = accountname;

	char str[256];
	sprintf(str, "KBE Login - AccountName: [%s], Password: [%s], Client Name: [%s]\n", accountName.c_str(), passwd, datas);
	printMessage(str);
	if (kbe_login(accountName.c_str(), passwd, datas, datasize, ip, port))
	{
		return true;
	}
	else
		return false;
}

bool TheWorld_UIClientApp::kbengine_Logout()
{
	printMessage("KBE Logout\n");
	kbengine_Reset();
	m_Avatars.clear();

	return true;
}
	
bool TheWorld_UIClientApp::kbengine_CreateAvatar(std::string avatarName)
{
	char str[256];
	sprintf(str, "KBE Create Avatar - Name: [%s]\n", avatarName);
	printMessage(str);

	kbe_lock();
	sprintf(str, "[1, \"kbeconsole_%s\"]", avatarName.c_str());
	kbe_callEntityMethod(kbe_playerID(), "reqCreateAvatar", str);
	kbe_unlock();

	return true;
}

bool TheWorld_UIClientApp::kbengine_RemoveAvatar(std::string avatarName)
{
	char str[256];
	sprintf(str, "KBE Remove Avatar - Name: [%s]\n", avatarName);
	printMessage(str);

	kbe_lock();
	kbe_callEntityMethod(kbe_playerID(), "reqRemoveAvatar", avatarName.c_str());
	kbe_unlock();

	return true;
}

bool TheWorld_UIClientApp::kbengine_SelectAvatarGame(KBEngine::DBID avatarDBID)
{
	char str[256];
	sprintf(str, "KBE Select Avatar Game - DBID: [%d]\n", (int)avatarDBID);
	printMessage(str);

	kbe_lock();
	kbe_callEntityMethod(kbe_playerID(), "selectAvatarGame", KBEngine::StringConv::val2str(avatarDBID).c_str());
	kbe_unlock();

	return true;
}

void TheWorld_UIClientApp::kbengine_Reset(void)
{
	kbe_reset();
}

void TheWorld_UIClientApp::printMessage(char *message, bool interline, bool console, KBEMessageType t)
{
	if (message[strlen(message) - 1] != '\n')
		kbe_printMessage("Next message without trailing New Line", t);
	kbe_printMessage(message, t);
	if (console)
	{
		printf(message);
		if (interline)
			printf("\n");
	}
}

int main(int argc, char *argv[])
{
#ifdef _DEBUG
	std::string kbenginedll_name = "TheWorld_ClientDll_d.dll";
#else
	std::string kbenginedll_name = "TheWorld_ClientDll.dll";
#endif

	g_hKBEngineDll = LoadLibrary(kbenginedll_name.c_str());
	if (g_hKBEngineDll == NULL)
	{
		std::string kbenginedll_name_failed = "load " + kbenginedll_name + " is failed!";
		MessageBox(NULL, kbenginedll_name_failed.c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return 0;
	}

	if (!kbe_init())
	{
		MessageBox(NULL, "kbengine_init() is failed!", "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		return 0;
	}
	
	TheWorld_UIClientApp *app = new TheWorld_UIClientApp();

	// TODO: login
	try
	{
		app->go();
	}
	catch (...)
	{
		MessageBox(NULL, "An exception has occured!", "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	
	delete app;
	kbe_destroy();

	FreeLibrary(g_hKBEngineDll);

	return 0;
}

