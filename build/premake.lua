DefaultLibraryInc = {}

-- ==============================================================================

-- stash the starting directory upfront to use as a reference root 
local initialDir = os.getcwd()
print( "Premake launch directory: " .. initialDir )

local rootBuildGenerationDir = "_generated"
local rootBinaryResultDir    = "_binaries"
local rootSourceDir          = "src"

function Src()
    return initialDir .. "/../" .. rootSourceDir .. "/"
end
function ExternalSrc()
    return initialDir .. "/../externals/"
end


function GetMacOSPackgesDir()
    return "/opt/homebrew/opt/"
end
function GetPrebuiltLibs_MacUniversal()
    return initialDir .. "/../libs/macos/universal-fat/"
end
function GetPrebuiltLibs_Win64()
    return initialDir .. "/../libs/windows/win64/"
end

-- ------------------------------------------------------------------------------
function GetBuildRootToken()
    if ( os.host() == "windows" ) then
        return "$(SolutionDir)"
    else
        return initialDir .. "/" .. rootBuildGenerationDir .. "/"
    end
end

-- artefact output directory base, differentiates via action so we can have
-- both vs2019/vs2022 outputs without trampling on each other
function GetArtefactBaseName()
    return "_%{_ACTION or ''}_artefact"
end

-- ------------------------------------------------------------------------------
function AddPCH( sourceFile, headerFilePath, headerFile )
    pchsource ( sourceFile )
    if ( os.host() == "windows" ) then
        pchheader ( headerFile )
    else
        pchheader ( headerFilePath .. headerFile )
    end
end

-- ==============================================================================

function SilenceMSVCSecurityWarnings()

    filter "system:Windows"
        defines {
            "_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES",
            "_CRT_NONSTDC_NO_WARNINGS",
            "_CRT_SECURE_NO_WARNINGS",
            "_WINSOCK_DEPRECATED_NO_WARNINGS",
        }
    filter {}

end

-- ------------------------------------------------------------------------------
function SetDefaultOutputDirectories(subgrp)

    targetdir   ( GetBuildRootToken() .. GetArtefactBaseName() .. "/%{cfg.system}/bin_%{cfg.shortname}/%{prj.name}/" )
    objdir      ( GetBuildRootToken() .. GetArtefactBaseName() .. "/%{cfg.system}/obj_%{cfg.shortname}/%{prj.name}/" )
    debugdir    ( "$(OutDir)" )

end


-- ------------------------------------------------------------------------------
function SetDefaultBuildConfiguration()

    language "C++"
    cppdialect "C++17"

    warnings "High"
    disablewarnings { "4100" }   -- unreferenced formal param

    SilenceMSVCSecurityWarnings()

    flags { "MultiProcessorCompile" }

    filter "configurations:Debug"
        defines   { "DEBUG" }
        symbols   "On"
    filter {}

    filter "configurations:Release"
        defines   { "NDEBUG" }
        flags     { "LinkTimeOptimization" }
        optimize  "Full"
    filter {}

    -- blanket static linking of all physx code we pull in
    defines {
        "PX_PHYSX_STATIC_LIB"
    }

end


-- ------------------------------------------------------------------------------
function SetDefaultAppConfiguration()

    -- windows
    filter "system:Windows"

        kind "ConsoleApp"

        defines {
            -- common thinning of windows.h include
            "WIN32_LEAN_AND_MEAN",
            "NOMINMAX",
            "NODRAWTEXT",
            "NOBITMAP",
            "NOMCX",
            "NOSERVICE",
            "NOHELP",
        }

    filter {}

end

-- ------------------------------------------------------------------------------
function AddCommonAppLink()

    filter "system:Windows"
        links {
            "ws2_32",
            "wsock32",
            "shlwapi",
            "version",
            "setupapi",
            "imm32",
        }
    filter {}

end



-- ==============================================================================

