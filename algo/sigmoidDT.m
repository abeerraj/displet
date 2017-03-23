% [8, 0.8]
function y = sigmoidDT(x, params)

y = 1./(1+exp(params(1)-params(2)*x));
