FROM gcc:latest

COPY ./packages packages

RUN apt update 
RUN xargs -a packages apt install -y

RUN printf "\nalias ls='ls --color=auto'\n" >> ~/.bashrc
RUN printf "\nalias ll='ls -alF'\n" >> ~/.bashrc