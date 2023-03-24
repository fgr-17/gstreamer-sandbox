FROM gcc:latest

COPY packages packages
COPY installPackages.sh installPackages.sh

RUN apt-get update
RUN chmod +x installPackages.sh
RUN ./installPackages.sh

RUN printf "\nalias ls='ls --color=auto'\n" >> ~/.bashrc
RUN printf "\nalias ll='ls -alF'\n" >> ~/.bashrc