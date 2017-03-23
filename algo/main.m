function err = main(params)

params.image_fn = sprintf('%06d_10', params.imgIdx);

% load images, groundtruth, calibration, and displets
% compute superpixel adjacency matrix, boundaries 
% and differences between superpixels based on color and location.  
[imgs, imgData, displets] = loadImgs(params);

% initialize plane parameters
planes = initPlaneParams(imgs.spI+1, imgs.ssI);

% numPlanes and numDisplets for easy access
szI = size(imgs.spI);
numPlanes = length(planes);
numDisplets = 0;
for r = 1:length(displets)
    numDisplets = numDisplets + length(displets{r});
end

% neighboring super-pixels 
% sp1: 1st neighbor, sp2: 2nd neighbor
[sp1, sp2] = ind2sub(size(imgData.adjMat), find(imgData.adjMat));

% initialize random variables on the graph
if ~isempty(displets)
    variables = [ones(1, numPlanes) * params.numParticles ones(1, numDisplets) * 2];
else
    variables = ones(1, numPlanes) * params.numParticles;
end

% states of random variables through iterations
% initialized for the 1st one.
y = zeros(numPlanes, 3, params.numIters+1);
y(:,:,1) = [planes.alpha; planes.beta; planes.gamma]';

% centers of planes in a matrix
centers = [planes.cx; planes.cy]';

% initialize standard deviation of particles
stds = zeros(params.numIters+1,3);
stds(1,:) = params.stdInit;

% create boundary probability map from disparity image
if params.occWeight > 0
    imgData.boundDistMat = getOccBoundaries(params, imgs.colorI, planes, sp1, sp2);
end

% energy, error and map estimations through iterations
energy = zeros(params.numIters, 1);
errs = zeros(params.numIters+1, 1);
map = zeros(params.numIters, numPlanes+numDisplets);

% initial disparity map from plane fitting
resultDispI = getResult(y(:,:,1), planes, szI);

% and corresponding disparity error
if isfield(imgs, 'gtDispI')
    errs(1) = disp_error(imgs.gtDispI, resultDispI, params.tau);
end

if params.verbose >= 1
    fprintf('Iter%02d\n', 0);
    if isfield(imgs, 'gtDispI')
      fprintf('error: %.2f%%\n', 100*errs(1));
    end
end

if params.verbose >= 2
    figure('Position', 2*[20 200 szI(2) szI(1)], 'Visible', 'on');
    axes('Position', [0 0 1 1]);
    displayI = displayResult(imgs, params, resultDispI); 
    imshow(displayI); refresh; pause(0.5);
end

