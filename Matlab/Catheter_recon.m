% clear all;
% close all;
% addpath C:\Users\kimhj\Desktop\Brachy_opt
% cd C:\Users\kimhj\Desktop\Brachy_opt
% 
% % cd Patient1
% % info = dicominfo('RP.1.3.6.1.4.1.2452.6.355886972.1250305556.2745081264.4248497238.dcm');
% cd Patient2
% info = dicominfo('RP.1.3.6.1.4.1.2452.6.3012614243.1167505823.3707139774.1781071597.dcm');
% % info_dose = dicominfo('dose_case_1.dcm');
% % info_struct = dicominfo('struct_case_1.dcm');
% y = dicomread(info);

% ch_1: Ovoid R, ch_2: Ovoid L, ch_3: Tandem
function [ch_1 ch_2 ch_3 ]= Catheter_recon(info,y)
% Source Position


addpath D:\___01_Works\IMBT\Development\2ProfLim\2ProfLim\Brachy_opt\patient4

channelSequence = fieldnames(info.ApplicationSetupSequence.Item_1.ChannelSequence);
numOfField = length(channelSequence)

for i=1:1:3 %numOfField
    
    disp('source');
    fn_points_sequence = fieldnames(info.ApplicationSetupSequence.Item_1.ChannelSequence.(channelSequence{i}).BrachyControlPointSequence);
    numOfPoints = length(fn_points_sequence);
    
    for s = 1:1:numOfPoints
        p = info.ApplicationSetupSequence.Item_1.ChannelSequence.(channelSequence{i}).BrachyControlPointSequence.(fn_points_sequence{s}).ControlPoint3DPosition;
        
       disp({p(1) p(2) p(3)});
       x(i,s) = p(1);
       y(i,s) = p(2);
       z(i,s) = p(3);
        
    end
    
    disp('');
    
end

scatter3(x(1,:),y(1,:),z(1,:));
hold on;
scatter3(x(2,:),y(2,:),z(2,:));
hold on;

if size(x,1) == 3
scatter3(x(3,:),y(3,:),z(3,:));
end

%% Catheter Reconstruction
fn_ROIContourSequence = fieldnames(info.Private_300f_1000.Item_1.ROIContourSequence);
numOfROISequence = length(fn_ROIContourSequence);

nums = 1;
ss=[];
for i=1:1:numOfROISequence
    
    disp('applicator');
    if strcmp(info.Private_300f_1000.Item_1.ROIContourSequence.(fn_ROIContourSequence{i}).ContourSequence.Item_1.ContourGeometricType,'OPEN_NONPLANAR')
        points = vec2mat(info.Private_300f_1000.Item_1.ROIContourSequence.(fn_ROIContourSequence{i}).ContourSequence.Item_1.ContourData,3);
        %points = reshape(info.Private_300f_1000.Item_1.ROIContourSequence.(fn_ROIContourSequence{i}).ContourSequence.Item_1.ContourData,3,[]);
        
        if isfield(ss,'a') == 0
            ss.a = points;
        elseif isfield(ss, 'b') == 0
            ss.b = points;
        elseif isfield(ss, 'c') == 0
            ss.c = points;
        end
        
        nums = nums + 1;
    end
end

ch_1 = ss.a;
% if isfield(ss, 'c') == 0
if ~isempty(ss.c)
    ch_2 = ss.b;
    ch_3 = ss.c;
else
    ch_2 = [];
    ch_3 = ss.c;
end
% else
%     ss_temp=[];
% end
if ~isempty(ss.c)
    plot3(ss.a(:,1),ss.a(:,2),ss.a(:,3),'--'); % Ring or Ovoid R
    hold on;
    plot3(ss.b(:,1),ss.b(:,2),ss.b(:,3),':'); % Tandem
    hold on;
    plot3(ss.c(:,1),ss.c(:,2),ss.c(:,3)); % Ovoid L
    hold off;
else
    plot3(ss.a(:,1),ss.a(:,2),ss.a(:,3));
    hold on;
    plot3(ss.b(:,1),ss.b(:,2),ss.b(:,3));
    hold off;
end
    
end

