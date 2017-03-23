function M_noc = maskNonOccluded (D,M,data)

% init non-occluded mask
M_noc = M;

% set stereo estimate
D_stereo = data.D;
radius = 5;
if any(size(D_stereo)~=size(D))
  D_stereo = data.Ds;
  radius = 2;
end

% compute ratio of infinite values contributing to occlusion
M_occ = D_stereo>D+5 & D_stereo<100 & M;
inf_ratio = sum(isinf(D_stereo(M_occ)))/length(find(M_occ));

% only remove occluded part if occlusion not caused by half-occlusion
if inf_ratio<0.9

  % compute & smooth non-occluded part of mask
  M_noc = D_stereo<D+5 & M;
  se = strel('disk',radius,0);
  M_noc = imerode(M_noc,se);
  M_noc = imdilate(M_noc,se);

  % find largest connected component
  CC = bwconncomp(M_noc,4);
  num_px = cellfun(@numel,CC.PixelIdxList);
  [foo,idx] = max(num_px);
  M_noc = false(size(M_noc));
  M_noc(CC.PixelIdxList{idx}) = true;
end
