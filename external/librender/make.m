dbclear all;
mex('renderMeshMex.cpp','-lGLEW','-lglut','-lGL','-lGLU');
disp('done!');
