% conversion from a-b-c to alpha-beta-gamma representation

function abg = abc2abg(abc, centers)
abg = abc;

for p = 1:size(centers,1)
    a = abc(p,1);
    b = abc(p,2);
    c = abc(p,3);
    cx = centers(p,1);
    cy = centers(p,2);
    abg(p,3) = c + a * cx + b * cy; 
end