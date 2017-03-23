% conversion from alpha-beta-gamma to a-b-c representation

function abc = abg2abc(abg, centers)
abc = abg;

for p = 1:size(centers,1)
    alpha = abg(p,1);
    beta = abg(p,2);
    gamma = abg(p,3);
    cx = centers(p,1);
    cy = centers(p,2);
    abc(p,3) = gamma - alpha * cx - beta * cy; 
end
