function E = unary(u1, v1, idx1, planes, D2, thresh)
    numPlanes = size(planes,2);
    
    d_sgm  = D2(idx1);
    iV = d_sgm >= 0;
    
    % get disparity values for valid pixels
    % and compute disparities from plane proposals
    d_p = [u1(iV), v1(iV), ones(sum(iV), 1)] * planes;
        
    d_disp = abs(d_p - repmat(d_sgm(iV), [1, numPlanes]));  % L1-norm
    d_disp = min(d_disp, thresh); % apply robust threshold
    
    E = sum(d_disp,1);
end
