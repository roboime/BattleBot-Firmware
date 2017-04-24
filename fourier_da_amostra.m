%%
close all;
clear all;
clc;

% recebe o sinal da porta serial
s = serial(''); %coloque aqui o numero da porta serial

set(s,'InputBufferSize',16); % numero de bytes do buffer
set(s,'FlowControl','hardware');
set(s,'BaudRate',9600);
set(s,'Parity','none');

set(s,'DataBits', 8);
set(s,'StopBit', 1);
Ts = %periodo amostral colocar o periodo de um loop do programa em segundos
set(s,'Timeout', Ts);%tempo em segundos que o matlab aguarda novos dados


%mostrar que as configurações foram feitas
disp(get(s,'Name')); % mostra o nome "Serial - COM"
prop(get(s,'BaudRate'));
prop(get(s,'Databits'));
prop(get(s,'StopBit'));
prop(get(s,'InputBufferSize'));
disp(['Port Setup Feito!!', num2str(prop)]);

fopen(s); % apre a porta serial
t=1;
while(T(t) < 5)
    %plota os dados recebidos do serial
    a = fgetl(s);
    
    T(t) = t*Ts;%mutiplicando pelo periodo de amostragem
    X(t) = num2str(a); %a é uma string , convertendo a strin em um numero
    
    t=t+1;
    a=0; %limpando o buffer
    
end;

figure();
plot(T,X);

%fazendo a transformada de furier
N = length(X);
k=0:N-1;
T=N*Ts;
freg=k/T;
Y=fftn(X)/N;
fc=ceil(N/2);
Y=Y(1:fc);


plot(freg(1:fc),abs(Y));








