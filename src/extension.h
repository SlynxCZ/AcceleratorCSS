//
// Created by Michal Přikryl on 12.06.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once

#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <sh_vector.h>
#include <iserver.h>

namespace acceleratorcss {
	class AcceleratorCSS_MM : public ISmmPlugin, public IMetamodListener {
	public:
		bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);

		bool Unload(char *error, size_t maxlen);

		void AllPluginsLoaded();

		const char *GetAuthor();

		const char *GetName();

		const char *GetDescription();

		const char *GetURL();

		const char *GetLicense();

		const char *GetVersion();

		const char *GetDate();

		const char *GetLogTag();

	private:
		void GameFrame(bool simulating, bool bFirstTick, bool bLastTick);

		void StartupServer(const GameSessionConfiguration_t &config, ISource2WorldSession *, const char *);
	};

	extern AcceleratorCSS_MM gPlugin;

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
}

PLUGIN_GLOBALVARS();
