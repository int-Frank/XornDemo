workspace "AdventuresIn2DSamples"
  architecture "x64"
  
  configurations
  {
    "Debug",
    "Release"
  }

  include("AdventuresIn2D/3rdParty/Core2DApp/premake-Core2DLib.lua")
  include("AdventuresIn2D/3rdParty/Core2DApp/3rdParty/DgLib/premake-proj-DgLib.lua")
  include("AdventuresIn2D/premake-AdventuresIn2D.lua")
  include("premake-Samples.lua")