function [PTV, bladder, rectum, bowel, sigmoid, body, ind_bodyd] = create_dosematrix_pure_matlab(PatientID, CTData, StructData, Catheter, AngleResolution)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Create_dosematrix.m
%
% This is to create dose kernel matrix for optimization
%   In an equation below: A_i x= d_i  
%   This code gives A_i (dose kernel matrix for each structure i)
%  
% Given RT image, structure, plan, and dose DICOM files,
% it generates dose kernal matrix
%
% The challenging point is that the structures were delineated,
% when the images were slightly rotated
% Thus, it should be compensated for correction
%
% It is still uncertain how to determine the z coordinates 
% Here, top z-axis location(zC_top) was referred to the coordinates in Eclipse
%
% ---------------------------------------------------------
% Input: RT image, structure, dose and plan DICOM files
%        systemA_2nd   (Tandem MC simulated dose),
%        systemA_ovoid(Num)_left/right  (Ovoid MC simulated dose (left/right))
%                                        (Num= 1~5)
%----------------------------------------------------------
% Output: Dose kernal matrices, denoted by 
%         [PTV, bladder, rectum, bowel,sigmoid, body]
% 
%   A= [ dose elemtns for Tandem-associated elements      dose elemtns for Ovoid-associated elements ]
%   x= [ Tandem-associate elements   Ovoid-associated elements ]
%   d= Ax
%
%  Tandem and Ovoid dose matrix elements were normalized, such that
%  activity of tandem (sum of all elemments) = activity of ovoid 
% ---------------------------------------------------------
%
%                                                     2021.12  Hojin Kim
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clearvars -except PatientID CTData StructData Catheter AngleResolution
close all;
fprintf("Patient ID = %s\n", PatientID);
fprintf("Start = %d\n\n", tic);
fprintf("AngleResolution = %d\n\n", AngleResolution);

workfolder = [pwd '\Matlab_workfolder']


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
zC_top= 106.0;  % from Eclipse (reverse of the bottom on display)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

flag_body=1;

%CTData
Cp0 = CTData.ImagePositionPatient;
Csp = CTData.PixelSpacing;
Cth = CTData.SliceThickness;
Cpor = CTData.ImageOrientationPatient;


%Norm_t = [(Cpor(2))*(Cpor(6)) - (Cpor(3))*(Cpor(5)) 
%           (Cpor(3))*(Cpor(4)) - (Cpor(1))*(Cpor(6)) 
%           (Cpor(1))*(Cpor(5)) - (Cpor(2))*(Cpor(4))];
 
%CTData.ImagePositionPatient(1) = CTData.ImagePositionPatient(1)- Cth*Norm_t(1) * (size(CTData.C,3)-1);
%CTData.ImagePositionPatient(2) = CTData.ImagePositionPatient(2) - Cth*Norm_t(2) * (size(CTData.C,3)-1);
%CTData.ImagePositionPatient(3) = CTData.ImagePositionPatient(3) - Cth*Norm_t(3) * (size(CTData.C,3)-1);


fprintf('ImagePositionPatient = [%f %f %f]\n', CTData.ImagePositionPatient);
fprintf('PixelSpacing = [%f %f]\n', CTData.PixelSpacing);
fprintf('SliceThickness = [%f]\n', CTData.SliceThickness);
fprintf('ImageOrientationPatient = [%f %f %f %f %f %f]\n', CTData.ImageOrientationPatient);

xC_orig = double(CTData.xPosition);
yC_orig = double(CTData.yPosition);
zC_orig = double(CTData.zPosition)
%zC_orig = flip(double(CTData.zPosition)); % zC's order is different, thus it should be flipped.

%Struct Data
strucname = StructData.structname
x = StructData.x
y = StructData.y;
z = StructData.z;

%sliceThickness = zC_orig(2) - zC_orig(1);
CTData.C= flipdim(CTData.C,3);

% check 3d image
%figure, imshow3D(CTData.C);

% creates new dose grid
vC = size(CTData.C);

% % check Struct Data
% StructData.x
% StructData.y
% StructData.structname
% 
% % check Catheter Data
% Catheter.ch1;
% Catheter.ch2;
% Catheter.ch3;

% zC_orig= zC;


%% Read structures and make binary images
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%load structures
%--- [x,y,z,strucname] = get_structure_v2(strucf);
%--- strucname


