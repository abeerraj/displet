function [tr,scale] = poseFromSample (x)

tr = [0 x(1) 0 x(2:4)];
scale = [1 1 1];
