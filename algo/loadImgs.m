% loads images, groundtruth (only for training), calibration files
%       displets (only if their weight is greater than 0)
%
% computes superpixel adjacency matrix, boundaries 
%          and differences between superpixels based on color and location.
% input: 
%       params: for data directory, image file name, and checking the
%               weight of displets.
% output:
%       imgs: struct array with fields colorI (color), dispI (initial disparity map),
%             spI (superpixel), ssI (stereoslic disparity), semI (segmentation seg.),
%             and gtDispI (ground-truth disparity).
%       imgData: struct with fields K (camera matrix), baseline (stereo baseline)
%                adjMat (adjacency matrix), sp_boundaries (superpixel boundaries),
%                colorLocProbs (probabilities for sampling from neighbors based on color and location)
%       displets: cell array of displets

function [imgs, imgData, displets] = loadImgs(params)
    imgs.colorI = imread([params.dataDir '/colored_0/' params.image_fn '.png']);
    imgs.dispI = disp_read([params.dataDir '/dispmaps/' params.dispType '/disp_0/' params.image_fn '.png']);
    imgs.spI = imread([params.dataDir '/stereoslic/'  params.dispType '/superpixels/' params.image_fn '.png']);
    imgs.ssI = disp_read([params.dataDir '/stereoslic/'  params.dispType '/disparity/' params.image_fn '.png']);
    imgs.semI = imread([params.dataDir '/ale_sem_seg/' params.image_fn '.png']);

    gt_disp_fn = [params.dataDir '/gt/disp_noc/' params.image_fn '.png'];
    if exist(gt_disp_fn, 'file')
        imgs.gtDispI = disp_read(gt_disp_fn);
    end

    if nargout >= 2
        camMats = loadCamMats([params.dataDir '/calib/' params.image_fn(1:end-3) '.txt']);
        imgData.K = camMats(:,:,1);
        imgData.baseline = abs(camMats(1,end,2));

        imgData.adjMat = superpixel_adjacencies(imgs.spI+1);  
        [M,~] = superpixel_masks(imgs.spI+1);
        imgData.sp_boundaries = superpixel_boundaries(M, imgs.spI+1);
        imgData.colorLocProbs = getColorLocProbs(imgs.colorI, imgs.spI);
    end

    displets = cell(0);
    if nargout == 3 && params.higherOrderWeight > 0
        displets = loadDisplets(imgs, params);
    end
end
