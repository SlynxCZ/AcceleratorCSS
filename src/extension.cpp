//
// Created by Michal Přikryl on 12.06.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#include "extension.h"
#include "CMiniDumpComment.hpp"
#include "log.h"

#include <funchook.h>
#include <entitysystem.h>
#include <entity2/entitysystem.h>

#include <csignal>
#include <ctime>
#include <deque>
#include <dirent.h>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#if defined WIN32
#include <corecrt_io.h>
#else
#include "client/linux/handler/exception_handler.h"
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"
#include "common/linux/http_upload.h"
#endif

#include "paths.h"
#include "common/path_helper.h"
#include "common/using_std_string.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/stack_frame.h"
#include "processor/simple_symbol_supplier.h"
#include "processor/stackwalk_common.h"
#include "processor/pathname_stripper.h"

constexpr size_t kMaxCallbackTrace = 5;

struct CallbackTraceEntry {
    std::string name;
    std::string profile;
    std::string callerStack;
};

std::deque<CallbackTraceEntry> g_CallbackTraceBuffer;
std::mutex g_CallbackTraceMutex;

namespace fs = std::filesystem;

static std::string lastMap;
char crashMap[256];
char crashGamePath[512];
char crashCommandLine[1024];
char dumpStoragePath[512];

google_breakpad::ExceptionHandler *exceptionHandler = nullptr;
CMiniDumpComment g_MiniDumpComment(95000);

void (*SignalHandler)(int, siginfo_t *, void *);

const int kExceptionSignals[] = {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS};
const int kNumHandledSignals = std::size(kExceptionSignals);

bool g_pluginRegistered = false;
funchook_t *g_funchook = nullptr;

using RegisterCallbackTraceFn = void (*)(const char *name, const char *profile, const char *callerStack);
RegisterCallbackTraceFn original_RegisterCallbackTrace = nullptr;

void hooked_RegisterCallbackTrace(const char *name, const char *profile, const char *callerStack) {
    const std::string sName = name ? name : "";
    const std::string sProfile = profile ? profile : "";
    const std::string sCaller = callerStack ? callerStack : "";

    try {
        if (acceleratorcss::g_Config.contains("ProfileExcludeFilters")) {
            const auto &filters = acceleratorcss::g_Config["ProfileExcludeFilters"];
            bool matched = false;
            for (const auto &f: filters) {
                if (sProfile.find(f.get<std::string>()) != std::string::npos) {
                    matched = true;
                    break;
                }
            }

            if (matched)
                return;
        }
    } catch (...) {
        ACC_CORE_WARN("- [ Profile exclude filter failed. ] -");
    } {
        std::lock_guard lock(g_CallbackTraceMutex);
        g_CallbackTraceBuffer.push_back({sName, sProfile, sCaller});
        if (g_CallbackTraceBuffer.size() > kMaxCallbackTrace)
            g_CallbackTraceBuffer.pop_front();
    }

    if (original_RegisterCallbackTrace)
        original_RegisterCallbackTrace(name, profile, callerStack);
}

bool HookRegisterCallbackTrace() {
    void *handle = dlopen("counterstrikesharp.so", RTLD_NOW | RTLD_NOLOAD);
    if (!handle) {
        ACC_CORE_ERROR("- [ Failed to dlopen counterstrikesharp.so: {} ] -", dlerror());
        return false;
    }

    void *symbol = dlsym(handle, "RegisterCallbackTrace");
    if (!symbol) {
        ACC_CORE_ERROR("- [ dlsym failed to find RegisterCallbackTrace: {} ] -", dlerror());
        return false;
    }

    ACC_CORE_INFO("- [ Resolved RegisterCallbackTrace at {} ] -", fmt::ptr(symbol));
    original_RegisterCallbackTrace = reinterpret_cast<RegisterCallbackTraceFn>(symbol);

    funchook_t *hook = funchook_create();
    if (!hook) {
        ACC_CORE_ERROR("- [ Failed to create funchook. ] -");
        return false;
    }

    if (funchook_prepare(hook, (void **) &original_RegisterCallbackTrace, (void *) hooked_RegisterCallbackTrace) != 0 ||
        funchook_install(hook, 0) != 0) {
        ACC_CORE_ERROR("- [ Failed to install hook on RegisterCallbackTrace. ] -");
        return false;
    }

    ACC_CORE_INFO("- [ Hook installed on RegisterCallbackTrace successfully. ] -");

    return true;
}

static bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor, void *context, bool succeeded) {
    ACC_CORE_CRITICAL("- [ Crash detected! Writing custom crash log... ] -");

    my_strlcpy(dumpStoragePath, descriptor.path(), sizeof(dumpStoragePath));
    my_strlcat(dumpStoragePath, ".txt", sizeof(dumpStoragePath));

    std::ofstream dumpFile(dumpStoragePath, std::ios::out | std::ios::trunc);
    if (!dumpFile.is_open()) {
        ACC_CORE_ERROR("- [ Failed to open crash log file: {} ] -", dumpStoragePath);
        return false;
    }

    dumpFile << "-------- CONFIG BEGIN --------\n";
    dumpFile << "Map=" << crashMap << "\n";
    dumpFile << "GamePath=" << crashGamePath << "\n";
    dumpFile << "CommandLine=" << crashCommandLine << "\n";
    dumpFile << "-------- CONFIG END --------\n\n";

    LoggingSystem_GetLogCapture(&g_MiniDumpComment, false);
    const char *pszConsoleHistory = g_MiniDumpComment.GetStartPointer();

    if (pszConsoleHistory[0]) {
        dumpFile << "-------- CONSOLE HISTORY BEGIN --------\n";
        dumpFile << pszConsoleHistory;
        dumpFile << "-------- CONSOLE HISTORY END --------\n\n";
    }

    dumpFile << "-------- CALLBACK TRACE BEGIN -> NEWEST CALLBACK IS FIRST --------\n"; {
        std::lock_guard lock(g_CallbackTraceMutex);
        for (auto it = g_CallbackTraceBuffer.rbegin(); it != g_CallbackTraceBuffer.rend(); ++it) {
            dumpFile << "Name: " << it->name << "\n";
            dumpFile << "Profile: " << it->profile << "\n";
            dumpFile << "CallerStack:\n" << it->callerStack << "\n";
            dumpFile << "------------------------\n";
        }
    }

    dumpFile << "-------- CALLBACK TRACE END --------\n";

    dumpFile.close();

    ACC_CORE_INFO("Custom crash log written to: {}", dumpStoragePath);
    return true;
}

CGameEntitySystem *GameEntitySystem() { return nullptr; }

class GameSessionConfiguration_t {
};

PLUGIN_EXPOSE(AcceleratorCSS_MM, acceleratorcss::gPlugin);

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&,
                   ISource2WorldSession*, const char*);

namespace acceleratorcss {
    AcceleratorCSS_MM gPlugin;

    bool AcceleratorCSS_MM::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
        PLUGIN_SAVEVARS();

        GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
        GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkServerService, INetworkServerService,
                            NETWORKSERVERSERVICE_INTERFACE_VERSION);
        GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer, IVEngineServer, INTERFACEVERSION_VENGINESERVER);

        strncpy(crashGamePath, ismm->GetBaseDir(), sizeof(crashGamePath) - 1);
        ismm->Format(dumpStoragePath, sizeof(dumpStoragePath), "%s/addons/AcceleratorCSS/logs", ismm->GetBaseDir());

        struct stat st = {0};
        if (stat(dumpStoragePath, &st) == -1) {
            if (mkdir(dumpStoragePath, 0777) == -1) {
                ACC_CORE_ERROR("- [ Failed to parse config: {} ] -", error);
                g_pluginRegistered = false;
                return false;
            }
        } else
            chmod(dumpStoragePath, 0777);

        google_breakpad::MinidumpDescriptor descriptor(dumpStoragePath);
        exceptionHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, dumpCallback, NULL, true, -1);

        struct sigaction oact;
        sigaction(SIGSEGV, NULL, &oact);
        SignalHandler = oact.sa_sigaction;

        SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &AcceleratorCSS_MM::GameFrame), true);
        SH_ADD_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService,
                    SH_MEMBER(this, &AcceleratorCSS_MM::StartupServer), true);

        strncpy(crashCommandLine, CommandLine()->GetCmdLine(), sizeof(crashCommandLine) - 1);

        if (late)
            StartupServer({}, nullptr, g_pNetworkServerService->GetIGameServer()->GetMapName());

        Log::Init();
        g_SMAPI->AddListener(this, this);

        try {
            std::ifstream configFile(AcceleratorCSS::paths::ConfigDirectory());
            if (configFile.is_open()) {
                configFile >> g_Config;
                ACC_CORE_INFO("- [ Config loaded: {} ] -", AcceleratorCSS::paths::ConfigDirectory());
            } else {
                ACC_CORE_WARN("- [ Could not open config: {} ] -", AcceleratorCSS::paths::ConfigDirectory());
                g_pluginRegistered = false;
            }
        } catch (const std::exception &e) {
            ACC_CORE_ERROR("- [ Failed to parse config: {} ] -", e.what());
            g_pluginRegistered = false;
        }

        ACC_CORE_INFO("- [ MM plugin loaded. ] -");

        return true;
    }

    bool AcceleratorCSS_MM::Unload(char *error, size_t maxlen) {
        Log::Close();
        g_pluginRegistered = false;

        if (g_funchook) {
            funchook_destroy(g_funchook);
            g_funchook = nullptr;
        }

        SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &AcceleratorCSS_MM::GameFrame),
                       true);
        SH_REMOVE_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService,
                       SH_MEMBER(this, &AcceleratorCSS_MM::StartupServer), true);

        delete exceptionHandler;

        ACC_CORE_INFO("- [ MM plugin unloaded. ] -");

        return true;
    }

    void AcceleratorCSS_MM::AllPluginsLoaded() {
        if (!HookRegisterCallbackTrace()) g_pluginRegistered = false;

        g_pluginRegistered = true;

        std::thread([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            if (g_pluginRegistered)
                ACC_CORE_INFO("- [ MM plugin is active and linked. ] -");
            else
                ACC_CORE_ERROR("- [ MM plugin did not register itself. ] -");
        }).detach();
    }

    void AcceleratorCSS_MM::GameFrame(bool simulating, bool bFirstTick, bool bLastTick) {
        bool weHaveBeenFuckedOver = false;
        struct sigaction oact;

        const char* currentMap = g_pNetworkServerService->GetIGameServer()->GetMapName();

        if (currentMap && *currentMap && lastMap != currentMap) {
            strncpy(crashMap, currentMap, sizeof(crashMap) - 1);
            lastMap = currentMap;

            ACC_CORE_INFO("- [ Detected map change: %s ] -", currentMap);
        }

        for (int i = 0; i < kNumHandledSignals; ++i) {
            sigaction(kExceptionSignals[i], NULL, &oact);

            if (oact.sa_sigaction != SignalHandler) {
                weHaveBeenFuckedOver = true;
                break;
            }
        }

        if (!weHaveBeenFuckedOver)
            return;

        struct sigaction act;
        memset(&act, 0, sizeof(act));
        sigemptyset(&act.sa_mask);

        for (int i = 0; i < kNumHandledSignals; ++i)
            sigaddset(&act.sa_mask, kExceptionSignals[i]);

        act.sa_sigaction = SignalHandler;
        act.sa_flags = SA_ONSTACK | SA_SIGINFO;

        for (int i = 0; i < kNumHandledSignals; ++i)
            sigaction(kExceptionSignals[i], &act, NULL);
    }

    void AcceleratorCSS_MM::StartupServer(const GameSessionConfiguration_t &config, ISource2WorldSession *,
                                          const char *pszMapName) {
        strncpy(crashMap, pszMapName, sizeof(crashMap) - 1);
    }

    const char *AcceleratorCSS_MM::GetAuthor() { return "Slynx"; }
    const char *AcceleratorCSS_MM::GetName() { return "AcceleratorCSS"; }
    const char *AcceleratorCSS_MM::GetDescription() { return "Local crash handler for C# plugins"; }
    const char *AcceleratorCSS_MM::GetURL() { return "https://funplay.pro/"; }
    const char *AcceleratorCSS_MM::GetLicense() { return "GPLv3"; }
    const char *AcceleratorCSS_MM::GetVersion() { return "1.0.0"; }
    const char *AcceleratorCSS_MM::GetDate() { return __DATE__; }
    const char *AcceleratorCSS_MM::GetLogTag() { return "ACC"; }
}
