% initializes plane parameters by fitting planes to the valid values of disparity image
% input: 
%       segI: superpixel image, from 1 to number of superpixels
%       dispI: disparity image
%       segNums: (optional) fits planes to only specified superpixels. 
% output:
%       planes: struct array. for each plane, it specifies pixels inside [u,v],
%               center of the plane [cx,cy], and the plane parameters [alpha, beta, gamma]


function planes = initPlaneParams(segI, dispI, segNums)

if nargin < 3
    numPlanes = double(max(segI(:)));
    segNums = 1:numPlanes;
else
    numPlanes = length(segNums);
end

dispIdu = zeros(size(dispI));
dispIdv = zeros(size(dispI));
dispIdu(:,1:end-1) = dispI(:,2:end)-dispI(:,1:end-1);
dispIdv(1:end-1,:) = dispI(2:end,:)-dispI(1:end-1,:);

segIvalid = segI;
segIvalid(dispI == -1) = -1;

valid = true(numPlanes, 1);

planes = struct;
for i = 1:numPlanes
    p = segNums(i);
    idx = find(segI == p);
    [v, u] = ind2sub(size(segI), idx);
    
    cy = round(median(v));
    cx = round(median(u));

    planes(i).idx = idx;
    planes(i).u = u;
    planes(i).v = v;
    planes(i).cx = cx;
    planes(i).cy = cy;
    
    validIdx = find(segIvalid == p);

    if length(validIdx) < 0.5 * length(idx)
        alpha = 0; beta = 0; gamma = -1;
        valid(i) = 0;
    else
        alpha = median(dispIdu(idx));
        beta = median(dispIdv(idx));
        gamma = median(dispI(idx) - (alpha * (u - cx) + beta * (v - cy)));
    end
    
    planes(i).alpha = alpha;
    planes(i).beta = beta;
    planes(i).gamma = gamma;   
end

centers = [planes.cx; planes.cy]';
gammas = [planes.gamma];
validGammas = gammas(valid);

if ~isempty(validGammas)
    nnIdx = knnsearch(centers(valid,:), centers(~valid,:));

    c = 1;
    for i = 1:numPlanes
        if ~valid(i)
            planes(i).gamma = validGammas(nnIdx(c));
            c = c+1;
        end
    end
end
    
end