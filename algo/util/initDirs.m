function dirs = initDirs(params)
    datasetDir = ['/ps/geiger/fguney/semantic_stereo/kitti_stereo/' params.mode];

    dirs.gtSem = [datasetDir '/semantic_labels'];
    dirs.gtDisp_noc = [datasetDir '/disp_noc'];
    dirs.gtDisp_occ = [datasetDir '/disp_occ'];
    dirs.gtRefl_noc = [datasetDir '/disp_refl_noc'];
    dirs.gtRefl_occ = [datasetDir '/disp_refl_occ'];

    dirs.result = ['/ps/geiger/fguney/semantic_stereo/crf_data/release/Kitti/' params.mode '/' params.exp]; 
end