% dataDir: data directory
% expName: name of the experiment, also name of the results folder
% mode: 'training' or 'testing'
% dispType: type of disparity map, 'sgm' or 'cnn'
% i: index to the image on Kitti
% callMain('/ps/geiger/fguney/semantic_stereo/crf_data/release', 'release', 'testing', 'cnn', 0);

function callMain(dataDir, expName, mode, dispType, i)

% init script to add relative paths
init;

if nargin == 4 && ischar(i)
    i = str2double(i);
end

%parfor i = 0:194
    params = struct;

    % data directory
    params.dataDir = [dataDir '/Kitti/' mode];
    params.modelDir = [dataDir '/models'];

    % disparity type: cnn or sgm
    params.dispType = dispType;
    
    % image index on Kitti
    params.imgIdx = i;

    % params.verbose >= 1: Iteration number and disparity error (only for training) are printed. 
    %                      It's also possible to print energy and map solution.
    % params.verbose >= 2: Input image (top-left), initial disparity image (top-right), 
    %                      resulting disparity image (bottom-left), and error image (bottom right, only for training) 
    %                      are visualized through the iterations. 
    params.verbose = 2;

    % params.saveResult: If 1, the resulting disparity image is saved in params.outDir
    params.outDir = [params.dataDir '/' expName]; 
    params.saveResult = 1;
    % params.saveErrEnergy: If 1, energy and error (only for training) through iterations 
    %                       are also saved in the same directory.
    params.saveErrEnergy = 0;

    % parameters of the algorithm
    % cnn values are set from the table in the supplementary material.
    % sgm values are set experimentally.
    
    params.unaryThresh = 6.9822;
    params.pwBoundThresh = 3.404;
    params.pwNormThresh = 0.0617;
    params.trwsNumIters = 200;
    params.numIters = 10; % note: for the experiments in the paper we used 40
    params.numParticles = 30;
    params.stdInit = [0.5, 0.5, 5];
    params.tau = 3;

    params.unaryDepthWeight = 1;

    if strcmp(params.dispType, 'cnn')
        params.pwBoundWeight = 1.505; 
        params.pwNormWeight = 586.8786;
    elseif strcmp(params.dispType, 'sgm')
        params.pwBoundWeight = 1.2; 
        params.pwNormWeight = 600;
    end

    params.sampleFromNeighbors = 1;
    params.occWeight = 1.0031;
    
    % displet realated parameters
    params.higherOrderWeight = 1.5365;
    params.higherOrderPenalty = 0.9658;
    params.higherOrderConsistency = 0.6269;
    params.sigmoid = [7.9020 0.8293];

    % used for ablation studies in the paper
    % params.accProb = 0.5;
    % params.numModels = 3;

    % run_sampling(params);
    main(params);
%end


