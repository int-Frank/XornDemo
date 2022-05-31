import subprocess
import sys
import os

sys.path.insert(0, 'DgLib\\3rdParty\\BuildScripts')

import DgBuild

samplesDir = "Samples"

def CreateSamplesPremakeFile(path):
    sampleNames = os.listdir(path)

    with open("premake-Samples.lua", "w") as file:
        for name in sampleNames:
            file.write(f"include(\"Samples/{name}/premake-{name}.lua\")\n")

def CreatePluginDirs(path):
    if not os.path.exists("XornApp/Plugins"):
        os.makedirs("XornApp/Plugins")
    
    sampleNames = os.listdir(path)
    for name in sampleNames:
        pluginPath = f"XornApp/Plugins/{name}"
        if not os.path.exists(pluginPath):
            os.makedirs(pluginPath)

CreateSamplesPremakeFile(samplesDir)
CreatePluginDirs(samplesDir)

DgBuild.Make_vpaths("DgLib/src/", "DgLib/DgLib_vpaths.lua")
subprocess.call("DgLib/3rdParty/premake/premake5.exe vs2022")
