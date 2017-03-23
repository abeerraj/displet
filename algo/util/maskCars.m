
function  aleM = maskCars(semI)
    szI = size(semI);
    semI_2d = reshape(semI, szI(1)*szI(2), szI(3));
    aleM = semI_2d(:,1) == 64 & semI_2d(:,2) == 0 & semI_2d(:,3) == 128; 
    aleM = reshape(aleM, szI(1), szI(2));
end