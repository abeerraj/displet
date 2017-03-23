% loads displets
% computes superpixels, mask, fitness score, distance transform for each displet

% input:
%       imgs: for semI and spI
%       params: for data directory, image file name, and sigmoid parameters
% output:
%       displets: 2D cell array encoding regions and displets of regions, 
%                 updated with computed fields.

function displets = loadDisplets(imgs, params)
    aleM = maskCars(imgs.semI);
    szI = [size(imgs.semI,1), size(imgs.semI,2)];
    
    nms = 1;
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    num_displets_per_region = 3;
    displetsData = load([params.dataDir '/displets/' params.dispType '/' params.image_fn '.mat']);

    accProb = 1;
    model_ids = [1 7 8 9 10 11 13 15];
    
    % for ablation study only
    if isfield(params, 'accProb')
        accProb = params.accProb;
    elseif isfield(params, 'numModels')
        model_ids = model_ids(randperm(length(model_ids), params.numModels));
    end

    displetsSorted = sortDispletsByScore(displetsData, nms, num_displets_per_region, accProb, model_ids);
    numRegions = length(displetsSorted);

    displets = displetsSorted; 
    for r = 1:numRegions
        for d = 1:length(displetsSorted{r})
            displets{r}{d}.psi = double(displetsSorted{r}{d}.s.psi-displetsSorted{r}{1}.s.psi);
            displets{r}{d}.M = imfill(displets{r}{d}.M, 'holes');
            displets{r}{d}.DT = double(bwdist(~displets{r}{d}.M));
            displets{r}{d}.DT = sigmoidDT(displets{r}{d}.DT, params.sigmoid);
            displets{r}{d}.DT = imresize(displets{r}{d}.DT, szI, 'nearest');
            displets{r}{d}.M = imresize(displets{r}{d}.M, szI, 'nearest');
            displets{r}{d}.D = imresize(displets{r}{d}.D, szI, 'nearest');
            displets{r}{d}.numPix = length(find(displets{r}{d}.M & aleM));
            [displets{r}{d}.sp,displets{r}{d}.dt] = maskDisplet(imgs.spI+1, displets{r}{d}.M, displets{r}{d}.DT);
            if 0
                figure;
                subplot(1,2,1); imagesc(displets{r}{d}.M);
                subplot(1,2,2); imagesc(displets{r}{d}.DT);
                keyboard;
            end
        end
    end
end

function [sp,dt] = maskDisplet(spI, M, DT)
    % get superpixel overlapping at least for 1 pixel with this displet
    spI_masked = spI;
    spI_masked(~M) = -1;
    sp = double(unique(spI_masked(spI_masked>0)))';
    dt = zeros(size(sp));
    
    % add superpixel mean distance from displet boundary
    for i = 1:length(sp)
        s = sp(i);
        dts = DT(spI==s);
        dt(i) = mean(dts);
    end
end
