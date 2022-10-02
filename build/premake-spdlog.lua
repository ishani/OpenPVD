
Library.Include.spdlog = function()
    defines
    {
        "SPDLOG_COMPILED_LIB"
    }

    externalincludedirs
    {
        Paths.ExternalSrc .. "spdlog/include/",
    }
end

project "spdlog"

    ConfigureLibrary( "spdlog", "C++" )

    Library.Include.spdlog()

    files
    {
        Paths.ExternalSrc .. "spdlog/src/*.cpp",
    }
