function mask_new3= rotate3D(mask,rot_x,rot_y,rot_z,vC);

% rotate about z for axial
for ii=1:vC(3);
    mask_new(:,:,ii)= imrotate_dec(mask(:,:,ii),-rot_z); %,'bilinear','crop');
end

% rotate about x for sagittal
for jj=1:vC(2)
    tmp= imrotate_dec(squeeze(mask_new(:,jj,:)),-rot_x); %,'bilinear','crop');
%     size(tmp)
%     size(reshape(tmp,vC(1),1,vC(3)))
    mask_new2(:,jj,:)= reshape(tmp,vC(1),1,vC(3));
end

% rotate about y for coronal
for kk=1:vC(1)
    tmp= imrotate_dec(squeeze(mask_new2(kk,:,:)),-rot_y); %,'bilinear','crop');
    mask_new3(kk,:,:)= reshape(tmp,1,vC(2),vC(3));
end


function out= imrotate_dec(I,deg);

[x, y] = meshgrid(1:size(I, 2), 1:size(I, 1));

deg= deg * pi / 180;

% Compute the rotation matrix
R = [ cos(deg) sin(deg); 
     -sin(deg) cos(deg)];

xy = [x(:), y(:)];

% Compute the middle point
mid = mean(xy, 1);

% Subtract off the middle point
xy = bsxfun(@minus, xy, mid);

% Rotate all of these coordinates by the desired angle
xyrot = xy * R;

% Reshape the coordinate matrices
xy = reshape(xy, [size(x), 2]);
xyrot = reshape(xyrot, [size(x), 2]);

% Interpolate the image data at the rotated coordinates
out = interp2(xy(:,:,1), xy(:,:,2), I, xyrot(:,:,1), xyrot(:,:,2));
end
end