%% Resampling the calculated MC results to the MR images ==> making beamlet-kernel for each structure
fprintf("-- Making beamlet-kernel\n");
brachy_opt_path = [workfolder '\MR' '\Brachy_opt\' PatientID];

if exist(brachy_opt_path, 'dir') == 0
    mkdir(brachy_opt_path);
end

cd(brachy_opt_path);

%% Catheter Information
ss1 = Catheter.ch1
ss2 = Catheter.ch2
ss3 = Catheter.ch3
%load test_ss1.mat;
%load test_ss2.mat;
%load test_ss3.mat;

%DisplayCatheter(ss1,ss2,ss3);

tandem = ss3;

%% Coordinate system rotate in opposing direction as computed
theta_x= acos(CTData.ImageOrientationPatient(5))*180/pi 
theta_y= acos(CTData.ImageOrientationPatient(3))*180/pi-90 
theta_z= acos(CTData.ImageOrientationPatient(2))*180/pi-90

rot_x=[ 1 0 0; 0 cos(theta_x*pi/180) -sin(theta_x*pi/180); 0 sin(theta_x*pi/180) cos(theta_x*pi/180) ]; %theta_yz = theta_x
rot_y= [ cos(theta_y*pi/180) 0 sin(theta_y*pi/180) ;  0 1 0; -sin(theta_y*pi/180) 0 cos(theta_y*pi/180) ]; % theta_xz= theta_y
rot_z= [ cos(theta_z*pi/180) -sin(theta_z*pi/180) 0; sin(theta_z*pi/180) cos(theta_z*pi/180) 0; 0 0 1 ];
 
% new catheter point location on image coord
ss1new= ss1*rot_x*rot_y*rot_z  % Ovoid R or Ring
ss2new= ss2*rot_x*rot_y*rot_z  % Ovoid L
tandem= tandem*rot_x*rot_y*rot_z  % Tandem

coord1= CTData.ImagePositionPatient;
coord1new= coord1*rot_x*rot_y*rot_z;

init_x= coord1new(:,1);
init_y= coord1new(:,2);

dx= CTData.PixelSpacing(1);
dy= CTData.PixelSpacing(2);

xC= init_x:dx:init_x+dx*511;
yC= init_y:dy:init_y+dy*511;
xC= xC-1;
yC= yC+1;
zC= zC_top:-mean(diff(zC_orig)):zC_top-mean(diff(zC_orig))*(vC(3)-1);
%zC= zC_orig;

[XCT YCT]= meshgrid(xC,yC);
[Xc Yc Zc]= meshgrid(xC,yC,zC); % coordinate for contouring

%% 
hist_zloc=[];
for a = 1:length(strucname); %[ 2 10 13 16 19 23 30:33 36 41:43 ];
    if strcmp(strucname(a),'BODY') 
        
        display(['Processing ' strucname{a} '...'])
        xtemp = x{a};
        ytemp = y{a};
        ztemp = z{a};
        ind_body = [];
       
        for b = 1:length(xtemp)
            locs=[ xtemp{b} ytemp{b} ztemp{b} ];
            locs_new= locs*rot_x*rot_y*rot_z;
            %tempx= locs_new(:,1); 
            %tempy= locs_new(:,2); 
            tempz= locs_new(:,3);
            
            zloc = tempz(1);
            zloc = round(1000*zloc)/1000;
            hist_zloc= [hist_zloc zloc];

            %{
            citemp = find(abs(zC-zloc)<=1.8);
            %iorgan = find(inpolygon(XCT,YCT,tempx,tempy));
            iorgan = find(insidepoly(XCT,YCT,tempx,tempy));
            ind_body = [ind_body; iorgan+(citemp-1)*vC(1)*vC(2)];
            %}
        end       
    else
        continue;
    end
    
end

% save hist_zloc_patient4 hist_zloc
% load hist_zloc_patient4 %Haksoo
z_trans= zC_top-max(hist_zloc);
tandem(:,3)= tandem(:,3)+z_trans;
hist_zloc_body= hist_zloc+z_trans;
hist_zloc_body= sort(hist_zloc_body,'descend');


%% Contour points should be rotated 
hist_zloc=[];

x_trans=-1;
y_trans=1;

thres= 1.5;
for a = 1:length(strucname); %[ 2 10 13 16 19 23 30:33 36 41:43 ];
     if strcmp(strucname(a),'HR-CTV') 
        
        display(['Processing ' strucname{a} '...'])
        xtemp = x{a};
        ytemp = y{a};
        ztemp = z{a};
        ind_PTV = [];
       
        for b = 1:length(xtemp)
            locs=[ xtemp{b} ytemp{b} ztemp{b} ];
            locs_new= locs*rot_x*rot_y*rot_z;
            tempx= locs_new(:,1)+x_trans; tempy= locs_new(:,2)+y_trans; tempz= locs_new(:,3)+z_trans;
            
            zloc = tempz(1);
            zloc = round(1000*zloc)/1000;
%             zloc= zloc-z_compensate
            hist_zloc=[ hist_zloc zloc ];
            citemp = find(abs(hist_zloc_body-zloc)<=1)
            
            %iorgan = find(inpolygon(XCT,YCT,tempx,tempy));
            iorgan = find(insidepoly(XCT,YCT,tempx,tempy));
            ind_PTV = [ind_PTV; iorgan+(citemp-1)*vC(1)*vC(2)];
        end   
     elseif strcmp(strucname(a),'_HR_25mm') 
            
        display(['Processing ' strucname{a} '...'])
        xtemp = x{a};
        ytemp = y{a};
        ztemp = z{a};
        ind_tuning = [];
       
        for b = 1:length(xtemp)
            locs=[ xtemp{b} ytemp{b} ztemp{b} ];
            locs_new= locs*rot_x*rot_y*rot_z;
            tempx= locs_new(:,1)+x_trans; tempy= locs_new(:,2)+y_trans; tempz= locs_new(:,3)+z_trans;
            
            zloc = tempz(1);
            zloc = round(1000*zloc)/1000;
%             zloc= zloc-z_compensate
            hist_zloc=[ hist_zloc zloc ];
            citemp = find(abs(hist_zloc_body-zloc)<=1)
            
            %iorgan = find(inpolygon(XCT,YCT,tempx,tempy));
            iorgan = find(insidepoly(XCT,YCT,tempx,tempy));
            ind_tuning = [ind_tuning ; iorgan+(citemp-1)*vC(1)*vC(2)];
        end
        
    elseif strcmp(strucname(a),'BLADDER')   
        display(['Processing ' strucname{a} '...'])
        xtemp = x{a};
        ytemp = y{a};
        ztemp = z{a};
        ind_bladder = [];
       
        for b = 1:length(xtemp)
            locs=[ xtemp{b} ytemp{b} ztemp{b} ];
            locs_new= locs*rot_x*rot_y*rot_z;
            tempx= locs_new(:,1)+x_trans; tempy= locs_new(:,2)+y_trans; tempz= locs_new(:,3)+z_trans;
            
            zloc = tempz(1);
            zloc = round(1000*zloc)/1000;
%             zloc= zloc-z_compensate
            hist_zloc=[ hist_zloc zloc ];
            citemp = find(abs(hist_zloc_body-zloc)<=1)
            
            %iorgan = find(inpolygon(XCT,YCT,tempx,tempy));
            iorgan = find(insidepoly(XCT,YCT,tempx,tempy));
            ind_bladder = [ind_bladder ; iorgan+(citemp-1)*vC(1)*vC(2)];
        end
    elseif strcmp(strucname(a),'RECTUM') 
        
        display(['Processing ' strucname{a} '...'])
        xtemp = x{a};
        ytemp = y{a};
        ztemp = z{a};
        ind_rectum = [];
       
        for b = 1:length(xtemp)
            locs=[ xtemp{b} ytemp{b} ztemp{b} ];
            locs_new= locs*rot_x*rot_y*rot_z;
            tempx= locs_new(:,1)+x_trans; tempy= locs_new(:,2)+y_trans; tempz= locs_new(:,3)+z_trans;
            
            zloc = tempz(1);
            zloc = round(1000*zloc)/1000;
%             zloc= zloc-z_compensate
            hist_zloc=[ hist_zloc zloc ];
            citemp = find(abs(hist_zloc_body-zloc)<=1)
            
            %iorgan = find(inpolygon(XCT,YCT,tempx,tempy));
            iorgan = find(insidepoly(XCT,YCT,tempx,tempy));
            ind_rectum = [ind_rectum; iorgan+(citemp-1)*vC(1)*vC(2)];
        end
    elseif strcmp(strucname(a),'BOWEL') 
        
        display(['Processing ' strucname{a} '...'])
        xtemp = x{a};
        ytemp = y{a};
        ztemp = z{a};
        ind_bowel = [];
       
        for b = 1:length(xtemp)
            locs=[ xtemp{b} ytemp{b} ztemp{b} ];
            locs_new= locs*rot_x*rot_y*rot_z;
            tempx= locs_new(:,1)+x_trans; tempy= locs_new(:,2)+y_trans; tempz= locs_new(:,3)+z_trans;
            
            zloc = tempz(1);
            zloc = round(1000*zloc)/1000;
%             zloc= zloc-z_compensate
            hist_zloc=[ hist_zloc zloc ];
            citemp = find(abs(hist_zloc_body-zloc)<=1)
            
            %iorgan = find(inpolygon(XCT,YCT,tempx,tempy));
            iorgan = find(insidepoly(XCT,YCT,tempx,tempy));
            ind_bowel = [ind_bowel ; iorgan+(citemp-1)*vC(1)*vC(2)];
        end
    elseif strcmp(strucname(a),'SIGMOID') 
        
        display(['Processing ' strucname{a} '...'])
        xtemp = x{a};
        ytemp = y{a};
        ztemp = z{a};
        ind_sigmoid = [];
       
        for b = 1:length(xtemp)
            locs=[ xtemp{b} ytemp{b} ztemp{b} ];
            locs_new= locs*rot_x*rot_y*rot_z;
            tempx= locs_new(:,1)+x_trans; tempy= locs_new(:,2)+y_trans; tempz= locs_new(:,3)+z_trans;
            
            zloc = tempz(1);
            zloc = round(1000*zloc)/1000;
%             zloc= zloc-z_compensate
            hist_zloc=[ hist_zloc zloc ];
            citemp = find(abs(hist_zloc_body-zloc)<=1)
            
            %iorgan = find(inpolygon(XCT,YCT,tempx,tempy));
            iorgan = find(insidepoly(XCT,YCT,tempx,tempy));
            ind_sigmoid = [ind_sigmoid; iorgan+(citemp-1)*vC(1)*vC(2)];
        end  
    elseif strcmp(strucname(a),'BODY') 
        if flag_body==1;
            display(['Processing ' strucname{a} '...'])
            xtemp = x{a};
            ytemp = y{a};
            ztemp = z{a};
            ind_body = [];

            for b = 1:length(xtemp)
                locs=[ xtemp{b} ytemp{b} ztemp{b} ];
                locs_new= locs*rot_x*rot_y*rot_z;
                tempx= locs_new(:,1)+x_trans; tempy= locs_new(:,2)+y_trans; tempz= locs_new(:,3)+z_trans;

                zloc = tempz(1);
                zloc = round(1000*zloc)/1000;
                citemp = find(abs(hist_zloc_body-zloc)<1);

                %iorgan = find(inpolygon(XCT,YCT,tempx,tempy));
                iorgan = find(insidepoly(XCT,YCT,tempx,tempy));
                ind_body = [ind_body; iorgan+(citemp-1)*vC(1)*vC(2)];
            end  
        end
    else
        continue;
    end
    
end

%% Binary Masking image for each structure
mask_PTV=zeros(vC);
mask_PTVorig=zeros(vC);
% mask_tuning=zeros(vC);
mask_bladder=zeros(vC);
mask_rectum=zeros(vC);
mask_bowel=zeros(vC);
mask_sigmoid=zeros(vC);
% mask_ring=zeros(vC);

mask_PTV(ind_PTV)=1;
ind= find(mask_PTV~=0);
min_ind= ceil(min(ind)/(512*512))
max_ind= ceil(max(ind)/(512*512))
zCmin= min(zC(min_ind),zC(max_ind))
zCmax= max(zC(min_ind),zC(max_ind))

mask_bladder(ind_bladder)=1;
mask_rectum(ind_rectum)=1;
mask_bowel(ind_bowel)=1;
mask_sigmoid(ind_sigmoid)=1;
% mask_ring(ind_ring)=1;
if flag_body==1;
    mask_body= zeros(vC);
    mask_body(ind_body)=1;
end

%% Downsampled for each mask image 
%-- Body down x4 , others down-sampled by x2
mask_bodyd= zeros(vC(1)/4,vC(2)/4,vC(3));
for ss=1:vC(3);
    if flag_body==1;
        mask_bodyd(:,:,ss)= mask_body(1:4:end,1:4:end,ss);
    end
    mask_PTVd(:,:,ss)= mask_PTV(1:2:end,1:2:end,ss);
    mask_bladderd(:,:,ss)= mask_bladder(1:2:end,1:2:end,ss);
    mask_rectumd(:,:,ss)= mask_rectum(1:2:end,1:2:end,ss);
    mask_boweld(:,:,ss)= mask_bowel(1:2:end,1:2:end,ss);
    mask_sigmoidd(:,:,ss)= mask_sigmoid(1:2:end,1:2:end,ss);
end
if flag_body==1;
    ind_bodyd= find(mask_bodyd~=0);
end
ind_PTVd= find(mask_PTVd~=0);
ind_bladderd= find(mask_bladderd~=0);
ind_rectumd= find(mask_rectumd~=0);
ind_boweld= find(mask_boweld~=0);
ind_sigmoidd= find(mask_sigmoidd~=0);


%% Define B and C in tandem
%  --> Determine available source positions
%
% ----------- This is a case-specific: OLD: 2024-09-29
% It assumed 
%     1. Tandem inferior end -- 10mm distance -- Ovoid
%     2. Ovoid MC calcuation was simulated, where (x,y,z)=(0,0,0)
%        corresponds to middle of tandem applicator
%      (Ex) tandem is 6.5cm long,  
%          ==> zeros of Ovoid MC were located 4.25cm away from Ovoid
%
% With Infinit TPS
%     1. Actual Dwell Positions are used
%     2. Ovoid MC calcuation was simulated, where (x,y,z)=(0,0,0)
%        corresponds to middle of tandem applicator
%      (Ex) tandem is 6.5cm long,  
%          ==> zeros of Ovoid MC were located 4.25cm away from Ovoid
%


tandem_new = tandem(1:3,:);

diff_tandem= diff(tandem_new);
dist= sqrt(diff_tandem(:,1).^2+diff_tandem(:,2).^2+diff_tandem(:,3).^2)
dist_tot= dist(1); %sum(dist);

pE(1)= tandem_new(1,1)+ 3*(tandem_new(2,1)-tandem_new(1,1));
pE(2)= tandem_new(1,2)+ 3*(tandem_new(2,2)-tandem_new(1,2));
pE(3)= tandem_new(1,3)+ 3*(tandem_new(2,3)-tandem_new(1,3));

pA(1)= tandem_new(1,1); pA(2)= tandem_new(1,2); pA(3)= tandem_new(1,3);

candB_x= pA(1):(pE(1)-pA(1))/1e3:pE(1); 
candB_y= pA(2):(pE(2)-pA(2))/1e3:pE(2); 
candB_z= pA(3):(pE(3)-pA(3))/1e3:pE(3); 

dist_AB=[];
temp= (candB_x'-pA(1)).^2 + (candB_y'-pA(2)).^2 + (candB_z'-pA(3)).^2 ;
temp_sum= sum(temp,2);
dist_AB= sqrt(temp_sum);

% ---- find pointB
ind65= find( abs(dist_AB-65)==min(abs(dist_AB-65)) );
pB(1)= candB_x(ind65); pB(2)= candB_y(ind65); pB(3)= candB_z(ind65);
sqrt(sum( (pB(1)-pA(1)).^2 + (pB(2)-pA(2)).^2 + (pB(3)-pA(3)).^2 ))

% ---- find pointC (center of ovoid computation)
ind325= find( abs(dist_AB-27)==min(abs(dist_AB-27)) );
pC(1)= candB_x(ind325); pC(2)= candB_y(ind325); pC(3)= candB_z(ind325);
sqrt(sum( (pC(1)-pA(1)).^2 + (pC(2)-pA(2)).^2 + (pC(3)-pA(3)).^2 ))

% ---- find pointO (Ovoid poistion)
ind75= find( abs(dist_AB-69.5)==min(abs(dist_AB-69.5)) );
pO(1)= candB_x(ind75); pO(2)= candB_y(ind75); pO(3)= candB_z(ind75);
sqrt(sum( (pO(1)-pO(1)).^2 + (pO(2)-pA(2)).^2 + (pO(3)-pA(3)).^2 ))

%% increment source point
resolution= 2.5;  % mm
angle_resolution=AngleResolution;
angle_pos= -180:angle_resolution:170;

end_tandem= floor(65/resolution)+1;
source_pos(1,:)= pA;
source_pos(end_tandem,:)= pB;

for ii=1:end_tandem-2
    temp_dist= resolution*ii
    ind_source= find( abs(dist_AB-temp_dist)==min(abs(dist_AB-temp_dist)) );
    source_pos(ii+1,:)= [ candB_x(ind_source)  candB_y(ind_source)  candB_z(ind_source) ];
end

% Above Code should be modified: 2024-09-29
source_pos = tandem(1:27,:);

EllapsedTime_prepare = toc

% Check Data
%source_pos
%angle_pos

%% Rotation of Tandem relative to the image coordinate system
% ------------------------------- Tandem
display(['Processing Rotation of Tandem relative to the image coordinate system'])
tic
PTV=[]; tuning=[]; bladder=[]; rectum=[]; sigmoid=[]; bowel=[]; body=[];

load systemA_2nd   % Tandem MC simulated results
original_A = A;
original_A= reshape(original_A,201,201,201);
ind_neg= find(original_A<0);
original_A(ind_neg)=0;
sum_tandem= sum(original_A(:))  % Activity of Tandem source = Activity of Ovoid


cnt=1;
for jj=1:length(angle_pos)
    for ii=1:size(source_pos,1)
        
        cnt

        if ii==1
            theta_x2= (atan((source_pos(ii,2)-source_pos(ii+1,2))/(source_pos(ii,3)-source_pos(ii+1,3)+0.000001))*180/pi);
            theta_y2= (-atan((source_pos(ii,1)-source_pos(ii+1,1))/(source_pos(ii,3)-source_pos(ii+1,3)+0.000001))*180/pi);
            theta_z2= angle_pos(jj);
        else
            theta_x2= (atan((source_pos(ii-1,2)-source_pos(ii,2))/(source_pos(ii-1,3)-source_pos(ii,3)+0.000001))*180/pi);
            theta_y2= (-atan((source_pos(ii-1,1)-source_pos(ii,1))/(source_pos(ii-1,3)-source_pos(ii,3)+0.000001))*180/pi);
            theta_z2= angle_pos(jj);
        end

        xt= source_pos(ii,1)-100:1:source_pos(ii,1)+100;
        yt= source_pos(ii,2)-100:1:source_pos(ii,2)+100;
        zt= source_pos(ii,3)+100:-1:source_pos(ii,3)-100;
        
        %load systemA_2nd   % Tandem MC simulated results
        %A= reshape(A,201,201,201);
        %ind_neg= find(A<0);
        %A(ind_neg)=0;
        %sum_tandem= sum(A(:));  % Activity of Tandem source = Activity of Ovoid
        %vA= size(A);

        A = original_A;
        vA= size(A);

        Arot=A;
       
        %Arot= rotate3D(A,theta_x2,theta_y2,theta_z2,vA);
        %rotate3D를 아래로 변경
        theta = [theta_x2 theta_y2 theta_z2]
        transl = [0 0 0];
        tform = rigidtform3d(theta,transl);
        centerOutput = affineOutputView(size(Arot),tform,"BoundsStyle","CenterOutput");
        Arot = imwarp(Arot, tform, "OutputView",centerOutput);
        
        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot_new= interp3(Xt,Yt,Zt,Arot,Xc,Yc,Zc);
        Arot_newd= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot_newd_4= zeros(vC(1)/4,vC(2)/4,vC(3));

        for ss=1:vC(3)
            Arot_newd(:,:,ss)= Arot_new(1:2:end,1:2:end,ss);  % for body dose calculation
            Arot_newd_4(:,:,ss)= Arot_new(1:4:end,1:4:end,ss);  % for body dose calculation
        end
       

        PTV(:,cnt)= Arot_newd(ind_PTVd);
        bladder(:,cnt)= Arot_newd(ind_bladderd);
        rectum(:,cnt)= Arot_newd(ind_rectumd);
        bowel(:,cnt)= Arot_newd(ind_boweld);
        sigmoid(:,cnt)= Arot_newd(ind_sigmoidd);

        if flag_body==1
            body(:,cnt)= Arot_newd_4(ind_bodyd);
        end

        cnt= cnt+1;
    end
end

%% ---------------------------- Ovoid
for ii=1:5;    % 5 --> # of possible ovoids for each side
    
    cnt
    xt= pC(1)-99.5:1:pC(1)+99.5;
    yt= pC(2)-99.5:1:pC(2)+99.5;
    zt= pC(3)-99.5:1:pC(3)+99.5;  
    
    theta_x2= (atan((pB(2)-pA(2))/(pB(3)-pA(3)))*180/pi);
    theta_y2= (-atan((pB(1)-pA(1))/(pB(3)-pA(3)))*180/pi);
    
    %%%%%%%%%%%%%%%%%%%%%%%%% ovoid1
    if ii==1;
        load systemA_ovoid1_left
        Al_o1= reshape(A,200,200,200)*sum_tandem/sum(A(:));
        Al_o1_rot= Al_o1;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot1= interp3(Xt,Yt,Zt,Al_o1_rot,Xc,Yc,Zc);
        Arot1_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot1_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot1_ds(:,:,ss)= Arot1(1:2:end,1:2:end,ss);
            Arot1_ds_4(:,:,ss)= Arot1(1:4:end,1:4:end,ss);
        end
    
        load systemA_ovoid1_right
        Ar_o1= reshape(A2,200,200,200)*sum_tandem/sum(A2(:));
        Ar_o1_rot= Ar_o1;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot2= interp3(Xt,Yt,Zt,Ar_o1_rot,Xc,Yc,Zc);
        Arot2_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot2_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot2_ds(:,:,ss)= Arot2(1:2:end,1:2:end,ss);
            Arot2_ds_4(:,:,ss)= Arot2(1:4:end,1:4:end,ss);
        end
        
    %%%%%%%%%%%%%%%%%%%%%%%%% ovoid2
    elseif ii==2
        load systemA_ovoid2_left
        Al_o2= reshape(B,200,200,200)*sum_tandem/sum(B(:));
        Al_o2_rot= Al_o2;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot1= interp3(Xt,Yt,Zt,Al_o2_rot,Xc,Yc,Zc);
        Arot1_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot1_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot1_ds(:,:,ss)= Arot1(1:2:end,1:2:end,ss);
            Arot1_ds_4(:,:,ss)= Arot1(1:4:end,1:4:end,ss);
        end
        
        load systemA_ovoid2_right
        Ar_o2= reshape(B2,200,200,200)*sum_tandem/sum(B2(:));
        %Ar_o1= permute(Ar_o1,[2 1 3]);
        Ar_o2_rot= Ar_o2;
        %Ar_o1_rot= rotate3D(Ar_o1,theta_x2,theta_y2,0,[200 200 200]);

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot2= interp3(Xt,Yt,Zt,Ar_o2_rot,Xc,Yc,Zc);
        Arot2_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot2_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot2_ds(:,:,ss)= Arot2(1:2:end,1:2:end,ss);
            Arot2_ds_4(:,:,ss)= Arot2(1:4:end,1:4:end,ss);
        end
        %%%%%%%%%%%%%%%%%%%%%%%%% ovoid3
     elseif ii==3
        load systemA_ovoid3_left
        Al_o3= reshape(C,200,200,200)*sum_tandem/sum(C(:));
        Al_o3_rot= Al_o3;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot1= interp3(Xt,Yt,Zt,Al_o3_rot,Xc,Yc,Zc);
        Arot1_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot1_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot1_ds(:,:,ss)= Arot1(1:2:end,1:2:end,ss);
            Arot1_ds_4(:,:,ss)= Arot1(1:4:end,1:4:end,ss);
        end

        load systemA_ovoid3_right
        Ar_o3= reshape(C2,200,200,200)*sum_tandem/sum(C2(:));
        Ar_o3_rot= Ar_o3;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot2= interp3(Xt,Yt,Zt,Ar_o3_rot,Xc,Yc,Zc);
        Arot2_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot2_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot2_ds(:,:,ss)= Arot2(1:2:end,1:2:end,ss);
            Arot2_ds_4(:,:,ss)= Arot2(1:4:end,1:4:end,ss);
        end
        
        %%%%%%%%%%%%%%%%%%%%%%%%% ovoid4
    elseif ii==4
        load systemA_ovoid4_left
        Al_o4= reshape(D,200,200,200)*sum_tandem/sum(D(:));
        Al_o4_rot= Al_o4;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot1= interp3(Xt,Yt,Zt,Al_o4_rot,Xc,Yc,Zc);
        Arot1_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot1_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot1_ds(:,:,ss)= Arot1(1:2:end,1:2:end,ss);
            Arot1_ds_4(:,:,ss)= Arot1(1:4:end,1:4:end,ss);
        end
        
        load systemA_ovoid4_right
        Ar_o4= reshape(D2,200,200,200)*sum_tandem/sum(D2(:));
        Ar_o4_rot= Ar_o4;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot2= interp3(Xt,Yt,Zt,Ar_o4_rot,Xc,Yc,Zc);
        Arot2_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot2_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot2_ds(:,:,ss)= Arot2(1:2:end,1:2:end,ss);
            Arot2_ds_4(:,:,ss)= Arot2(1:4:end,1:4:end,ss);
        end

        %%%%%%%%%%%%%%%%%%%%%%%%% ovoid5
    elseif ii==5
        load systemA_ovoid5_left
        Al_o5= reshape(E,200,200,200)*sum_tandem/sum(E(:));
        Al_o5_rot= Al_o5;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot1= interp3(Xt,Yt,Zt,Al_o5_rot,Xc,Yc,Zc);
        Arot1_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot1_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot1_ds(:,:,ss)= Arot1(1:2:end,1:2:end,ss);
            Arot1_ds_4(:,:,ss)= Arot1(1:4:end,1:4:end,ss);
        end
        
        load systemA_ovoid5_right
        Ar_o5= reshape(E2,200,200,200)*sum_tandem/sum(E2(:));
        Ar_o5_rot= Ar_o5;

        [Xt Yt Zt]= meshgrid(xt,yt,zt);
        Arot2= interp3(Xt,Yt,Zt,Ar_o5_rot,Xc,Yc,Zc);
        Arot2_ds= zeros(vC(1)/2,vC(2)/2,vC(3));
        Arot2_ds_4= zeros(vC(1)/4,vC(2)/4,vC(3));
        for ss=1:vC(3);
            Arot2_ds(:,:,ss)= Arot2(1:2:end,1:2:end,ss);
            Arot2_ds_4(:,:,ss)= Arot2(1:4:end,1:4:end,ss);
        end
    end
   
   
    PTV(:,cnt)= Arot1_ds(ind_PTVd);
    bladder(:,cnt)= Arot1_ds(ind_bladderd);
    rectum(:,cnt)= Arot1_ds(ind_rectumd);
    bowel(:,cnt)= Arot1_ds(ind_boweld);
    sigmoid(:,cnt)= Arot1_ds(ind_sigmoidd);
    if flag_body==1
        body(:,cnt)= Arot1_ds_4(ind_bodyd);
    end
    cnt= cnt+1;
    
    PTV(:,cnt)= Arot2_ds(ind_PTVd);
    bladder(:,cnt)= Arot2_ds(ind_bladderd);
    rectum(:,cnt)= Arot2_ds(ind_rectumd);
    bowel(:,cnt)= Arot2_ds(ind_boweld);
    sigmoid(:,cnt)= Arot2_ds(ind_sigmoidd);

    if flag_body==1
        body(:,cnt)= Arot2_ds_4(ind_bodyd);
    end
    cnt= cnt+1;
    
end

time_kernel_matrices= toc

% save Brachy_Patient4_Dec2020_tandem_ovoid_BODY  body ind_body ind_bodyd  mask_body mask_PTV ind_PTV  mask_bladder ind_bladder mask_rectum ind_rectum mask_bowel ind_bowel mask_sigmoid ind_sigmoid -v7.3
save Brachy_Patient2_Dec2020_tandem_ovoid_BODY_test body PTV bladder rectum bowel sigmoid mask_body mask_PTV mask_bladder mask_rectum mask_sigmoid ind_bodyd -v7.3

%save Brachy_Patient2_Dec2024M07_tandem_ovoid_BODY_new ind_bodyd body PTV bladder rectum bowel sigmoid -v7.3

fprintf("######## Dose Kernel Creation Completed");
end
