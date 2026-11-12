Lab 9: pitch and yaw camera control

A space map with a honda fit, moon rock, jump pad, white astronaut dummy, and dog.
Blinn-Phong lighting and a separate shader for the ground.
New materials include glossy blue paint, dull gray rock, and orange pad paint.
The white point light starts at (-2, 2, 2).
The camera eye stays fixed at (0, 2, -3), among the meshes, initially facing the car.
Mouse movement updates pitch and yaw. Pitch is limited to -80 to +80 degrees;
yaw can rotate continuously. The view uses glm::lookAt(eye, eye + direction, up),
with a unit direction computed from the pitch and yaw angles in radians.

Build and run

Run these commands from the Lab9 project folder:

```sh
cmake -S . -B build
cmake --build build
./build/P3Base resources
```

Controls

Move the mouse: look around (0.1 degree per pixel); move up to look up.
The cursor is captured so rotation continues beyond the window edges.
Esc: close the window and release the cursor.
Q / E: move the point light left / right.
M: toggle the car paint between light and dark blue.

Mesh without normals

two OBJ meshes without normals:

moonRockNoNormals.obj: the rock at (-7, 1.1, -3), left of the car.
dogNoNormals.obj: the dog at (1, 0, 4.2), beside the dummy.

The rock was made from the starter sphere; the dog uses the starter dog with
normal references removed. The honda fit was made using blender.
Space Texture
https://opengameart.org/content/seamless-space-backgrounds
