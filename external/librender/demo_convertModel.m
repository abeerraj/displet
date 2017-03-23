clear; close all; dbstop error;
disp('==========================');

% load mesh
mesh = loadSketchupModel('porsche_911.obj',3000);
mesh.vertices(:,1) = -mesh.vertices(:,1);
mesh.vertices(:,2) = -mesh.vertices(:,2);

% visualize mesh
figure;
patch('vertices',mesh.vertices,'faces',mesh.faces,'facecolor','r');
axis equal; xlabel('x'); ylabel('y'); zlabel('z');

% save mesh
save('porsche_911.mat','mesh');

% done
disp('done!');