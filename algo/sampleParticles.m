% copies plane hypotheses from neighboring superpixels 
% with the probabilities based on color and location 

function sampledParticles = sampleParticles(plane, colorLocProbs, numSamples, planes_abg, centers)
    sampledIdx = discretesample(colorLocProbs, numSamples);
    shifted = sampledIdx >= plane;
    sampledIdx(shifted) = sampledIdx(shifted) + 1; 

    % first converted from alpha-beta-gamma to a-b-c representation
    particles_abc = abg2abc(planes_abg(sampledIdx,:), centers(sampledIdx,:));
    
    % then converted back to alpha-beta-gamma for the query plane
    sampledParticles = abc2abg(particles_abc, repmat(centers(plane,:), length(sampledIdx), 1));
end