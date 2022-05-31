import subprocess
import sys
import os

sys.path.insert(0, 'AdventuresIn2D\\3rdParty\\Core2DApp\\3rdParty\\DgLib\\3rdParty\\BuildScripts')

import DgBuild

samplesDir = "Samples"

def CreateSamplesPremakeFile(path):
    sampleNames = os.listdir(path)

    with open("premake-Samples.lua", "w") as file:
        for name in sampleNames:
            file.write(f"include(\"Samples/{name}/premake-{name}.lua\")\n")

def CreatePluginDirs(path):
    if not os.path.exists("AdventuresIn2D/Plugins"):
        os.makedirs("AdventuresIn2D/Plugins")
    
    sampleNames = os.listdir(path)
    for name in sampleNames:
        pluginPath = f"AdventuresIn2D/Plugins/{name}"
        if not os.path.exists(pluginPath):
            os.makedirs(pluginPath)

CreateSamplesPremakeFile(samplesDir)
CreatePluginDirs(samplesDir)

DgBuild.Make_vpaths("AdventuresIn2D/3rdParty/Core2DApp/3rdParty/DgLib/src/", "AdventuresIn2D/3rdParty/Core2DApp/3rdParty/DgLib/DgLib_vpaths.lua")
subprocess.call("AdventuresIn2D/3rdParty/Core2DApp/3rdParty/DgLib/3rdParty/premake/premake5.exe vs2022")
