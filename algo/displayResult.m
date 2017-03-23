% input image (top-left), initial disparity image (top-right), 
% resulting disparity image (bottom-left), and error image (bottom right, only for training) 

function displayI = displayResult (imgs, params, resultDispI)

    if isfield(imgs, 'gtDispI')
        [~, errI] = disp_error(imgs.gtDispI, resultDispI, params.tau);
    else
        errI = zeros(size(imgs.colorI));
    end
    
    tl = im2double(imgs.colorI);
    tr = disp_to_color(imgs.dispI);
    bl = disp_to_color(resultDispI); 
    br = errI;

    displayI = [im2uint8(tl), im2uint8(tr); im2uint8(bl), im2uint8(br)];
end

