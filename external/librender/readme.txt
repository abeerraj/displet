librender
Andreas Geiger and Chaohui Wang 2014

This software allows OpenGl offscreen rendering directly from within
Matlab to yield depth maps and object maps from 3D meshes specified
by vertices and triangular faces.

To get started, compile the MATLAB wrapper using make.m

Note that freeglut3 is required for compilation, other glut implementations
might or might not work. In Ubuntu, simply install freeglut via:
sudo apt-get install freeglut3-dev libglew-dev

Next, please run demo_render.m for a demonstration. Vertices and faces
are specified as 3-column matrices. The other parameters are documented
in demo_render.m. This zip file includes a car object model. For
converting novel Google Sketchup models in obj format to triangle meshes
please modify and use demo_convertModel.m
