#Copyright 2021 Intel Corporation
#SPDX-License-Identifier: Apache-2.0

import sys, numpy
import pysg as sg
from pysg import Any, vec3f, Data

sg.init(sys.argv)

# specific rkcommon::math types needed for SG
pos = Any(vec3f(0.0, 0.0, 10.0))
dir = Any(vec3f(0.0, 0.0, -1.0))
up = Any(vec3f(0.0, 1.0, 0.0))

W = 1024
H = 768

aspect = sg.Any(W / H)

vertex = numpy.array([
   [-1.0, -1.0, 3.0],
   [-1.0, 1.0, 3.0],
   [1.0, -1.0, 3.0],
   [0.1, 0.1, 0.3]
], dtype=numpy.float32)

color = numpy.array([
    [0.9, 0.5, 0.5, 1.0],
    [0.8, 0.8, 0.8, 1.0],
    [0.8, 0.8, 0.8, 1.0],
    [0.5, 0.9, 0.5, 1.0]
], dtype=numpy.float32)

index = numpy.array([
    [0, 1, 2], [1, 2, 3]
], dtype=numpy.uint32)

mat = numpy.array([0], dtype=numpy.uint32)

frame = sg.Frame()
world = frame.child("world")

cam = frame.child("camera")
cam.createChild("aspect", "float", aspect)
cam.createChild("position", "vec3f", pos)
cam.createChild("direction", "vec3f", dir)
cam.createChild("up", "vec3f", up)

geom = world.createChild("mesh", "geometry_triangles")
geom.createChildData("vertex.position", Data(vertex))
geom.createChildData("vertex.color", Data(color))
geom.createChildData("index", Data(index))

lightsMan = sg.LightsManager()
lightsMan.addLight("ambientlight", "ambient")
lightsMan.updateWorld(world)

world.render()

frame.startNewFrame(bool(0))
frame.waitOnFrame()
frame.saveFrame("sgTutorial.png", 0)
