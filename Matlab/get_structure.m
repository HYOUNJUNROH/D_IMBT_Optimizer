function [x,y,z,strucname] = get_structure(A)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [x,y,z,strucname] = get_structure(A)
% 
% this takes gets the x, y, and points for all the contours and also gets
% the structure names
% 
% example
% A = dicominfo('RT_Structure_Set_Storage-.dcm');
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
display('Loading structure information ...')
dicomstruct = dicominfo(A);
display('The following structures are found in this dicom file.')
flag = 0;
count = 0;
%lists all the structures
while flag == 0
    count = count + 1;
    try 
        eval(['roiname = dicomstruct.StructureSetROISequence.Item_' ...
            num2str(count) '.ROIName;']);
        display([num2str(count) '. ' roiname])
        strucname{count} = roiname;
    catch
        flag = 1;
        count = count-1;
    end
end


for c = 1:length(strucname)
    eval(['x' num2str(c) ' = [];'])
    eval(['y' num2str(c) ' = [];'])
    eval(['z' num2str(c) ' = [];'])

    try
        for a = 1:10000000
            eval(['B = dicomstruct.ROIContourSequence.Item_' num2str(c) '.ContourSequence.Item_' num2str(a) ...
                '.ContourData;']);
            xi = B(1:3:end);
            yi = B(2:3:end);
            zi = B(3:3:end);
            eval(['x' num2str(c) '{a} = xi;'])
            eval(['y' num2str(c) '{a} = yi;'])
            eval(['z' num2str(c) '{a} = zi;'])
            
            
        end
    end
    eval(['x{c} = x' num2str(c) ';'])
    eval(['y{c} = y' num2str(c) ';'])
    eval(['z{c} = z' num2str(c) ';'])

end


end

 