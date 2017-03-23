% adds displet hypotheses to the particles associated with displet's superpixels
% sets the particleOrigins of the updated super-pixels to the corresponding displetID. 
% updates numFilled array for the updated superpixels

function [particles, numFilled, particleOrigins] = addDispletHypos(particles, displets)
    numParticles = size(particles, 2);
    numPlanes = size(particles, 3);
    numFilled = zeros(numPlanes, 1);
    particleOrigins = zeros(numParticles, numPlanes);
    
    for r = 1:length(displets) % for each region
        for d = 1:length(displets{r}) % for each displet in the region
            displet = displets{r}{d};
            for s = 1:length(displet.sp) % for each superpixel of the displet
                sp = displet.sp(s);
                
                if numFilled(sp) < numParticles-1
                    % update number of filled particles for the superpixel
                    numFilled(sp) = numFilled(sp) + 1;

                    % add the displet plane hypothesis as particle
                    particles(:, numFilled(sp), sp) = [displet.p(s,1), displet.p(s,2), displet.p(s,3)];

                    % mark the origin of particle with the displet-specific ID.
                    particleOrigins(numFilled(sp), sp) = displetID(r,d);
                end
            end
        end
    end
end