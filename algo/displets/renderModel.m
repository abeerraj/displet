function [D,M] = renderModel(tr,scale,model,calib,lowres,linewidth)

if nargin<5
  lowres = 1;
end

if nargin<6
  linewidth = 0;
end

% object pose
Tr_object_plane = [[rodrigues(tr(1:3)) tr(4:6)']; 0 0 0 1];

% extract vertices and faces
vertices = model.hull.vertices;
faces = model.hull.faces;

% scale mesh
vertices = vertices.*(ones(size(vertices,1),1)*(scale.*model.scale));

% change up axis to -y
vertices = [vertices(:,1) -vertices(:,3) vertices(:,2)];

% project from object to camera coordinates
vertices = project(vertices,calib.Tr_plane_cam*Tr_object_plane);

% render mesh
if lowres
  intrinsics = [calib.f calib.f calib.cu calib.cv]/calib.scale;
  imgsize = [calib.ws calib.hs];
else
  intrinsics = [calib.f calib.f calib.cu calib.cv];
  imgsize = [calib.w calib.h];
end
[D,M] = renderMeshMex(vertices,faces,intrinsics,imgsize,linewidth);

% convert depth to disparity
D = (calib.f*calib.base)./D;
