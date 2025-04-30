% Before
%  angle 1 / pos1 pos2 pos3 ... pos27
%  angle 2 / pos1 pos2 pos3 ... pos27
% After
%  pos 1 / angle1 angle2 ... angle36
%  pos 2 / angle1 angle2 ... angle36

function x_new= convert_order_x(x, numOfAngles);
    x_new= zeros(size(x));
    seg= length(x)/numOfAngles;
    for ii=1:seg
        num1= (ii-1)*numOfAngles+1;
        num2= (ii-1)*numOfAngles+numOfAngles;
        x_new(num1:num2,:)= x(ii:seg:end,:);
    end

end