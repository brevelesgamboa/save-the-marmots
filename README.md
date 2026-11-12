# CSC-4710 Final Project
The plan is to build a low-poly, PS1/PS2-style 3D space driving game where the
player drives a vehicle which is similar to a honda fit across a procedurally generated moon
terrain featuring mini skyscraper cities and robot factories. The player can drive through the
streets to run over waves of evil robots for points, dodge falling meteors, and use jump pads
with boost to launch onto skyscraper rooftops for bonus objectives (saving the dogs). The
project will include a space skybox, a third-person chase camera, standard forward lighting on
textured low-poly models, and bounding-volume collision detection for the buildings, rooftops,
and robots.

# Build and run

Run these commands from the project folder:

```sh
cmake -S . -B build
cmake --build build
./build/P3Base resources
```

# Controls

Move the mouse: look around (0.1 degree per pixel); move up to look up.
The cursor is captured so rotation continues beyond the window edges.
Esc: close the window and release the cursor.
Q / E: move the point light left / right.
M: toggle the car paint between light and dark blue.


The vehicle model was made using blender.
Space Texture
https://opengameart.org/content/seamless-space-backgrounds
