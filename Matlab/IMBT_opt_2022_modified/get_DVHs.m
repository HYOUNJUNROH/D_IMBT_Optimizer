function [factor DVH_t,DVH_OAR1,DVH_OAR2,DVH_OAR3,DVH_OAR4 ]=...
    get_DVHs(prescriptionDose, Normalization, PTV,OAR1,OAR2,OAR3,OAR4,x_beam,x,sig);

if nargin < 9
    sig=0;
end
x_perc=x;

PTV_dummy = PTV*x_beam(:);
OAR1_dummy = OAR1*x_beam(:); % bladder
OAR2_dummy = OAR2*x_beam(:); % rectum
OAR3_dummy = OAR3*x_beam(:); % bowel
OAR4_dummy = OAR4*x_beam(:); % sigmoid
% OAR5_dummy = OAR5*x_beam(:);
% OAR6_dummy = OAR6*x_beam(:);
%Sem_dummy= Sem*x_beam(:);
% OAR1_dummy = OAR5*x_beam(:);

size(PTV_dummy)

Dummy_t = hist(PTV_dummy,x);
DVH_t = fliplr(cumsum(fliplr(Dummy_t)));
DVH_t = DVH_t/max(DVH_t);

val=Normalization/100.0; % Normalization 90% of Target
ind_norm= find(abs(DVH_t-val)== min(abs(DVH_t-val)));
x(ind_norm)
factor= prescriptionDose/x(ind_norm) % Prescription Dose: 50Gy

Dummy_OAR1 = hist(OAR1_dummy,x);
DVH_OAR1 = fliplr(cumsum(fliplr(Dummy_OAR1)));
DVH_OAR1 = DVH_OAR1/max(DVH_OAR1);

Dummy_OAR2 = hist(OAR2_dummy,x);
DVH_OAR2 = fliplr(cumsum(fliplr(Dummy_OAR2)));
DVH_OAR2 = DVH_OAR2/max(DVH_OAR2);

Dummy_OAR3 = hist(OAR3_dummy,x);
DVH_OAR3 = fliplr(cumsum(fliplr(Dummy_OAR3)));
DVH_OAR3 = DVH_OAR3/max(DVH_OAR3);

Dummy_OAR4 = hist(OAR4_dummy,x);
DVH_OAR4 = fliplr(cumsum(fliplr(Dummy_OAR4)));
DVH_OAR4 = DVH_OAR4/max(DVH_OAR4);

% Dummy_OAR5 = hist(OAR5_dummy,x);
% DVH_OAR5 = fliplr(cumsum(fliplr(Dummy_OAR5)));
% DVH_OAR5 = DVH_OAR5/max(DVH_OAR5);
% 
% Dummy_OAR6 = hist(OAR6_dummy,x);
% DVH_OAR6 = fliplr(cumsum(fliplr(Dummy_OAR6)));
% DVH_OAR6 = DVH_OAR6/max(DVH_OAR6);

% 

if sig==1
    figure;
    plot(x_perc*factor,DVH_t); hold on;
    plot(x_perc*factor,DVH_OAR1,'r');
    plot(x_perc*factor,DVH_OAR2,'k');
    plot(x_perc*factor,DVH_OAR3,'g');
    plot(x_perc*factor,DVH_OAR4,'m');

    xlim([0 100])
    grid on; hold off;
    legend('PTV','OAR1','OAR2','OAR3','OAR4') %'OAR5'); %,'OAR6'); %,'Tuning');
end
% -------------------------------
