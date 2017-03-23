function s_map = sampling (num_samples,verbose)

global data

% init sample
pix = samplePixelFromMask(data.Ms);
pos = posFromPixel(pix);
s.x = [data.orientation pos(1) 0 pos(2)];

% init figure
if verbose>=2
  h.f = figure('Position',[580 300 680 680]);
  h.a = axes('Position',[0 0 1 1]);
  axis off;
  plotModelFit(s.x);
  refresh; pause(0.1);
end

% init psi / map sample
s.psi = calcPsi(s.x);
s_map = s;

% for all samples do
move_stats = [];
for iter=1:num_samples

  % propose new sample
  [s_,logq] = createProposal(s,iter>num_samples/10);
  s_.psi = calcPsi(s_.x);

  % accept?
  T = 200/(200+iter);
  A = min(1,exp((logq+s.psi-s_.psi)/T));
  accept = rand<=A;
  if verbose>=1
    move_stats = updateMoveStatistics(move_stats,s_,accept);
  end
  if accept
    s = s_;
  end

  % update map and plot on improvement
  if s.psi<s_map.psi
    s_map = s;
    if verbose>=2
      printIter(iter,s_map);
      plotModelFit(s.x);
    end
  end
  
  % show sample every x iterations
  if verbose>=3 && mod(iter,10)==0
    printIter(iter,s_map);
    plotModelFit(s.x);
  end
end

if verbose>=1
  plotMoveStatistics(move_stats);
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function move_stats = updateMoveStatistics (move_stats,s,accept)

if ~isfield(move_stats,s.move)
  move_stats.(s.move) = zeros(1,2);
end

move_stats.(s.move)(2) = move_stats.(s.move)(2)+1;
if accept
  move_stats.(s.move)(1) = move_stats.(s.move)(1)+1;
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function plotMoveStatistics (move_stats)

fprintf('Move statistics:\n');
fields=sort(fieldnames(move_stats));
for i=1:length(fields)
  fn = fields{i};
  acc = move_stats.(fn)(1);
  all = move_stats.(fn)(2);
  fprintf('%s: %d/%d (%.2f %%)\n',fn,acc,all,100*acc/all);
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function printIter (iter,s_map)

fprintf('Iter: %d, Psi: %.2f, r: %.2f, t: %.2f %.2f %.2f\n',iter,s_map.psi,s_map.x(1),...
         s_map.x(2),s_map.x(3),s_map.x(4));
