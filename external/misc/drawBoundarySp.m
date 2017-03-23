function boundaryI = drawBoundarySp(colorI, segI, color)
    eightNeighborTotal = 8;
    eightNeighborOffsetX = [-1,-1, 0, 1, 1, 1, 0,-1];
    eightNeighborOffsetY = [0,-1,-1,-1, 0, 1, 1, 1];

    [height, width] = size(segI);
    boundaryI = colorI;
    
    for y = 1:height
        for x = 1:width
            pixelLabelIndex = segI(y,x);
            
            drawBoundary = 0;
            for neighborIndex = 1:eightNeighborTotal
                neighborX = x + eightNeighborOffsetX(neighborIndex);
                neighborY = y + eightNeighborOffsetY(neighborIndex);
                if (neighborX < 1 || neighborX > width || neighborY < 1 || neighborY > height) 
                    continue;
                end
                
                if (segI(neighborY,neighborX) ~= pixelLabelIndex) 
                    drawBoundary = 1;
                    break;
                end
            end
            
            if (drawBoundary)
                boundaryI(y,x,:) = color;
            end
        end
    end
end