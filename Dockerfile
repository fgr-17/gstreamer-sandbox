FROM python:3.9-slim

COPY ./src/packages packages

RUN apt update 
RUN xargs -a packages apt install -y

RUN printf "\nalias ls='ls --color=auto'\n" >> ~/.bashrc
RUN printf "\nalias ll='ls -alF'\n" >> ~/.bashrc


RUN python -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

COPY Socket-Client/requirements.txt .

RUN python -m pip install --upgrade pip && \
    pip install -r requirements.txt

ENV PACKAGE_PATH="/workspace/src/package"