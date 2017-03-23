function normals = convert2normals(particles, K, baseline)

[~, numParticles, numPlanes] = size(particles);
normals = zeros(3, numParticles, numPlanes);

for p = 1:numPlanes
    for r = 1:numParticles
        normals(:, r, p) = dispPlaneToNormal(particles(:,r,p), K, baseline);
    end
end
