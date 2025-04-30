function [ x wb ]= fluence_map_opt_GroupSparsity_Tandem(A_PTV,d_PTV,A_OAR,d_OAR, opts); 

A= A_PTV;
d= d_PTV;

AtA= A'*A;
Atd= A'*d;
A2tA2= A_OAR'*A_OAR;

seg= size(A_PTV,2)/36;
trunc= opts.trunc;

%% Fluence-map Optimization with only PTV
% -- With C.P. L1-norm
x= zeros(size(A,2),1);
y= zeros(size(x));
z= zeros(size(x));

lambda= opts.lambda;
c= opts.c; t= opts.t;
acc= opts.acc;
inv_L1= opts.inv_L1
L0=opts.L0 
L1=opts.L1; 
Lhalf=opts.Lhalf
WW= opts.WW;

if ~isempty(WW);
   lambda= opts.lambda*WW;
else
   lambda= opts.lambda*ones(size(x));
end
mat = @(X) reshape(X,n1,n2,field);
vec = @(X) X(:);

for b=1:36
    A_b= A(:,(b-1)*seg+1:b*seg);
    wb(b)= c*(normest(A_b))^0.25; %0.25;
end

% figure(); plot(wb)
wb= c*wb./max(wb);

disp('Beginning iteration')

for ii=1:opts.MaxIter
    
    if mod(ii,1000)==0; 
        ii 
    end
     
%     %% Update x    
%     tempx= x(:)-t*(A'*x(:) + A_OAR'*x(:) );
%     x= tempx; %max(tempx,0);
        
    %% Define y and z;
    if acc==1;
        if ii>1;
            ks= ii/(ii+3);
            y= x + ks*(x-x_old);
            z= y(:)-t*( AtA*y(:)-Atd + A2tA2*y(:) );
        else
            z= x(:)-t*( AtA*x(:)-Atd + A2tA2*x(:) );
    %         x= max(x,0);
        end
    else
       z= x(:)-t*( AtA*x(:)-Atd + A2tA2*x(:) );
    end
    
    x_old=x;
    
    %% Define u
    for b=1:36;
        z= z(:);
        zb= z(seg*(b-1)+1:seg*b,1);
        lambda_b= lambda(seg*(b-1)+1:seg*b,1);
        if inv_L1==1;
            u= max(zb-lambda_b*t,0);
        else
            u= max(zb,0);
        end
        
        if L0==1;
            tempx= proxh0_group(u,t,wb(b));
        elseif L1==1;
            tempx= proxh1_group(u,t,wb(b));
        elseif Lhalf==1;
            tempx= proxhalf_group(u,t,wb(b));
        else
            disp('wrong selection');
        end
        x(seg*(b-1)+1:seg*b,1)= tempx;
    end    
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    x2= convert_order_x(x);
    x2(trunc:end,1)=0;
    x= convert_reverse_order_x(x2);
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    hist_norm(ii)= norm(x-x_old)/norm(x);
    if hist_norm<1e-5; 
        ii 
        break; 
    end;
    
%     save tempx x hist_norm ii;
%     if ii==3000; save tempx1 x; end;
%     if ii==5000; save tempx2 x; end;   
%     if ii==10000; save tempx3 x; end;   
end

function x= proxhalf_group(u,t,w);
    x= zeros(size(u));
    
    temp= (t*w)./(norm(u)^1.5);
    th= 2*sqrt(6)/9;
    if temp<=th
        val= (2/sqrt(3))*sin( (1/3)*(acos(3*sqrt(3)*temp/4)+0.5*pi) );
        x= u*sqrt(val);
    end
    
end

function x= proxh1_group(u,t,w);
    x= zeros(size(u));
   
    temp_val= (t*w)./abs(u);
    temp_u= u- u.*min(temp_val,1);
    x= temp_u;
    
end

function x= proxh0_group(u,t,w);
    x= zeros(size(u));
   
    indx= find(u > sqrt(2*t*w));
    if norm(u)>sqrt(2*t*w)
        x=u;
    end
    
end
    
end