workspace ("OpenPVD")

    filename "%{_ACTION or ''}_OpenPVD"

    configurations  { "Debug", "Release" }
    platforms       { "x64" }

    location (rootBuildGenerationDir)

    -- windows generic
    filter "system:Windows"

        architecture "x64"
        defines {
            "WIN32",
            "_WINDOWS",
        }

    filter {}

-- ==============================================================================

group "libs"

-- slim build of core PVD and physx foundation source to let us use their types
project "pxpvd"

    filename "%{_ACTION or ''}_lib_pxpvd"

    kind "StaticLib"
    language "C++"

    SetDefaultBuildConfiguration()
    SetDefaultOutputDirectories()

    includedirs
    {
        ExternalSrc() .. "PhysX/pxshared/include/",
        ExternalSrc() .. "PhysX/physx/include",

        ExternalSrc() .. "PhysX/physx/source/filebuf/include",
        ExternalSrc() .. "PhysX/physx/source/foundation/include",
        ExternalSrc() .. "PhysX/physx/source/pvd/include",
    }

    files
    {
        ExternalSrc() .. "PhysX/physx/source/foundation/include/*.h",
        ExternalSrc() .. "PhysX/physx/source/foundation/src/*.cpp",
        ExternalSrc() .. "PhysX/physx/source/foundation/src/windows/*.cpp",

        ExternalSrc() .. "PhysX/physx/source/pvd/**.h",
        ExternalSrc() .. "PhysX/physx/source/pvd/**.cpp",
    }


-- ==============================================================================

DefaultLibraryInc["spdlog"] = function()
    defines
    {
        "SPDLOG_COMPILED_LIB"
    }

    externalincludedirs
    {
        ExternalSrc() .. "spdlog/include/",
    }
end

project "spdlog"

    filename "%{_ACTION or ''}_lib_spdlog"

    kind "StaticLib"
    language "C++"

    SetDefaultBuildConfiguration()
    SetDefaultOutputDirectories()

    DefaultLibraryInc["spdlog"]()

    files
    {
        ExternalSrc() .. "spdlog/src/*.cpp",
    }


-- ==============================================================================

include "premake-glfw.lua"
include "premake-imgui.lua"


-- ==============================================================================

group "app"

function ConfigureApp( appName )

    filename ( "%{_ACTION or ''}_app_" .. appName )
    targetname ( "opvd-" .. appName )

    SetDefaultBuildConfiguration()
    SetDefaultAppConfiguration()
    SetDefaultOutputDirectories()

    AddCommonAppLink()

    debugdir "$(SolutionDir)../../"

    for libName, libFn in pairs(DefaultLibraryInc) do
        libFn()
    end

    links 
    {
        "pxpvd",
        "spdlog",
    }

    includedirs
    {
        Src(),

        ExternalSrc() .. "CLI11/",
        ExternalSrc() .. "unordered_dense/include/",

        ExternalSrc() .. "PhysX/pxshared/include/",
        ExternalSrc() .. "PhysX/physx/include",

        ExternalSrc() .. "PhysX/physx/source/filebuf/include",
        ExternalSrc() .. "PhysX/physx/source/foundation/include",
        ExternalSrc() .. "PhysX/physx/source/pvd/include",
        ExternalSrc() .. "PhysX/physx/source/pvd/src",
    }

    files 
    {
        Src() .. "common/*.*",

        Src() .. "app." .. appName .. ".cpp",
        Src() .. "pch.cpp",
        Src() .. "pch.h",
    }

    -- enable a pch
    AddPCH( 
        "../src/pch.cpp",
        Src(),
        "pch.h" )

end

-- ==============================================================================

project "capture"

    ConfigureApp("capture")

-- ==============================================================================

project "filter"

    ConfigureApp("filter")

-- ==============================================================================

project "viewer"

    ConfigureApp("viewer")

    IncludeGLFW();
    IncludeIMGUI();

    links
    {
        "glfw",
        "imgui"
    }

    includedirs
    {
    }
    files
    {
    }


