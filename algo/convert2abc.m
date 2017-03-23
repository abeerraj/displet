function abc = convert2abc(abg, centers)
abc = abg;

for p = 1:length(centers)
    alpha = abg(1,:,p);
    beta = abg(2,:,p);
    gamma = abg(3,:,p);
    cx = centers(p,1);
    cy = centers(p,2);
    abc(3,:,p) = gamma - alpha * cx - beta * cy; 
end