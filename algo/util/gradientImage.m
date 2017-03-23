function gradI = gradientImage (colorI)

% if not double, convert to double
if ~isfloat(colorI)
    colorI = im2double(colorI);
end

gradI(:,:,1) = channelGradient(shiftdim(colorI(:,:,1)));
gradI(:,:,2) = channelGradient(shiftdim(colorI(:,:,2)));
gradI(:,:,3) = channelGradient(shiftdim(colorI(:,:,3)));
gradI(:,:,4) = channelGradient(rgb2gray(colorI));
gradI = max(gradI,[],3);

gradI = (max(min(gradI,5),2)-2)/3;
%gradI = double(gradI>2);

%se = strel('disk',2,0);
%gradI = imdilate(gradI,se);

if 0
    figure,imagesc(gradI);
    keyboard;
    %figure,imagesc(channelGradient(rgb2gray(colorI)));
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function gradC = channelGradient (grayI)


%kernel = [-1 0 1 ; -2 0 2; -1 0 1];
kernel = [2   1   0   -1  -2; 3   2   0   -2  -3; 4   3   0   -3  -4; 3   2   0   -2  -3;  2   1   0   -1  -2];

gradU = conv2(grayI,kernel,'same');
gradV = conv2(grayI,kernel','same');

gradC = sqrt(gradU.*gradU+gradV.*gradV);
