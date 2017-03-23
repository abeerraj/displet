function [d_err,err] = disp_error(D_gt,D_est,tau)

E = abs(D_gt-D_est);
E(D_gt<=0) = 0;
greaterThanTau = find(E>tau);
d_err = length(greaterThanTau)/length(find(D_gt>0));

E(E>5.0) = 5.0;
E = E./ 5.0; 
err = repmat(E,[1 1 3]);

err_ = reshape(err, [size(E,1)*size(E,2),3]);
err_(greaterThanTau, :) = repmat([1,0,0], [length(greaterThanTau),1]);

err = reshape(err_, size(err));


