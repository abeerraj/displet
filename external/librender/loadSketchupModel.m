function mesh = loadSketchupModel (filename,max_num_faces)

% load 3d object
obj = read_wobj(filename);

% concatenate sub-meshes
mesh.vertices = obj.vertices;
mesh.vertices_color = obj.vertices_color;
mesh.faces = [];


for i=1:length(obj.objects)
  data = obj.objects(i).data;
  if isfield(data,'vertices')
    mesh.faces = [mesh.faces; data.vertices];
  end
end

% convert units into meters
if strcmp(obj.units,'mm') || strcmp(obj.units,'millimeters')
  mesh.vertices = mesh.vertices * 0.001;
elseif strcmp(obj.units,'cm') || strcmp(obj.units,'centimeters')
  mesh.vertices = mesh.vertices * 0.01;
elseif strcmp(obj.units,'dm') || strcmp(obj.units,'decimeters')
  mesh.vertices = mesh.vertices * 0.1;
elseif strcmp(obj.units,'m') || strcmp(obj.units,'meters')
  mesh.vertices = mesh.vertices * 1;
elseif strcmp(obj.units,'feet')
  mesh.vertices = mesh.vertices * 0.3048;
elseif strcmp(obj.units,'inches')
  mesh.vertices = mesh.vertices * 0.0254;
else
  error('Unsupported units: %s',units);
end

% reduce number of faces
if nargin>1
  num_faces = size(mesh.faces,1);
  if num_faces>max_num_faces
    % buffer the old mesh
    mesh_old = mesh;
    mesh = reducepatch(mesh,max_num_faces);
    num_faces_reduced = size(mesh.faces,1);
    
    % also reduce vertices color
    [overlap, idx_old, idx_new]=intersect(mesh_old.vertices, mesh.vertices, 'rows');
    if (~isempty(mesh_old.vertices_color))
        mesh.vertices_color = mesh_old.vertices_color(idx_old, :);
    else
        mesh.vertices_color = [];
    end
    fprintf('Reduced faces: %d -> %d\n',num_faces,num_faces_reduced);
  end
end

clear mesh_old;

% center mesh
c = (min(mesh.vertices)+max(mesh.vertices))/2;
mesh.vertices = mesh.vertices-ones(size(mesh.vertices,1),1)*c;

