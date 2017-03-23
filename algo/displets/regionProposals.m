function M = regionProposals (data,verbose)

Ds = data.Ds;
Ds(Ds<20) = nan;
Ds(~data.Ss_car) = nan;

Zs = data.calib.f*data.calib.base./Ds;
[Us,Vs] = meshgrid(0:size(data.Ds,2)-1,0:size(data.Ds,1)-1);
Xs = (data.calib.scale*Us-data.calib.cu).*Zs/data.calib.f;
Ys = (data.calib.scale*Vs-data.calib.cv).*Zs/data.calib.f;

Ss = data.Ss_car;
Ss(isnan(Ds)) = 0;
se = strel('disk',3,0);
Ss = imdilate(Ss,se);
Ss = imerode(Ss,se);
Cs = bwconncomp(Ss,8);

if verbose>=2
  figure,imagesc(data.Ss_car);
  figure,imagesc(Ss);
  keyboard;
end

M{1} = [];
M{2} = [];
for i=1:Cs.NumObjects
  
  idx_seg = Cs.PixelIdxList{i};
  if length(idx_seg)<50
    continue;
  end
  
  Ss_car = false(size(data.Ss_car));
  Ss_car(idx_seg) = true;
  idx_val = find(~isnan(Zs)&Ss_car&Ys<1);
  
  if verbose>=3
    Ds = zeros(size(data.Ds));
    Ds(idx_val) = data.Ds(idx_val);
    figure,imagesc(Ss_car);
    figure,imagesc(Ds);
    keyboard;
  end
  
  %%%% LONGITUDINAL  
  P = [Xs(idx_val) Ys(idx_val) Zs(idx_val)];
  med_val = median(P(:,1));
  idx_band = find(abs(P(:,1)-med_val)<2.5);
  P = P(idx_band,:);
  
  if size(P,1)<50
    continue;
  end
  
  x = 0:0.1:20;
  y = evaluate(kde(P(:,3)',0.7),x);
  [foo1,foo2,ymin,imin] = extrema(30*y);
  xmin = x(imin);
  
  if verbose>=1
    figure;
    plot(P(:,3),P(:,1),'.r','MarkerSize',1);
    axis equal; hold on;
    plot(x,30*y,'b-');
    plot(x(imin),ymin,'rx')
    keyboard;
  end

  for j=1:length(xmin)-1
    if verbose>=1
      fprintf('comp %d, min %d\n',i,j);
    end
    idx_ins = find(~isnan(Zs)&Ss_car&Zs>xmin(j)&Zs<xmin(j+1)...
                   &abs(Xs-med_val)<2.5 & Ys<1.5);
    if length(idx_ins)<50
      continue;
    end
    Ds = false(size(data.Ds));
    Ds(idx_ins) = true;
    if verbose>=2
      figure,imagesc(Ds);
      keyboard;
    end
    M{1}{end+1} = Ds;
  end
  
  %%%% LATERAL
  x = -5:0.1:5;
  P = [Xs(idx_val) Ys(idx_val) Zs(idx_val)];
  med_val = median(P(:,3));
  idx_band = find(abs(P(:,3)-med_val)<2);
  P = P(idx_band,:);
  
  if size(P,1)<50
    continue;
  end
  
  y = evaluate(kde(P(:,1)',0.4),x);
  [foo1,foo2,ymin,imin] = extrema(30*y);
  xmin = x(imin);
  
  if verbose>=1
    figure;
    plot(P(:,1),P(:,3),'.r','MarkerSize',1);
    axis equal; hold on;
    plot(x,30*y,'b-');
    plot(x(imin),ymin,'rx');
    keyboard;
  end

  for j=1:length(xmin)-1
    
    if verbose>=1
      fprintf('comp %d, min %d\n',i,j);
    end
    
    idx_ins = find(~isnan(Xs)&Ss_car&Xs>xmin(j)&Xs<xmin(j+1)...
                   &abs(Zs-med_val)<2 & Ys<1.3);
                 
    if length(idx_ins)<50
      continue;
    end
    Ds = false(size(data.Ds));
    Ds(idx_ins) = true;
    
    if verbose>=2
      figure,imagesc(Ds);
      keyboard;
    end
    
    redundant = 0;
    for k=1:length(M{1})
      if length(find(M{1}{k}&Ds))/length(find(M{1}{k}|Ds))>0.5
        redundant = 1;
        break;
      end
    end
    if ~redundant
      M{2}{end+1} = Ds;
    end
  end  
end
