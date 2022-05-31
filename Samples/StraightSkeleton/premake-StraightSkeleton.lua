-- Be sure to define the path to the vcpkg package directory as vcpkgPackageDir

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
    vcpkgPackageDir .. "/boost-utility_x64-windows/include",
    vcpkgPackageDir .. "/boost-foreach_x64-windows/include",
    vcpkgPackageDir .. "/boost-array_x64-windows/include",
    vcpkgPackageDir .. "/boost-throw-exception_x64-windows/include",
    vcpkgPackageDir .. "/boost-range_x64-windows/include",
    vcpkgPackageDir .. "/cgal_x64-windows/include",
    vcpkgPackageDir .. "/boost-optional_x64-windows/include",
    vcpkgPackageDir .. "/boost-tuple_x64-windows/include",
    vcpkgPackageDir .. "/boost-numeric-conversion_x64-windows/include",
    vcpkgPackageDir .. "/boost-core_x64-windows/include",
    vcpkgPackageDir .. "/boost-random_x64-windows/include",
    vcpkgPackageDir .. "/boost-property-map_x64-windows/include",
    vcpkgPackageDir .. "/boost-lexical-cast_x64-windows/include",
    vcpkgPackageDir .. "/boost-any_x64-windows/include",
    vcpkgPackageDir .. "/boost-integer_x64-windows/include",
    vcpkgPackageDir .. "/boost-io_x64-windows/include",
    vcpkgPackageDir .. "/boost-math_x64-windows/include",
    vcpkgPackageDir .. "/boost-preprocessor_x64-windows/include",
    vcpkgPackageDir .. "/boost-assert_x64-windows/include",
    vcpkgPackageDir .. "/boost-multiprecision_x64-windows/include",
    vcpkgPackageDir .. "/boost-predef_x64-windows/include",
    vcpkgPackageDir .. "/boost-iterator_x64-windows/include",
    vcpkgPackageDir .. "/boost-type-index_x64-windows/include",
    vcpkgPackageDir .. "/boost-container_x64-windows/include",
    vcpkgPackageDir .. "/gmp_x64-windows/include",
    vcpkgPackageDir .. "/boost-graph_x64-windows/include",
    vcpkgPackageDir .. "/boost-move_x64-windows/include",
    vcpkgPackageDir .. "/boost-unordered_x64-windows/include",
    vcpkgPackageDir .. "/boost-container-hash_x64-windows/include",
    vcpkgPackageDir .. "/boost-variant_x64-windows/include",
    vcpkgPackageDir .. "/boost-algorithm_x64-windows/include",
    vcpkgPackageDir .. "/boost-concept-check_x64-windows/include",
    vcpkgPackageDir .. "/boost-smart-ptr_x64-windows/include",
    vcpkgPackageDir .. "/boost-config_x64-windows/include",
    vcpkgPackageDir .. "/boost-mpl_x64-windows/include",
    vcpkgPackageDir .. "/boost-type-traits_x64-windows/include",
    vcpkgPackageDir .. "/mpfr_x64-windows/include",
    vcpkgPackageDir .. "/boost-static-assert_x64-windows/include",
    vcpkgPackageDir .. "/boost-detail_x64-windows/include",
    "src",
    "%{wks.location}/XornCore/src",
    "%{wks.location}/DgLib/src"
  }
  
  links
  {
    vcpkgPackageDir .. "/gmp_x64-windows/lib/gmp.lib",
    vcpkgPackageDir .. "/mpfr_x64-windows/lib/mpfr.lib",
    "DgLib",
	"XornCOre"
  }
  
  postbuildcommands 
  {
    "{COPY} %{wks.location}/build/%{prj.name}-%{cfg.buildcfg}/StraightSkeleton.dll %{wks.location}/XornApp/Plugins/StraightSkeleton",
    "{COPY} " .. vcpkgPackageDir .. "/gmp_x64-windows/bin/gmp-10.dll %{wks.location}/XornApp/Plugins/StraightSkeleton",
    "{COPY} " .. vcpkgPackageDir .. "/mpfr_x64-windows/bin/mpfr-6.dll %{wks.location}/XornApp/Plugins/StraightSkeleton"
  }

  filter "configurations:Debug"
    runtime "Debug"
    symbols "on"

  filter "configurations:Release"
    runtime "Release"
    optimize "on"