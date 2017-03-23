function n = dispPlaneToNormal(dispPlane,K,base_m)
% computes 3D scaled normal from disparity plane

% Parameters
%   dispPlane   coefficients of the disparity plane, conventions see below
%   K           3x3 camera matrix containing principal distance and point
%   base_m      (positive) length of the stereo baseline

    % camera parameters
    f  = K(1,1);
    cu = K(1,3);
    cv = K(2,3);
    
    % parameters of the disparity plane  
    % disp = alpha*u + beta*v + gamma
    alpha = dispPlane(1);
    beta  = dispPlane(2);
    gamma = dispPlane(3);
        
    % transform disparity plane to scaled normal
    n    = zeros(3,1);
    n(1) = -alpha/base_m;
    n(2) = -beta/base_m;
    n(3) = -(gamma+cu*alpha+cv*beta) / (f*base_m);
    
end