# RoboIME Sanhaço
Esse é o repositório oficial onde fica o código do firmware e o projeto do hardware utilizado pela equipe de batalha de robôs da RoboIME. Contribuições são aceitas. O projeto está sendo acompanhado em: http://redmine.roboime.com.br/projects/batalha-de-robos

# Compilação
Para compilar o código, foi utilizado `avr-gcc 7.1.0`, `avr-binutils 2.28` e `avr-libc 2.0.0`. Depois de instaladas essas versões, basta rodar o `make` para compilar tudo. Para programar a placa, use ` make upload PORT=<porta>`. `make clean` limpa os arquivos de objeto e `make dump` exporta um excerto do código em Assembly para o arquivo `avrdisasm.txt`.
