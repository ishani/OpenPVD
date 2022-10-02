

-- ==============================================================================

Library.Include.freetype = function()

    filter "system:Windows"
        externalincludedirs
        {
            Paths.PrebuiltLibs.windows .. "freetype/include/",
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
            Paths.MacOSPackages .. "freetype/include/freetype2/",
        }
    filter {}

end

-- ==============================================================================
Library.Link.freetype  = function()

    -- windows has debug/release static builds
    filter "system:Windows"
        links ( "freetype.lib" )
        filter { "system:Windows", "configurations:Release" }
            libdirs ( Paths.PrebuiltLibs.windows .. "freetype/lib/release_static" )
        filter { "system:Windows", "configurations:Debug" }
            libdirs ( Paths.PrebuiltLibs.windows .. "freetype/lib/debug_static" )
    filter {}

    -- 
    filter "system:linux"
        links ( "freetype" )
    filter {}

    -- 
    filter "system:macosx"
        libdirs ( Paths.PrebuiltLibs.macosx )
        links
        {
            "freetype",
            "png",
            "bz2",
        }
    filter {}
    
end


Paths.Libs.imgui = Paths.ExternalSrc .. "imgui/"

-- ==============================================================================
Library.Include.imgui = function()

    Library.Include.freetype()

    includedirs
    {
        Paths.Libs.imgui .. "/",
        Paths.Libs.imgui .. "/backends/",
        Paths.Libs.imgui .. "/misc/freetype/",
        Paths.Libs.imgui .. "/misc/cpp/",
    }

end

-- ==============================================================================
project "imgui"

    ConfigureLibrary( "imgui", "C++" )

    Library.Include.glfw()
    Library.Include.imgui()

    defines 
    {
        "IMGUI_IMPL_OPENGL_LOADER_GLAD2",
    }
    files
    {
        Paths.Libs.imgui .. "/*.cpp",
        Paths.Libs.imgui .. "/*.h",
        Paths.Libs.imgui .. "/freetype/*.cpp",
        Paths.Libs.imgui .. "/freetype/*.h",
        Paths.Libs.imgui .. "/backends/imgui_impl_glfw.*",
        Paths.Libs.imgui .. "/backends/imgui_impl_opengl3.*",

        Paths.Libs.imgui .. "/misc/cpp/*.*",
    }
