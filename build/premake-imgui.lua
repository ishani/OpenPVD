

-- ==============================================================================
function IncludeFreetype()

    filter "system:Windows"
    externalincludedirs
    {
        GetPrebuiltLibs_Win64() .. "freetype/include/",
    }
    filter {}

    filter "system:linux"
    externalincludedirs
    {
        "/usr/include/freetype2/",
    }
    filter {}

    filter "system:macosx"
    externalincludedirs
    {
        GetMacOSPackgesDir() .. "freetype/include/freetype2/",
    }
    filter {}
end

-- ==============================================================================
function LinkFreetype()

    -- windows has debug/release static builds
    filter "system:Windows"
        links ( "freetype.lib" )
        filter { "system:Windows", "configurations:Release" }
            libdirs ( GetPrebuiltLibs_Win64() .. "freetype/lib/release_static" )
        filter { "system:Windows", "configurations:Debug" }
            libdirs ( GetPrebuiltLibs_Win64() .. "freetype/lib/debug_static" )
    filter {}

    -- 
    filter "system:linux"
        links ( "freetype" )
    filter {}

    -- 
    filter "system:macosx"
        libdirs ( GetPrebuiltLibs_MacUniversal() )
        links
        {
            "freetype",
            "png",
            "bz2",
        }
    filter {}
end


function Root_IMGUI()
    return ExternalSrc() .. "imgui/"
end

-- ==============================================================================
function IncludeIMGUI()

    includedirs
    {
        Root_IMGUI() .. "/",
        Root_IMGUI() .. "/backends/",
        Root_IMGUI() .. "/misc/freetype/",
        Root_IMGUI() .. "/misc/cpp/",
    }
    IncludeFreetype()
end

-- ==============================================================================
project "imgui"

    kind "StaticLib"
    language "C++"

    filename "%{_ACTION or ''}_lib_imgui"

    SetDefaultBuildConfiguration()
    SetDefaultOutputDirectories()

    IncludeGLFW()
    IncludeIMGUI()

    defines 
    {
        "IMGUI_IMPL_OPENGL_LOADER_GLAD2",
    }
    files
    {
        Root_IMGUI() .. "/*.cpp",
        Root_IMGUI() .. "/*.h",
        Root_IMGUI() .. "/freetype/*.cpp",
        Root_IMGUI() .. "/freetype/*.h",
        Root_IMGUI() .. "/backends/imgui_impl_glfw.*",
        Root_IMGUI() .. "/backends/imgui_impl_opengl3.*",

        Root_IMGUI() .. "/misc/cpp/*.*",
    }
