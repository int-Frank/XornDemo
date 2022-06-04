# XornDemo
This project demoonstrates hwo to pull together the various Xorm modules into a single solution.

## Submodules
Ensure to update submodules when you clone the repo.

## Python
You must have Python 3 installed to run the project generation tool.

## CGAL and Boost
You will need to install CGAL and Boost. Use the vcpkg library manager as outlined in the CGAL documentation [here](https://doc.cgal.org/latest/Manual/windows.html#title0) for both. CGAL must be installed one level up in the directory structure to this project:

    c:/dev/AdventuresIn2D
    c:/dev/vcpkg

# Build
Run `GenerateSolution.py` to generate a VS2022 solution.
