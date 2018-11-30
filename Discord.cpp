/*******************************************************
 * Copyright 2018 (C) St34lth4ng3l
 * 
 * Do the whatever you want with it, but i won't give you help with it,
 * you have to integrate it on your own, and setup your DiscordApp and the libs on your own.
 *
 * Remove the util's methods or replace them with your own ones
 *
 *******************************************************/
#include "stdafx.h"
#include "Discord.h"
#include "AtumApplication.h"
#include "ShuttleChild.h"
#include "ClientParty.h"

// Set our AppID
const std::string Discord::DiscordAppID = "Insert here your Discord App ID, otherwise it won't work";

// Init all static variables
bool Discord::Connected = false;
long Discord::LastUpdateTime = timeGetTime();

//************************************
// Method:		public OnReady
// Created:		2018-05-21
// Author:		St0rmy
// Returns:		void -
// Parameter:	const DiscordUser * user -
// Description:	Get's called when discord client is ready
//************************************
void OnReady(const DiscordUser* user)
{
	DBGOUT(util::format("[Discord] Connected with Discord. User: %s(%s)", user->username, user->userId).c_str());
	Discord::Connected = true;
}

//************************************
// Method:		public OnError
// Created:		2018-05-21
// Author:		St0rmy
// Returns:		void -
// Parameter:	int errorCode -
// Parameter:	const char * errorMessage -
// Description:	Get's called when discord received an error
//************************************
void OnError(int errorCode, const char* errorMessage)
{
	DBGOUT(util::format("[Discord Error] Code: %u Message: %s", errorCode, errorMessage).c_str());
}

//************************************
// Method:		public OnDisconnect
// Created:		2018-05-21
// Author:		St0rmy
// Returns:		void -
// Parameter:	int errorCode -
// Parameter:	const char * errorMessage -
// Description:	Get's called when discord got disconnected
//************************************
void OnDisconnect(int errorCode, const char* errorMessage)
{
	DBGOUT(util::format("[Discord] Disconnected! Code: %u Message: %s", errorCode, errorMessage).c_str());
	Discord::Connected = false;
}

//************************************
// Method:		public Discord::Init
// Created:		2018-05-21
// Author:		St0rmy
// Returns:		void -
// Description:	Initializes the Discord client - Must be called before updating the presence
//************************************
void Discord::Init()
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	// Assign callbacks
	handlers.ready = OnReady;
	handlers.errored = OnError;
	handlers.disconnected = OnDisconnect;

	DBGOUT(util::format("[Discord] Initializing with AppID: %s", DiscordAppID.c_str()));

	// Discord_Initialize(const char* applicationId, DiscordEventHandlers* handlers, int autoRegister, const char* optionalSteamId)
	Discord_Initialize(DiscordAppID.c_str(), &handlers, 1, nullptr);
}

//************************************
// Method:		public Discord::UpdateStatus
// Created:		2018-05-21
// Author:		St0rmy
// Returns:		void -
// Description:	Updates the current presence according to character stats
//************************************
void Discord::UpdateStatus()
{
	// Get current time
	long currentTime = timeGetTime();

	// Check if 5 seconds have passed, discord has a limit of 1 API call every 15 seconds but we want to update every 5 seconds
	if (LastUpdateTime + 5000 > currentTime || (LastUpdateTime > 0 && currentTime < 0) /*Force update when currenttime overflows but lastupdatetime is still in valid range*/)
		return;

	// Reset the update time
	LastUpdateTime = currentTime;

	// Only update presence when discord is initialized
	if (Connected)
	{
		// Create a new object on the stack
		DiscordRichPresence discordPresence{};

		// Prevent access violations
		if (!g_pD3dApp || !g_pD3dApp->m_pShuttleChild)
			return;

		// Include all ingame states
		if (g_pD3dApp->m_dwGameState == GameState::_GAME
			|| g_pD3dApp->m_dwGameState == GameState::_CITY
			|| g_pD3dApp->m_dwGameState == GameState::_SHOP)
		{
			// Get player info
			CHARACTER Player = g_pShuttleChild->GetMyShuttleInfo();
			// Get party info
			CClientParty* Party = g_pD3dApp->m_pShuttleChild->m_pClientParty;

			// Get current map
			MAP_INFO* mapname = g_pDatabase->GetMapInfo(Player.MapChannelIndex.MapIndex);

			// Set the state "Map - Playmode (Solo/Party)"
			char stateBuf[128];
			sprintf_s(stateBuf, "%s%s", (mapname) ? util::remove_color(mapname->MapName).c_str() : "Unknown", (Party->GetPartyInfo().bPartyType != _NOPARTY) ? " - In Party" : " - Solo");
			discordPresence.state = stateBuf;

			// Set details "Name (Nation Gear:Level)"
			char detailBuf[128];
			sprintf_s(detailBuf, "%s (%s:%u)", util::remove_color(Player.CharacterName).c_str(), CAtumSJ::GetUnitKindString(Player.UnitKind), Player.Level);
			discordPresence.details = detailBuf;

			// Set nation images
			if (COMPARE_INFLUENCE(Player.InfluenceType, INFLUENCE_TYPE_VCN))
			{
				// Set BCU Icons and Text
				discordPresence.largeImageKey = "nation_bcu";
				discordPresence.largeImageText = "Bygeniou City United (BCU)";
			}
			else if (COMPARE_INFLUENCE(Player.InfluenceType, INFLUENCE_TYPE_ANI))
			{
				// Set ANI Icons and Text
				discordPresence.largeImageKey = "nation_ani";
				discordPresence.largeImageText = "Arlington National Influence (ANI)";
			}
			else
			{
				// Set Neutral Icons and Text
				discordPresence.largeImageKey = "nation_neutral";
				discordPresence.largeImageText = "FreeSKA";
			}

			// Only set party info when in party
			if (Party->GetPartyInfo().bPartyType != _NOPARTY)
			{
				// Current party size + 1 because this gear is not included
				discordPresence.partySize = Party->GetPartyMemberCount() + 1;
				// Max party size
				discordPresence.partyMax = SIZE_MAX_PARTY_MEMBER;
			}
		}
		else if (g_pD3dApp->m_dwGameState == GameState::_SELECT)
		{
			// Player is in character selection
			discordPresence.state = "In character selection";
		}
		else if (g_pD3dApp->m_dwGameState == GameState::_CREATE)
		{
			// Player is creating a new character
			discordPresence.state = "Creating new character";
		}

		// Send our information to discord
		Discord_UpdatePresence(&discordPresence);
	}

	// Run discord callbacks, so we can catch errors etc
#ifdef DISCORD_DISABLE_IO_THREAD
	Discord_UpdateConnection();
#endif
	Discord_RunCallbacks();
}


//************************************
// Method:		public Discord::Terminate
// Created:		2018-05-21
// Author:		St0rmy
// Returns:		void -
// Description:	Terminates the connection to discord
//************************************
void Discord::Terminate()
{
	Discord_Shutdown();
}