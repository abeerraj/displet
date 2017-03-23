function p = posFromPixel(pix)

global data

d = data.Ds(pix(2)+1,pix(1)+1);

if d<0
  error('Invalid disparity!');
end

z = data.calib.base*data.calib.f/d;
x = (pix(1)*data.calib.scale-data.calib.cu)*z/data.calib.f;
v = [x z]/norm([x z]);
p = [x z]+1*v;
