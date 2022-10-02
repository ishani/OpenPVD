-- slim build of core PVD and physx foundation source to let us use their types

Library.Include.pxpvd = function()
    
    defines
    {
        "SPDLOG_COMPILED_LIB"
    }

    includedirs
    {
        Paths.ExternalSrc .. "PhysX/pxshared/include/",
        Paths.ExternalSrc .. "PhysX/physx/include",

        Paths.ExternalSrc .. "PhysX/physx/source/filebuf/include",
        Paths.ExternalSrc .. "PhysX/physx/source/foundation/include",
        Paths.ExternalSrc .. "PhysX/physx/source/pvd/include",
        Paths.ExternalSrc .. "PhysX/physx/source/pvd/src",
    }
end

project "pxpvd"

    ConfigureLibrary( "pxpvd", "C++" )

    Library.Include.pxpvd()

    files
    {
        Paths.ExternalSrc .. "PhysX/physx/source/foundation/include/*.h",
        Paths.ExternalSrc .. "PhysX/physx/source/foundation/src/*.cpp",
        Paths.ExternalSrc .. "PhysX/physx/source/foundation/src/windows/*.cpp",

        Paths.ExternalSrc .. "PhysX/physx/source/pvd/**.h",
        Paths.ExternalSrc .. "PhysX/physx/source/pvd/**.cpp",
    }
