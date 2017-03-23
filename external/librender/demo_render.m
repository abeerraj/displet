clear all; close all; dbstop error;
disp('======================');

% load object mesh
load('porsche_911.mat','mesh');

% parameters
intrinsics = [250 250 160 120]; % intrinsic camera parameters (fu,fv,cu,cv)
imgsize = [320 240];            % image size (w,h)

% get vertices, triangular faces and colors
colors = [];
vertices = mesh.vertices;
faces = mesh.faces;
if isfield(mesh,'vertices_color')
  colors = mesh.vertices_color*255;
end
vNum = size(vertices, 1);

% no colors in mesh => assign colors according to vertex position
colors = 255*(vertices-ones(vNum,1)*min(vertices))./ ...
             (ones(vNum,1)*(max(vertices)-min(vertices)));

% create figure
h.f = figure('Position',[100 100 3*320 2*240]);
h.a1 = axes('Position',[0.0 0.5 1/3 0.5]);
h.a2 = axes('Position',[1/3 0.5 1/3 0.5]);
h.a3 = axes('Position',[2/3 0.5 1/3 0.5]);
h.a4 = axes('Position',[0.0 0.0 1/3 0.5]);
h.a5 = axes('Position',[1/3 0.0 1/3 0.5]);
h.a6 = axes('Position',[2/3 0.0 1/3 0.5]);

% show rotating object
for i=0:10000
  
  % rotate object around y-axis
  ry = i/10;
  R = [cos(ry) 0 sin(ry) 0; 0 1 0 0; -sin(ry) 0 cos(ry) 0; 0 0 0 1];
  vertices_r = project(vertices,R);
  
  % translate object by (y,z) = (0.3,3.0)
  vertices_r(:,2) = vertices_r(:,2)+0.3;
  vertices_r(:,3) = vertices_r(:,3)+3.0;
  
  % render solid mesh (input 'colors' can also be omitted)
  [D,M,I] = renderMeshMex(vertices_r,faces,intrinsics,imgsize,0,colors);
  I(~M) = nan; D(~M) = nan;
  
  % render mesh as wireframe
  [D2,M2,I2] = renderMeshMex(vertices_r,faces,intrinsics,imgsize,1,colors);
  I2(~M2) = nan; D2(~M2) = nan;
  
  % show solid model
  set(h.f,'CurrentAxes',h.a1); imagesc(double(M)); axis off;
  set(h.f,'CurrentAxes',h.a2); imagesc(D,[1 4]); axis off; colormap(jet(1024));
  set(h.f,'CurrentAxes',h.a3); imagesc(I); axis off;
  
  % show wireframe model
  set(h.f,'CurrentAxes',h.a4); imagesc(double(M2)); axis off;
  set(h.f,'CurrentAxes',h.a5); imagesc(D2,[1 4]); axis off; colormap(jet(1024));
  set(h.f,'CurrentAxes',h.a6); imagesc(I2); axis off;
  
  % refresh
  drawnow; pause(0.01);
end
