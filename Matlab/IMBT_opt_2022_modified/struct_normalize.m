function [ struct_new d_new ]= struct_normalize(struct,d);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%% struct==> A, where Ax=d 
%%%%% (A/normA)*x= d/normA
%%%%%% A'x= d', where A'= A/normA, d'= d/normA
%%%%%%   -- normA is not a norm operator for entire matrix A, but
%%%%%%   -- it is a row-by-row operator
%%%%%%                           Feb 2018. Hojin Kim
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

for ii=1:size(struct,1);
    tmp= norm(struct(ii,:));
    if tmp~=0;
        hist_norm(ii)= tmp;
    else
        hist_norm(ii)= 1;
    end
    struct(ii,:)= struct(ii,:)/hist_norm(ii);
    d(ii,:)= d(ii,:)/hist_norm(ii);
end
struct_new= struct;
d_new= d;