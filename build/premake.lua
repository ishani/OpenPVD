-- ==============================================================================
-- OpenPVD build script for Premake 5 (https://premake.github.io/)
-- ==============================================================================

-- declare a table of useful paths, all rooted from the location of this build script
Paths = {}
Paths.Libs = {}

-- stash the starting directory upfront to use as a reference root 
Paths.InitialDir = os.getcwd()
print( "Premake launch directory: " .. Paths.InitialDir )

Paths.Src           = Paths.InitialDir .. "/../src/"
Paths.ExternalSrc   = Paths.InitialDir .. "/../externals/"
Paths.MacOSPackages = "/opt/homebrew/opt/"

Paths.PrebuiltLibs = {}
Paths.PrebuiltLibs.macosx   = Paths.InitialDir .. "/../libs/macos/universal-fat/"
Paths.PrebuiltLibs.windows  = Paths.InitialDir .. "/../libs/windows/win64/"

-- name of where we keep all generated build machinery and resulting executables etc
local rootBuildGenerationDir = "_generated"


Library = {}
Library.Include = {}
Library.Link = {}

-- ==============================================================================
-- declare a function to configure universal output directories, separating by 
-- action (eg. vs2019/2022/etc), target system (macos/windows/etc),
-- configuration (release, debug etc) and project name
--
function GetBuildRootToken()
    if ( os.target() == "windows" ) then
        return "$(SolutionDir)"
    else
        return Paths.InitialDir .. "/" .. rootBuildGenerationDir .. "/"
    end
end

function ApplyDefaultOutputDirectories()

    -- artefact output directory base, differentiates via action so we can have
    -- both vs2019/vs2022 outputs without trampling on each other
    local artefactBaseName = "_%{_ACTION or ''}_artefact"

    targetdir   ( GetBuildRootToken() .. artefactBaseName .. "/%{cfg.system}/bin_%{cfg.shortname}/%{prj.name}/" )
    objdir      ( GetBuildRootToken() .. artefactBaseName .. "/%{cfg.system}/obj_%{cfg.shortname}/%{prj.name}/" )
    debugdir    ( "$(OutDir)" )

end

-- ------------------------------------------------------------------------------
function ApplyDefaultBuildConfiguration()

    language "C++"
    cppdialect "C++17"

    warnings "High"
    disablewarnings { 
        "4100",                     -- unreferenced formal param
        "4201",                     -- non-standard unnamed struct
    }

    filter "system:Windows"
        -- silence all MSVC security whining
        defines {
            "_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES",
            "_CRT_NONSTDC_NO_WARNINGS",
            "_CRT_SECURE_NO_WARNINGS",
            "_WINSOCK_DEPRECATED_NO_WARNINGS",
        }
    filter {}

    flags {
        "MultiProcessorCompile"     -- /MP on MSVC to locally distribute compile
    }

    filter "configurations:Debug"
        defines   { "DEBUG" }
        symbols   "On"
    filter {}

    filter "configurations:Release"
        defines   { "NDEBUG" }
        flags     { "LinkTimeOptimization" }
        optimize  "Full"
    filter {}

    -- ensure blanket static linking of all physx code we pull in
    defines {
        "PX_PHYSX_STATIC_LIB",
    }

end

function ApplyWindowsDefines()

    filter "system:Windows"
        -- common thinning of windows.h include
        defines {
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
function ApplyApplicationConfiguration()

    ApplyWindowsDefines()

    -- windows
    filter "system:Windows"

        kind "ConsoleApp"

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

-- ------------------------------------------------------------------------------
-- precompiled headers in MSVC don't want the fully qualified path to the header,
-- whereas make/xcode targets seem to
function AddPCH( inSourceFile, inHeaderFilePath, inHeaderFile )
    pchsource ( inSourceFile )
    if ( os.target() == "windows" ) then
        pchheader ( inHeaderFile )
    else
        pchheader ( inHeaderFilePath .. inHeaderFile )
    end
end

DefaultLibraryInc = {}


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

function ConfigureLibrary( inLibName, inLang )

    filename ( "%{_ACTION or ''}_lib_" .. inLibName )

    kind "StaticLib"
    language ( inLang )

    ApplyDefaultBuildConfiguration()
    ApplyDefaultOutputDirectories()

end

-- ==============================================================================

group "libs"

include "premake-spdlog.lua"
include "premake-physx.lua"

include "premake-gl.lua"
include "premake-glfw.lua"
include "premake-imgui.lua"

-- ==============================================================================

group "app-common"

project "opvd"

    ConfigureLibrary( "opvd", "C++" )
    ApplyWindowsDefines()

    Library.Include.pxpvd()
    Library.Include.spdlog()

    includedirs
    {
        Paths.Src,

        Paths.ExternalSrc .. "CLI11/",
        Paths.ExternalSrc .. "unordered_dense/include/",
    }
    files 
    {
        Paths.Src .. "common/*.*",
    }

-- ==============================================================================

group "app"

function ConfigureApp( appName )

    filename ( "%{_ACTION or ''}_app_" .. appName )
    targetname ( "opvd-" .. appName )

    ApplyDefaultOutputDirectories()
    ApplyDefaultBuildConfiguration()
    ApplyApplicationConfiguration()


    debugdir "$(SolutionDir)../../"

    Library.Include.pxpvd()
    Library.Include.spdlog()

    links 
    {
        "pxpvd",
        "spdlog",
        "opvd",
    }

    includedirs
    {
        Paths.Src,

        Paths.ExternalSrc .. "CLI11/",
        Paths.ExternalSrc .. "unordered_dense/include/",
    }

    files 
    {
        Paths.Src .. "common/*.h",

        Paths.Src .. appName .. "/*.cpp",
        Paths.Src .. "app." .. appName .. ".cpp",

        Paths.Src .. "pch.cpp",
        Paths.Src .. "pch.h",
    }

    AddPCH( 
        "../src/pch.cpp",
        Paths.Src,
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

    Library.Include.glfw()
    Library.Include.imgui()

    Library.Include.glbinding()
    Library.Include.globjects()
    Library.Include.glm()

    links
    {
        "glbinding",
        "globjects",

        "glfw",
        "imgui"
    }

    includedirs
    {
    }
    files
    {
    }


