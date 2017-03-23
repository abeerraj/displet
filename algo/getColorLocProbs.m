function colorLocProbMat_n = getColorLocProbs(colorI, spI)
    sigma_color = 8;
    sigma_loc = 200;

    colorTransform = makecform('srgb2lab');
    colorI_lab = applycform(colorI, colorTransform);
    szI = size(spI);
    colors = reshape(colorI_lab, szI(1)*szI(2), 3);
    colors(:,1) = colors(:,1) * 0.5; 
    
    numPlanes = double(max(spI(:))) + 1;
    means = zeros(numPlanes, 3);
    centers = zeros(numPlanes, 2);
    
    for i = 1:numPlanes
        idx = find(spI == i-1);
        [v, u] = ind2sub(szI, idx);   
        centers(i,:) = [round(median(u)), round(median(v))];
        means(i,:) = mean(colors(idx,:));
    end

    colorDistMat = pdist2(means, means);
    colorDistMat = triu(colorDistMat);
    
    locDistMat = triu(pdist2(centers, centers));
    colorLocProbMat = exp(-((colorDistMat.^2) ./ (2*sigma_color^2) + (locDistMat.^2) ./ (2*sigma_loc^2))); 
    colorLocProbMat = triu(colorLocProbMat, 1);

    colorLocProbMat_n = zeros(numPlanes, numPlanes-1);
    for i = 1:numPlanes
        out_i = [colorLocProbMat(1:i-1, i)', colorLocProbMat(i, i+1:end)];
        colorLocProbMat_n(i,:) = (out_i - min(out_i)) / (max(out_i) - min(out_i));
    end
    
    if 0
        boundaryI = drawBoundarySp(colorI, spI+1, [255,0,0]);
        imageHandle = imshow(boundaryI);
        set(imageHandle, 'ButtonDownFcn', @ImageClickCallback);
    end

    function ImageClickCallback(objectHandle, ~)
        axesHandle = get(objectHandle, 'Parent');
        coordinates = get(axesHandle, 'CurrentPoint');
        coordinates = round(coordinates(1,1:2));
        clickedSeg = spI(coordinates(2), coordinates(1))+1;

        for c = 1:length(centers)
            if c < clickedSeg
                prob = colorLocProbMat_n(clickedSeg, c);
            else
                prob = colorLocProbMat_n(clickedSeg, c-1);
            end

            if(clickedSeg == c)
                color = 'green';
            elseif(prob < 0.5)
                color = 'cyan';
            else
                color = 'magenta';
            end
            t(c) = text(centers(c,1)-5, centers(c,2), sprintf('%.02f', prob), 'color', color);
        end

        pause(5);
        delete(t);     
    end
end