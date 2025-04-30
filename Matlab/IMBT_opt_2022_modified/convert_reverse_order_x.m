% Before
%  pos 1 / angle1 angle2 ... angle36
%  pos 2 / angle1 angle2 ... angle36
% After
%  angle 1 / pos1 pos2 pos3 ... pos27
%  angle 2 / pos1 pos2 pos3 ... pos27

function x_new= convert_reverse_order_x(x, numOfAngles);
    x_new= zeros(size(x));
    seg= length(x)/numOfAngles;
    for ii=1:numOfAngles
        num1= (ii-1)*seg+1;
        num2= (ii-1)*seg+seg;
        x_new(num1:num2,:)= x(ii:numOfAngles:end,:);
    end

end