function [n1 n2 n3 n4]=compute_weight(struct1,struct2,struct3,struct4,x);

struct= struct1;
n1=norm(struct*x)/(size(struct,1));

struct= struct2;
n2=norm(struct*x)/(size(struct,1));

struct= struct3;
n3=norm(struct*x)/(size(struct,1));

struct= struct4;
n4=norm(struct*x)/(size(struct,1));
