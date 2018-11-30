/*******************************************************
 * Copyright 2018 (C) St34lth4ng3l
 * 
 * Do the whatever you want with it, but i won't give you help with it,
 * you have to integrate it on your own, and setup your DiscordApp and the libs on your own.
 *
 * Remove the util's methods or replace them with your own ones
 *
 *******************************************************/
#pragma once

#include <discord_rpc.h>

class Discord
{
private:
	static const std::string DiscordAppID;
	static long LastUpdateTime;

public:
	static bool Connected;

public:
	static void Init();
	static void UpdateStatus();
	static void Terminate();
};

