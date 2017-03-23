function boundDistMat = getOccBoundaries(params, colorI, planes, sp1, sp2)
    D1 = disp_read([params.dataDir '/dispmaps/' params.dispType '/disp_0/' params.image_fn '.png']);
    D2 = disp_read([params.dataDir '/dispmaps/' params.dispType '/disp_1/' params.image_fn '.png']);
    B1 = disparityBoundariesMex(D1);
    O1 = occlusionBoundariesMex(D1, D2);
    PB = max(B1,O1);
    GI = gradientImage(colorI);
    
    numPlanes = length(planes);
    boundDistMat = zeros(numPlanes, numPlanes);
    r = 5; w = size(PB,2); h = size(PB,1);
    se = strel('disk',1,0);

    for i=1:length(sp1)
        u1 = planes(sp1(i)).cx; v1 = planes(sp1(i)).cy; 
        u2 = planes(sp2(i)).cx; v2 = planes(sp2(i)).cy;
        u1w = max(min(u1,u2)-r,1); v1w = max(min(v1,v2)-r,1);
        u2w = min(max(u1,u2)+r,w); v2w = min(max(v1,v2)+r,h);
        u1 = u1-u1w+1; u2 = u2-u1w+1;
        v1 = v1-v1w+1; v2 = v2-v1w+1;
        PB_sub = PB(v1w:v2w,u1w:u2w);
        GI_sub = GI(v1w:v2w,u1w:u2w);
        
        if sum(PB_sub(:))==0 || sum(GI_sub(:))==0
            continue;
        end
        
        MB_sub = zeros(size(PB_sub));
        [u,v] = bresenham(u1,v1,u2,v2);
        idx = sub2ind(size(PB_sub),v,u);
        MB_sub(idx) = 1;
        MB_sub = imdilate(MB_sub,se);
        PB_masked = MB_sub.*PB_sub;
        GI_masked = MB_sub.*GI_sub;
        boundDistMat(sp1(i),sp2(i)) = min(max(PB_masked(:))/5,1)*max(GI_masked(:));
    end
end