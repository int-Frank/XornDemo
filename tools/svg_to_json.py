# This file takes an svg and converts it to an obj for use with the XornApp.

import xml.etree.ElementTree as ET

#------------------------------------------------------------
# Setup
#------------------------------------------------------------

filePath = 'C:/Users/frank/Documents/Shadow-tests.svg'
outputPath = 'C:/Users/frank/Documents/Shadow-tests.obj'
flipVertical = True

#------------------------------------------------------------
# Implementation
#------------------------------------------------------------

pathTag = '{http://www.w3.org/2000/svg}path'
lineTag = 'd'
vertical = 1
if (flipVertical):
    vertical = -1

xml = ET.parse(filePath)
polygons = []

# Read in all the polygons
for ele in xml.iter(pathTag):

    # paths are in the form 'M x0 y1 L x1 y1 L x2 y2...'
    pathElements = ele.attrib[lineTag][2:].split('L')
    polygon = []
    for s in pathElements:
        xy = s.strip().split(' ')
        x = float(xy[0])
        y = vertical * float(xy[1])
        polygon.append((x, y))
    if polygon[0][0] == polygon[-1][0] and polygon[0][1] == polygon[-1][1]:
        polygon.pop()
    polygons.append(polygon)

f = open(outputPath, "w")

# Output all the vertices
for polygon in polygons:
    for v in polygon:
        f.write(f"v {v[0]} {v[1]}\n")

# Output all the lines
vertIndex = 1
for polygon in polygons:
    s = "l "
    for v in polygon:
        s = s + f" {vertIndex}"
        vertIndex += 1
    f.write(s + '\n')

f.close()
