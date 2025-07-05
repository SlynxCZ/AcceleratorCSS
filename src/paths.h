//
// Created by Michal Přikryl on 20.06.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once

#include <eiface.h>
#include <string>

namespace AcceleratorCSS {
    namespace paths {

        static std::string gameDirectory;

        inline std::string GameDirectory()
        {
            if (gameDirectory.empty())
            {
                CBufferStringGrowable<255> gamePath;
                g_pEngineServer->GetGameDir(gamePath);
                gameDirectory = std::string(gamePath.Get());
            }

            return gameDirectory;
        }

        inline std::string GetRootDirectory() { return GameDirectory() + "/addons/AcceleratorCSS"; }
        inline std::string GetLogsDirectory() { return GameDirectory() + "/addons/AcceleratorCSS/logs"; }
        inline std::string ConfigDirectory() { return GameDirectory() + "/addons/AcceleratorCSS/config.json"; }
        inline std::string GamedataDirectory() { return GameDirectory() + "/addons/AcceleratorCSS/gamedata.json"; }

    } // namespace paths
} // namespace AcceleratorCSS
