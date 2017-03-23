function data = loadData(params)

% load calibration
calib_fn = [params.dataDir '/calib/' params.image_fn(1:end-3) '.txt'];
data.calib = loadCalibration(calib_fn);
data.calib.scale = 4;

imgs = loadImgs(params);

% load image
data.I = imgs.colorI;

% load disparity map
data.D = imgs.dispI;

% for smoothing displets
data.spI = imgs.spI;

if 0
  figure,imagesc(data.D),colormap(jet(1024));
  keyboard;
end

% set disparity of half-occluded left part to infinite (=> not included in iou)
V = data.D >= 0;
l = [];
for v=round(size(V,1)/2):5:size(V,1)
  l = [l find(V(v,:),1,'first')];
end
u = max(round(median(l))-20,1);
data.D(:,1:u) = inf;

% load ground truth disparity map
% data.D_noc = imgs.gtDispI;

% load segmentation map
data.S = imgs.semI;

% add image dimensions to calib
data.calib.w = size(data.I,2);
data.calib.h = size(data.I,1);
data.calib.ws = floor(data.calib.w/data.calib.scale);
data.calib.hs = floor(data.calib.h/data.calib.scale);

% compute scaled versions
data.Is = min(imresize(data.I,[data.calib.hs data.calib.ws],'bilinear'),1);
data.Ds = imresize(data.D,[data.calib.hs data.calib.ws],'nearest');
% data.Ds_noc = imresize(data.D_noc,[data.calib.hs data.calib.ws],'nearest');
data.Ss = imresize(data.S,[data.calib.hs data.calib.ws],'nearest');

% car segments
data.Ss_car = data.Ss(:,:,1)==64 & data.Ss(:,:,2)==0 & data.Ss(:,:,3)==128;

% road plane -> camera motion
data.calib.Tr_plane_cam = eye(4);
data.calib.Tr_plane_cam(2,4) = 1.6;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function calib = loadCalibration(filename)

% open file
fid = fopen(filename,'r');

if fid<0
  calib = [];
  return;
end

% read projection matrices
calib.P{1} = readVariable(fid,'P0',3,4);
calib.P{2} = readVariable(fid,'P1',3,4);
calib.P{3} = readVariable(fid,'P2',3,4);
calib.P{4} = readVariable(fid,'P3',3,4);

% extract intrinsics
calib.f    = calib.P{2}(1,1);
calib.cu   = calib.P{2}(1,3);
calib.cv   = calib.P{2}(2,3);
calib.base = -calib.P{2}(1,4)/calib.P{2}(1,1);

% close file
fclose(fid);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function A = readVariable(fid,name,M,N)

% rewind
fseek(fid,0,'bof');

% search for variable identifier
success = 1;
while success>0
  [str,success] = fscanf(fid,'%s',1);
  if strcmp(str,[name ':'])
    break;
  end
end

% return if variable identifier not found
if ~success
  A = [];
  return;
end

% fill matrix
A = zeros(M,N);
for m=1:M
  for n=1:N
    [val,success] = fscanf(fid,'%f',1);
    if success
      A(m,n) = val;
    else
      A = [];
      return;
    end
  end
end
