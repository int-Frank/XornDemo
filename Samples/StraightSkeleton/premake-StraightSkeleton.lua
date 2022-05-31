project "StraightSkeleton"
  location ""
  kind "SharedLib"
  targetdir ("%{wks.location}/build/%{prj.name}-%{cfg.buildcfg}")
  objdir ("%{wks.location}/build/intermediate/%{prj.name}-%{cfg.buildcfg}")
  systemversion "latest"
  language "C++"
  cppdialect "C++17"
    
  files 
  {
    "src/**.h",
    "src/**.cpp",
	"StraightSkeleton.dll.manifest"
  }
    
  includedirs
  {
    "%{wks.location}/../vcpkg/packages/boost-utility_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-foreach_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-array_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-throw-exception_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-range_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/cgal_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-optional_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-tuple_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-numeric-conversion_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-core_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-random_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-property-map_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-lexical-cast_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-any_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-integer_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-io_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-math_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-preprocessor_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-assert_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-multiprecision_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-predef_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-iterator_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-type-index_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-container_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/gmp_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-graph_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-move_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-unordered_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-container-hash_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-variant_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-algorithm_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-concept-check_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-smart-ptr_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-config_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-mpl_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-type-traits_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/mpfr_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-static-assert_x64-windows/include",
    "%{wks.location}/../vcpkg/packages/boost-detail_x64-windows/include",
    "src",
    "%{wks.location}/AdventuresIn2D/3rdParty/Core2DApp/src",
    "%{wks.location}/AdventuresIn2D/3rdParty/Core2DApp/3rdParty/DgLib/src"
  }
  
  links
  {
    "%{wks.location}/../vcpkg/packages/gmp_x64-windows/lib/gmp.lib",
    "%{wks.location}/../vcpkg/packages/mpfr_x64-windows/lib/mpfr.lib",
    "DgLib",
	"Core2DLib"
  }
  
  postbuildcommands 
  {
    "{COPY} %{wks.location}/build/%{prj.name}-%{cfg.buildcfg}/StraightSkeleton.dll %{wks.location}/AdventuresIn2D/Plugins/StraightSkeleton",
    "{COPY} %{wks.location}/../vcpkg/packages/gmp_x64-windows/bin/gmp-10.dll %{wks.location}/AdventuresIn2D/Plugins/StraightSkeleton",
    "{COPY} %{wks.location}/../vcpkg/packages/mpfr_x64-windows/bin/mpfr-6.dll %{wks.location}/AdventuresIn2D/Plugins/StraightSkeleton"
  }

  filter "configurations:Debug"
    runtime "Debug"
    symbols "on"

  filter "configurations:Release"
    runtime "Release"
    optimize "on"