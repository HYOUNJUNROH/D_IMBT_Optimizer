function [C,Cinfo,Z] = get_CT(ctfldr)

%% [C,Cinfo,Z] = get_CT(ctfldr)
% get_CT.m opens the CT dicom images in the directory, and provides useful
% information of CT images
% 
% INPUT
%   ctfldr: Location of the directory containing .dcm files 
% OUTPUT
%   C: CT images (2D transverse planes stacked in 3D)
%   Cinfo: dicominfo (provided by toolbox in matlab)
%   Z: slice position in z-direction

%%
cd(ctfldr)
%clear
A = dir;
k = length(A);
sflag = 0;
tslcloc = Inf;
for a = 1:k
    if mod(a,10)==0
        display(['Loading ..... ' num2str(round(a/k*100)) '% Complete'])
    end
    try
        I = dicominfo(A(a,1).name);
        B = double(dicomread(A(a,1).name));
        if sflag == 0
            vB = size(B);
            C = zeros(vB(1),vB(2),k);
            inCT = zeros(1,k);
            Z = zeros(1,k);
            sflag = 1;
        end
        C(:,:,I.InstanceNumber) = B;
        Z(I.InstanceNumber) = I.ImagePositionPatient(3);
        inCT(I.InstanceNumber) = 1;
        if Z(I.InstanceNumber)<tslcloc
            tslcloc = Z(I.InstanceNumber);
            Cinfo = I;
        end
    end
end

vCT = find(inCT);
C = C(:,:,vCT);
Z = Z(vCT);
%%%%added the following for DIRQA
[ZS,IS] = sort(Z,'ascend');
Z = ZS;
C = C(:,:,IS);