for t = 2:params.numIters+1
    % initialize particles
    particles = zeros(3, params.numParticles, numPlanes);
    
    % initialize unary
    E_data = zeros(numPlanes, params.numParticles);

    % if displets are used, add hypotheses from displets as particles
    % and set particle origins to corresponding displets
    if params.higherOrderWeight == 0
        numFilled = zeros(numPlanes, 1);
        particleOrigins = zeros(params.numParticles, numPlanes);
    else
        [particles, numFilled, particleOrigins] = addDispletHypos(particles, displets);
    end

    for p = 1:numPlanes
        % to keep track of particles filled.
        cursor = numFilled(p) + 1;
        % get the map estimation from the prev. iter. as the first particle
        particles(:,cursor,p) = y(p,:,t-1);
        % update number of remaining particles
        numLeft = params.numParticles - cursor;
        
        for d = 1:3
            if params.sampleFromNeighbors
                % if we're going to sample from the neighbors, then only
                % sample half of the remaining particles around the map
                particles(d,cursor+1:cursor+ceil(numLeft/2),p) = normrnd(y(p,d,t-1), stds(t-1,d), ceil(numLeft/2), 1);
            else
                % otherwise sample all around the map.
                particles(d,cursor+1:end,p) = normrnd(y(p,d,t-1), stds(t-1,d), numLeft, 1);
            end
        end
        
        if floor(numLeft/2) > 0 && params.sampleFromNeighbors
            % update cursor and sample particles from the neighboring
            % superpixels based on color and loc differences
            cursor = cursor + ceil(numLeft/2);
            particles(:,cursor+1:end,p) = sampleParticles(p, imgData.colorLocProbs(p,:), floor(numLeft/2), y(:,:,t-1), centers)';
        end       
        
        % UNARY: compute truncated unary term based on the particles, 
        %        initial diparity map and threshold parameter.
        factors{p}.v = p;   
        E_data(p,:) = unary(planes(p).u - planes(p).cx, planes(p).v - planes(p).cy, planes(p).idx, particles(:,:,p), imgs.dispI, params.unaryThresh);
        factors{p}.e = params.unaryDepthWeight * E_data(p,:); 
    end   
    
    % PAIRWISE: compute boundary pairwise term
    particles_abc = convert2abc(particles, centers);
    E_pairwise = energyPairwiseMex(imgData.adjMat, imgData.sp_boundaries, particles_abc, params.numParticles, params.pwBoundThresh, szI(1)); 
    % PAIRWISE: convert from plane represenation to normal representation
    %           and compute normal pairwise term
    particles_normals = convert2normals(particles_abc, imgData.K, imgData.baseline);
    E_pairwiseNormals = energyPairwiseNormalsMex(imgData.adjMat, particles_normals, params.numParticles, params.pwNormThresh); 
    
    % PAIRWISE: add computed pairwise terms (by weighting) to the graph as factors
    %           between neighboring superpixels
    factorsCount = numPlanes;
    for i = 1:length(sp1)
        factors{factorsCount+i}.v = [sp1(i), sp2(i)];
        factors{factorsCount+i}.e = params.pwBoundWeight * E_pairwise{sp1(i), sp2(i)}(:)' + params.pwNormWeight * E_pairwiseNormals{sp1(i), sp2(i)}(:)';
        if params.occWeight > 0
            % add the weighting computed from occlusions
            bd = params.occWeight*imgData.boundDistMat(sp1(i),sp2(i));
            factors{factorsCount+i}.e = (1-bd) * factors{factorsCount+i}.e;
        end
    end
    
    % DISPLETS
    if ~isempty(displets)
        factorsCount = factorsCount + length(sp1); 
        idx = 1;
        % for each region
        for r = 1:length(displets)
            % for each displet proposed for the region
            for d = 1:length(displets{r})
                displet = displets{r}{d};
                % find the particles coming from the displet
                [states, addedHypos_sps] = find(particleOrigins == displetID(r,d));
                states = states';

                % displet potential
                factorsCount = factorsCount + 1;
                factors{factorsCount}.v = numPlanes+idx;
                factors{factorsCount}.e = [0, params.higherOrderPenalty * 10000 * displet.psi - params.higherOrderWeight * displet.numPix];
                
                % consistency potential: between displet and its superpixels
                for i = 1:length(displet.sp)
                    relIdx = find(displet.sp(i) == addedHypos_sps);
                    if ~isempty(relIdx)
                        factorsCount = factorsCount + 1;
                        factors{factorsCount}.v = [displet.sp(i) numPlanes+idx];
                        E = [zeros(params.numParticles,1) params.higherOrderConsistency * 100000 * displet.dt(i) * ones(params.numParticles,1)];
                        E(states(relIdx),2) = 0;
                        factors{factorsCount}.e = E(:)';
                    end
                end
                idx = idx + 1;
            end
        end
    end
    
    % inference
    options.maxIter = params.trwsNumIters;
    [map(t-1,:), energy(t-1)] = trwsMex(variables, factors, options);

    % map estimate of the iteration
    for p = 1:numPlanes
        y(p,:,t) = particles(:, map(t-1,p), p);
    end
    
    % resulting disparity image and error
    resultDispI = getResult(y(:,:,t), planes, szI);
    if isfield(imgs, 'gtDispI')
        errs(t) = disp_error(imgs.gtDispI, resultDispI, params.tau);
    end
        
    if params.verbose >= 1
        fprintf('Iter%02d\n', t-1);
        %fprintf('map: %s\n', num2str(map(t-1,1:numPlanes)));
        %fprintf('energy: %f\n', energy(t-1));
        
        if isfield(imgs, 'gtDispI')
            fprintf('error: %.2f%%\n', 100*errs(t));
        end
    end
    
    if params.verbose >= 2
        displayI = displayResult(imgs, params, resultDispI);
        imshow(displayI); refresh; pause(0.5);
    end
    
    % update standard deviations for sampling particles
    stds(t,:) = repmat(0.5 * exp(-(t-1)/10), 1, 3);
    stds(t,3) = 10 * stds(t,3);
end

err = errs(end);

if params.saveResult
    if ~exist(params.outDir, 'dir')
        mkdir(params.outDir)
    end
    
    result_filename = [params.outDir '/' params.image_fn '.png'];
    disp_write(resultDispI, result_filename);
    
    if params.verbose >= 1
      fprintf('results saved to: %s\n', result_filename);
    end

    if params.saveErrEnergy
        dlmwrite([params.outDir '/' params.image_fn '_errs.txt'], errs);
        dlmwrite([params.outDir '/' params.image_fn '_energy.txt'], energy);
    end
end


