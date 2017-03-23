% applies the plane equation to given coordinates
% input:
%       plane: plane parameters
%       u-v: pixel coordinates
% output:
%       result: resulting disparity

function result = dispPlane(plane, u, v)
    result = [u, v, ones(size(u))] * plane';
end