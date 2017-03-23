function [all_noc, refl_noc, all_occ, refl_occ] = getErr(mode, exp, dispType, tau, images)
    params.mode = mode;
    params.exp = exp;
    
    if nargin < 3
        params.dispType = 'cnn';
    else
        params.dispType = dispType;
    end
    
    if nargin < 4 
        params.tau = 3;
    else
        params.tau = tau;
    end
    
    if nargin < 5
        numImgs = 194 + strcmp(params.mode, 'testing');
        images = 0:numImgs-1;
    else
        numImgs = length(images);
    end

    params.saveErrEnergy = 0;
    dirs = initDirs(params);
    errs_noc = zeros(numImgs, 1);
    greaterThanTau_noc = zeros(numImgs, 1);
    numValid_noc = zeros(numImgs, 1);
    
    errs_occ = zeros(numImgs, 1);
    greaterThanTau_occ = zeros(numImgs, 1);
    numValid_occ = zeros(numImgs, 1);

    parfor i = 1:numImgs
        img = images(i);
        fn = sprintf('%06d_10.png', img);
        resultDispI = disp_read([dirs.result '/' fn]);

        gtDispI_noc = disp_read([dirs.gtDisp_noc, '/', fn]);    
        gtReflDispI_noc = disp_read([dirs.gtRefl_noc, '/', fn]);  
        
        gtDispI_occ = disp_read([dirs.gtDisp_occ, '/', fn]);    
        gtReflDispI_occ = disp_read([dirs.gtRefl_occ, '/', fn]);  
        
        [greaterThanTau_noc(i), numValid_noc(i)] = disp_refl_error(gtReflDispI_noc, resultDispI, params.tau);
        [errs_noc(i), ~] = disp_error(gtDispI_noc, resultDispI, params.tau);
        
        [greaterThanTau_occ(i), numValid_occ(i)] = disp_refl_error(gtReflDispI_occ, resultDispI, params.tau);
        [errs_occ(i), ~] = disp_error(gtDispI_occ, resultDispI, params.tau);
    end

    all_noc = sum(errs_noc) / numImgs;
    all_noc = all_noc * 100;
    
    refl_noc = sum(greaterThanTau_noc) / sum(numValid_noc);
    refl_noc = refl_noc * 100;
    
    all_occ = sum(errs_occ) / numImgs;
    all_occ = all_occ * 100;
    
    refl_occ = sum(greaterThanTau_occ) / sum(numValid_occ);
    refl_occ = refl_occ * 100;
end

function [greaterThanTau, numValid] = disp_refl_error(D_gt,D_est,tau)
    E = abs(D_gt-D_est);
    E(D_gt<=0) = 0;
    greaterThanTau = find(E>tau);
    greaterThanTau = length(greaterThanTau);
    numValid = length(find(D_gt>0));
end

