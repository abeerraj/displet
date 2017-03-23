function [s_,logq] = createProposal (s,sample_orientation)

global data

% copy sample and init log proposal probability
s_ = s;
logq = 0;

switch randi(2)
  
  % global move
  case 1
    
    s_.move = 'global';
    
    % parameters of normal distribution
    pix = samplePixelFromMask(data.Ms);
    mu = posFromPixel(pix);
    sigma = 0.5*[1 1];
    
    % draw location from normal distribution
    s_.x = [data.orientation 0 0 0];
    s_.x([2 4]) = mvnrnd(mu,sigma);
    
    % log proposal ratio
    logq = logq + mvnlogpdf(s_.x([2 4]),mu,sigma)-mvnlogpdf(s.x([2 4]),mu,sigma);
    
  % local move
  case 2
    
    % randomly sample blocks
    active = de2bi(randi(15),4);
    s_.move = ['local_' sprintf('%d',active)];
    
    % parameters of student-t distribution
    sigma = [0.1 0.1 0.01 0.1].^2;
    nu = 1;

    % perturb parameter vector according to student-t distribution
    s_.x = s.x + sqrt(sigma).*mvtrnd(diag(sigma),nu);
    
    % replace inactive parameters with old values
    s_.x(~active) = s.x(~active);
end

% hard limits
s_.x(1) = min(max(s_.x(1),data.orientation-pi/4),data.orientation+pi/4);
s_.x(3) = min(max(s_.x(3),-0.2),0.2);

if ~sample_orientation
  s_.x(1) = data.orientation;
end

%     % translate along principal axis
%       sigma1 = 0.2.^2;
%       sigma2 = 0.1.^2;
%       nu = 1;
%       v = [cos(s.x(2)) sin(s.x(2))];
%       w = [v(2) -v(1)];
%       s_.x([4 6]) = s.x([4 6])+sqrt(sigma1)*mvtrnd(sigma1,nu)*v ...
%                               +sqrt(sigma2)*mvtrnd(sigma2,nu)*w
