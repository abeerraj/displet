function displets_sorted = sortDispletsByScore (displetsData, nms, num_displets_per_region, accProb, model_ids, max_dist, min_overlap)

if nargin<6
    max_dist = 15; % meters (z)
end

if nargin<7
    min_overlap = 200; % pixels
end

M = displetsData.M;
displets = displetsData.displets;
preds = displetsData.preds;
num = displetsData.num;

% copy predictions to displets
for d=1:length(displets)
    displets{d}.p = preds{d};
end

num_displets_per_proposal = num.models*num.orientations;
num_displets_per_orientation = 2;

displets_sorted = [];
for i=1:num.proposals
    i1 = (i-1)*num_displets_per_proposal+1;
    i2 = i1+num_displets_per_proposal-1;
    displets_curr = displets(i1:i2);
    psi = zeros(1,num_displets_per_proposal);  
    for j=1:num_displets_per_proposal
    psi(j) = displets_curr{j}.s.psi;
    end
    [~,idx] = sort(psi);  
    displets_curr = displets_curr(idx);
    
    idx = false(1, length(displets_curr));
    for d = 1:length(displets_curr)
        idx(d) = (rand <= accProb) && any(model_ids == displets_curr{d}.model_id);
    end
    displets_curr = displets_curr(idx);

    if nms
        displets_curr = nonMaximumSuppression(displets_curr,num_displets_per_orientation);
    end
    displets_curr = filterValidDisplets(M{i},displets_curr,max_dist,min_overlap);
    num_displets = min(length(displets_curr),num_displets_per_region);
    displets_curr = displets_curr(1:num_displets);
    if ~isempty(displets_curr)
        displets_sorted{end+1} = displets_curr;
    else
        displets_sorted{end+1} = cell(1,0);
    end
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function displets_out = nonMaximumSuppression (displets_in,num_displets_per_orientation)

displets_out = [];
for i=1:length(displets_in)
    count = 0;
    for j=1:length(displets_out)
        if displets_in{i}.orientation==displets_out{j}.orientation
            count = count+1;
        end
    end
    if count<num_displets_per_orientation
        displets_out{end+1} = displets_in{i};
    end
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function displets_out = filterValidDisplets (M,displets_in,max_dist,min_overlap)

displets_out = [];

for i=1:length(displets_in)
    if ~isempty(displets_in{i}.p) && ...                % has predictions
        sum(sum(displets_in{i}.M&M))>min_overlap && ...  % has enough overlap
        displets_in{i}.s.x(4) < max_dist                 % is not too far
        displets_out{end+1} = displets_in{i};
    end
end
