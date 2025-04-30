function DisplayCatheter(ch1,ch2,ch3)
%DISPLAYCATHETER Summary of this function goes here
%   Detailed explanation goes here
ss.a = ch1;
ss.b = ch2;
ss.c = ch3;

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

