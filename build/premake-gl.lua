
-- ------------------------------------------------------------------------------
Library.Include.glm = function()

    externalincludedirs
    {
        Paths.ExternalSrc .. "glm",
    }
    files
    {
        Paths.ExternalSrc .. "glm/glm/*.hpp"
    }
end

-- ==============================================================================
-- glbinding
--
Paths.Libs.glbinding = Paths.ExternalSrc .. "glbinding/source/"

Library.Include.glbinding = function()

    externalincludedirs
    {
        Paths.Libs.glbinding .. "glbinding/include",
        Paths.Libs.glbinding .. "glbinding-aux/include",
        Paths.Libs.glbinding .. "3rdparty/KHR/include",

        Paths.ExternalSrc .. "glbinding_ext/include",
    }
    defines
    {
        "GLBINDING_STATIC_DEFINE",
        "GLBINDING_AUX_STATIC_DEFINE",
    }

end

project "glbinding"

    ConfigureLibrary( "glbinding", "C++" )

    Library.Include.glbinding()

    defines
    {
        "SYSTEM_WINDOWS"
    }
    files
    {
        Paths.Libs.glbinding .. "glbinding/source/**.cpp",
        Paths.Libs.glbinding .. "glbinding-aux/source/**.cpp",
    }

-- ==============================================================================
-- globjects
--
Paths.Libs.globjects = Paths.ExternalSrc .. "globjects/source/"

Library.Include.globjects = function()

    externalincludedirs {
        Paths.Libs.globjects .. "globjects/include",
        Paths.ExternalSrc .. "globjects_ext/include",
    }

    defines {
        "GLOBJECTS_STATIC_DEFINE",
        "GLOBJECTS_CHECK_GL_ERRORS",
    }

end

project "globjects"

    ConfigureLibrary( "globjects", "C++" )

    Library.Include.glbinding()
    Library.Include.glm()
    Library.Include.globjects()

    defines
    {
        "SYSTEM_WINDOWS"
    }
    files
    {
        Paths.Libs.globjects .. "globjects/source/**.cpp",
    }
