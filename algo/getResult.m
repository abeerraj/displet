% computes the disparity image by using the predictions for each plane
% input:
%       pred: predictions, number of planes x 3
%       planes: for centers, and pixels inside the plane.
% output:
%       resultDipsI: resulting disparity image

function resultDispI = getResult(pred, planes, szI)
    resultDispI = -ones(szI);

    for p = 1:length(planes)
        resultDispI(planes(p).idx) = dispPlane(pred(p,:), planes(p).u - planes(p).cx, planes(p).v - planes(p).cy);
    end
end