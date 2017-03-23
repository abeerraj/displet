function run_sampling(params)
warning off MATLAB:MKDIR:DirectoryExists; clear functions;

% global variable for quick data exchange
global data

% parameters
num_samples = 5000;
model_ids = [1 7 8 9 10 11 13 15];
orientations = 0:pi/2:3*pi/2;

% load data & create region proposals
params.image_fn = sprintf('%06d_10', params.imgIdx);
data = loadData(params);
M = regionProposals(data,0);
M = [M{1} M{2}];

% number of proposals / models / orientations
num.proposals = length(M);
num.models = length(model_ids);
num.orientations = length(orientations);

% sample displets
displets = [];
for i=1:num.proposals
  for j=1:num.models
    for k=1:num.orientations
    
      % status
      fprintf('Processing region %d/%d, model %d/%d, orientation %d/%d (#displets: %d) ...\n',i,length(M),j,length(model_ids),k,length(orientations),length(M)*length(model_ids)*length(orientations));

      % set mask, model & orientation
      data.Ms = M{i};
      data.model = loadModel(model_ids(j), [params.modelDir]);
      data.orientation = orientations(k);

      % run sampler
      displet = [];
      displet.region_id = i;
      displet.model_id = model_ids(j);
      displet.orientation = orientations(k);
      displet.s = sampling(num_samples,0);

      % extract displet and mask from pose      
      [tr,scale] = poseFromSample(displet.s.x);
      [displet.D,displet.M] = renderModel(tr,scale,data.model,data.calib);
      displet.D(~displet.M) = -1;
      displet.M = maskNonOccluded(displet.D,displet.M,data);

      % add to displets
      displets{end+1} = displet;
    end
  end
end

% smooth displets
preds = smoothDisplets(params, data, displets);

% save the necessary data
save(sprintf('%s/displets/%s/%s.mat', params.dataDir, params.dispType, params.image_fn), 'displets', 'num', 'M', 'preds');

