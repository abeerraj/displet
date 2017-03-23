function pix = samplePixelFromMask (M)

idx = find(M);
i = randi(length(idx));
[v,u] = ind2sub(size(M),idx(i));
pix = [u v]-1;
