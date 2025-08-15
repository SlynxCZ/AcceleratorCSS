-- AcceleratorCSS build script for xmake
local ACCELERATORCSS_VERSION = os.getenv("ACCELERATORCSS_VERSION") or "1.0-dev"

add_rules("mode.debug", "mode.release")

-- External packages via xmake-repo
add_requires("fmt")
add_requires("spdlog")
add_requires("breakpad")
add_requires("nlohmann_json")
add_requires("funchook")

-- Paths
local ROOT     = os.projectdir()
local SDK_PATH = os.getenv("HL2SDKCS2")
local MM_PATH = os.getenv("MMSOURCE112")

target("AcceleratorCSS")
set_kind("shared")
set_languages("cxx20")
set_symbols("hidden")

add_defines("stricmp=strcasecmp")
add_cxxflags("-include", "hl2sdk_compat.h")

-- Source files
add_files({
    path.join(SDK_PATH, "public/tier0/memoverride.cpp"),
    path.join(ROOT, "src", "extension.cpp"),
    path.join(SDK_PATH, "tier1/convar.cpp"),
    path.join(SDK_PATH, "tier1/generichash.cpp"),
    path.join(SDK_PATH, "tier1/keyvalues3.cpp"),
    path.join(SDK_PATH, "entity2/entityidentity.cpp"),
    path.join(SDK_PATH, "entity2/entitysystem.cpp"),
    path.join(SDK_PATH, "entity2/entitykeyvalues.cpp"),
    path.join(MM_PATH, "core/sourcehook/sourcehook.cpp"),
    path.join(MM_PATH, "core/sourcehook/sourcehook_impl_chookidman.cpp"),
    path.join(MM_PATH, "core/sourcehook/sourcehook_impl_chookmaninfo.cpp"),
    path.join(MM_PATH, "core/sourcehook/sourcehook_impl_cvfnptr.cpp"),
    path.join(MM_PATH, "core/sourcehook/sourcehook_impl_cproto.cpp"),
    path.join(ROOT, "src", "log.cpp"),
    path.join(ROOT, "protobufs", "generated", "**.pb.cc")
})

add_headerfiles(
        path.join(ROOT, "src", "**.h"),
        path.join(ROOT, "src", "**.hpp"),
        path.join(ROOT, "protobufs", "generated", "**.pb.h")
)

-- Link libraries from SDK & vendor
add_links({
    path.join(SDK_PATH, "lib", "linux64", "release", "libprotobuf.a"),
    path.join(SDK_PATH, "lib", "linux64", "libtier0.so"),
    path.join(SDK_PATH, "lib", "linux64", "tier1.a"),
    path.join(SDK_PATH, "lib", "linux64", "interfaces.a"),
    path.join(SDK_PATH, "lib", "linux64", "mathlib.a"),
    "funchook",
    "distorm"
})

add_linkdirs({
    path.join(ROOT, "vendor", "funchook", "lib", "Release")
})

add_cxxflags("-lstdc++", "-Wno-register")

add_defines({
    "_LINUX",
    "LINUX",
    "POSIX",
    "GNUC",
    "COMPILER_GCC",
    "PLATFORM_64BITS",
    "META_IS_SOURCE2",
    "_GLIBCXX_USE_CXX11_ABI=0"
})

-- Include dirs
add_includedirs({
    path.join(ROOT, "src"),
    path.join(ROOT, "protobufs", "generated"),
    path.join(ROOT, "vendor", "funchook", "include"),
    path.join(ROOT, "vendor", "breakpad", "src"),
    path.join(ROOT, "vendor", "fmt", "include"),
    path.join(ROOT, "vendor", "spdlog", "include"),
    path.join(ROOT, "vendor", "nlohmann"),
    path.join(ROOT, "vendor"),
    path.join(ROOT, "breakpad-config", "linux"),
    -- SDK
    SDK_PATH,
    path.join(SDK_PATH, "thirdparty", "protobuf-3.21.8", "src"),
    path.join(SDK_PATH, "common"),
    path.join(SDK_PATH, "game", "shared"),
    path.join(SDK_PATH, "game", "server"),
    path.join(SDK_PATH, "public"),
    path.join(SDK_PATH, "public", "engine"),
    path.join(SDK_PATH, "public", "mathlib"),
    path.join(SDK_PATH, "public", "tier0"),
    path.join(SDK_PATH, "public", "tier1"),
    path.join(SDK_PATH, "public", "entity2"),
    path.join(SDK_PATH, "public", "game", "server"),
    path.join(SDK_PATH, "public", "schemasystem"),
    -- Metamod
    path.join(MM_PATH, "core"),
    path.join(MM_PATH, "core", "sourcehook")
})

-- Link xmake packages
add_packages("fmt", "spdlog", "breakpad", "nlohmann_json", "funchook")
