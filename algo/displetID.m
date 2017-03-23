% returns id of a displet given its region number r and displet number d
% like sub2ind, from 2D to 1D, for a displet-specific ID.
% assumes 8 displets per region.

function id = displetID(r, d)
    id = (r-1) * 8 + d;
end