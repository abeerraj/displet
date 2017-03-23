function model = loadModel (model_idx,model_dir)

load(sprintf('%s/semi_convex_hull/%02d.mat',model_dir,model_idx));
