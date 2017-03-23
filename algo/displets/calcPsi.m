function psi = calcPsi (s)

global data

% render model
[tr,scale] = poseFromSample(s);
[Dr,Mr] = renderModel(tr,scale,data.model,data.calib);
Dr(~Mr) = -inf;

% explain as much of Ms as possible
Ss = data.Ms & data.Ds>0; % instance label with valid disparities
ds = data.Ds(Ss);
dr = Dr(Ss);
d_num = length(dr);
d_err = min(abs(ds-dr),3)/3;
psi = 10*sum(d_err)/d_num;

% maximize intersection-over-union
if 0
    X = Mr & data.Ds>0 & data.Ds<Dr+5; % non-occluded part of rendered mask
    iou = length(find(X&data.Ms))/(length(find(X|data.Ms))+eps);
    psi = psi - 5*iou;
else
    X = Mr & data.Ds>0 & data.Ds<Dr+5; % non-occluded part of rendered mask
    if length(find(X&data.Ms))/length(find(data.Ms))<0.2
        psi = 10000;
    end
end

% occlusion penalty
psi = psi + 0.005*length(find(Mr&data.Ds>0&Dr>data.Ds+10));
