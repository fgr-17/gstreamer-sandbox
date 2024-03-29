FROM python:3.9-slim

COPY ./src/packages packages
COPY ./src/installPackages.sh installPackages.sh

RUN apt-get update
RUN chmod +x installPackages.sh
RUN ./installPackages.sh

RUN printf "\nalias ls='ls --color=auto'\n" >> ~/.bashrc
RUN printf "\nalias ll='ls -alF'\n" >> ~/.bashrc


RUN python -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

COPY Socket-Client/requirements.txt .

RUN python -m pip install --upgrade pip && \
    pip install -r requirements.txt

ENV PACKAGE_PATH="/workspace/src/package"