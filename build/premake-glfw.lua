
Paths.Libs.glfw     = Paths.ExternalSrc .. "glfw/"
Paths.Libs.glfw_ext = Paths.ExternalSrc .. "glfw_ext/"

-- ------------------------------------------------------------------------------
Library.Include.glfw = function()

    externalincludedirs
    {
        Paths.Libs.glfw .. "include",
        Paths.Libs.glfw_ext .. "include",
    }

    defines 
    {
        "OPVD_PCH_GLFW"
    }

    filter "system:Windows"
        defines 
        {
            "_GLFW_WIN32",
            "_GLFW_USE_HYBRID_HPG",
        }
    filter {}

    filter "system:linux"
        defines 
        {
            "_GLFW_X11"
        }
    filter {}

    filter "system:macosx"
        defines 
        {
            "_GLFW_COCOA"
        }
    filter {}

end

-- ==============================================================================

project "glfw"

    ConfigureLibrary( "glfw", "C++" )

    Library.Include.glfw()

    files 
    {
        Paths.Libs.glfw .. "include/*.h",

        Paths.Libs.glfw_ext .. "**.*",
    }

    local common_headers = {
        "src/internal.h",
        "src/mappings.h",
        "src/null_platform.h",
        "src/null_joystick.h",
        "include/glfw/glfw3.h",
        "include/glfw/glfw3native.h",
    }
    local common_sources = {
        "src/context.c",
        "src/init.c",
        "src/input.c",
        "src/monitor.c",
        "src/vulkan.c",
        "src/window.c",
        "src/platform.c",
        "src/null_init.c",
        "src/null_monitor.c",
        "src/null_window.c",
        "src/null_joystick.c",
    }

    local windows_headers = {
        "src/win32_platform.h",
        "src/win32_joystick.h",
        "src/wgl_context.h",
        "src/egl_context.h",
        "src/osmesa_context.h",
    }
    local windows_sources = {
        "src/win32_init.c",
        "src/win32_joystick.c",
        "src/win32_monitor.c",
        "src/win32_module.c",
        "src/win32_time.c",
        "src/win32_thread.c",
        "src/win32_window.c",
        "src/wgl_context.c",
        "src/egl_context.c",
        "src/osmesa_context.c",
    }

    local linux_headers = {
        "src/x11_platform.h",
        "src/xkb_unicode.h",
        "src/posix_time.h",
        "src/posix_thread.h",
        "src/glx_context.h",
        "src/egl_context.h",
        "src/osmesa_context.h",
        "src/linux_joystick.h",
    }
    local linux_sources = {
        "src/x11_init.c",
        "src/x11_monitor.c",
        "src/x11_window.c",
        "src/xkb_unicode.c",
        "src/posix_time.c",
        "src/posix_thread.c",
        "src/glx_context.c",
        "src/egl_context.c",
        "src/osmesa_context.c",
        "src/linux_joystick.c",
    }

    local osx_headers = {
        "src/cocoa_platform.h",
        "src/cocoa_joystick.h",
        "src/posix_thread.h",
        "src/nsgl_context.h",
        "src/egl_context.h",
        "src/osmesa_context.h",
    }
    local osx_sources = {
        "src/cocoa_init.m",
        "src/cocoa_joystick.m",
        "src/cocoa_monitor.m",
        "src/cocoa_window.m",
        "src/cocoa_time.c",
        "src/posix_thread.c",
        "src/nsgl_context.m",
        "src/egl_context.c",
        "src/osmesa_context.c",
    }

    for k,v in pairs(common_headers) do 
        files       { Paths.Libs.glfw .. v }
    end
    for k,v in pairs(common_sources) do 
        files       { Paths.Libs.glfw .. v }
    end

    filter "system:Windows"
        for k,v in pairs(windows_headers) do 
            files       { Paths.Libs.glfw .. v }
        end
        for k,v in pairs(windows_sources) do 
            files       { Paths.Libs.glfw .. v }
        end
    filter {}

    filter "system:linux"
        for k,v in pairs(linux_headers) do 
            files       { Paths.Libs.glfw .. v }
        end
        for k,v in pairs(linux_sources) do 
            files       { Paths.Libs.glfw .. v }
        end
    filter {}

    filter "system:macosx"
        for k,v in pairs(osx_headers) do 
            files       { Paths.Libs.glfw .. v }
        end
        for k,v in pairs(osx_sources) do 
            files       { Paths.Libs.glfw .. v }
        end
    filter {}

