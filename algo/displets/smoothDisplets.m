function preds = smoothDisplets(params, data, displets)

szI = size(data.spI);
for d = 1:length(displets)
    displets{d}.M = imfill(displets{d}.M, 'holes');
    displets{d}.M = imresize(displets{d}.M, szI, 'nearest');
    displets{d}.D = imresize(displets{d}.D, szI, 'nearest');
end

% vMat = [];
% vRow = [];
preds = cell(0);

for d = 1:length(displets)    
    fprintf('Smoothing:(%d/%d)\n', d, length(displets));
    
    displet = displets{d};
    spI = maskDisplet(data.spI, displet.M);
    
    if length(unique(spI(spI > 0))) < 2
        preds{d} = [];
        continue;
    end
        
    displet.D(~displet.M) = -1;
    dispI = displet.D;
    adjMat = superpixel_adjacencies(spI);
    [M,~] = superpixel_masks(spI);
    sup_boundaries = superpixel_boundaries(M, spI);

    planes = initPlaneParams(spI, dispI);
    numPlanes = length(planes);

    [sp1, sp2] = ind2sub(size(adjMat), find(adjMat));
    variables = ones(1, numPlanes) * params.numParticles;
    y = zeros(numPlanes, 3, params.numIters+1);
    y(:,:,1) = [planes.alpha; planes.beta; planes.gamma]';
    centers = [planes.cx; planes.cy]';
    stds = zeros(params.numIters+1,3);
    stds(1,:) = params.stdInit;

    energy = zeros(params.numIters, 1);
    map = zeros(params.numIters, numPlanes);

    factors = [];
    for t = 2:params.numIters+1
        particles = zeros(3, params.numParticles, numPlanes);
        E_data = zeros(numPlanes, params.numParticles); 
        numFilled = zeros(numPlanes, 1);

        for p = 1:numPlanes
            cursor = numFilled(p) + 1;
            particles(:,cursor,p) = y(p,:,t-1);
            numLeft = params.numParticles - cursor;

            for i = 1:3
                particles(i,cursor+1:end,p) = normrnd(y(p,i,t-1), stds(t-1,i), numLeft, 1);
            end  

            factors{p}.v = p;   
            E_data(p,:) = unary(planes(p).u - planes(p).cx, planes(p).v - planes(p).cy, planes(p).idx, particles(:,:,p), dispI, params.unaryThresh);
            factors{p}.e = params.unaryDepthWeight * E_data(p,:); 
        end   

        particles_abc = convert2abc(particles, centers);
        particles_normals = convert2normals(particles_abc, data.calib.P{1}, data.calib.base);
        E_pairwise = energyPairwiseMex(adjMat, sup_boundaries, particles_abc, params.numParticles, params.pwBoundThresh, szI(1)); 
        E_pairwiseNormals = energyPairwiseNormalsMex(adjMat, particles_normals, params.numParticles, params.pwNormThresh); 

        factorsCount = numPlanes;
        for i = 1:length(sp1)
            factors{factorsCount+i}.v = [sp1(i), sp2(i)];
            factors{factorsCount+i}.e = params.pwBoundWeight * E_pairwise{sp1(i), sp2(i)}(:)' + params.pwNormWeight * E_pairwiseNormals{sp1(i), sp2(i)}(:)';
        end

        [map(t-1,:), energy(t-1)] = trwsMex(variables, factors);
        for p = 1:numPlanes
            y(p,:,t) = particles(:, map(t-1,p), p);
        end

        stds(t,:) = repmat(0.5 * exp(-(t-1)/10), 1, 3);
        stds(t,3) = 10 * stds(t,3);
    end

    pred = y(:,:,params.numIters+1);
%     resultDispI = -ones(szI);
%     for p = 1:length(planes)
%         resultDispI(planes(p).idx) = dispPlane(pred(p,:), planes(p).u - planes(p).cx, planes(p).v - planes(p).cy);
%     end
    preds{d} = pred;
    
%     v = disp_to_color(imresize(resultDispI, szI/4, 'nearest'));
%     vRow = [vRow, v]; 
%     if mod(d,8) == 0
%         vMat = [vMat; vRow];
%         vRow = [];   
%     end
end

end

function spI_masked = maskDisplet(spI, M)
    spI_masked = double(spI);
    spI_masked(~M) = -1;
    
    [~, ~, spI_masked] = unique(spI_masked);
    spI_masked = reshape(spI_masked, size(spI)) - 1;
end
