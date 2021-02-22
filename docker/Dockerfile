# This dockerfile uses Environment for ESP32 CI builds
#
# Ubuntu 18.04
# https://hub.docker.com/_/ubuntu
#
# ESP-IDF v4.1
# https://docs.espressif.com/projects/esp-idf/en/v4.1/get-started/index.html#get-esp-idf
#
# Referenced from : https://github.com/larryli/esp-idf-ci-env

# Author: Javier James
# Company: Kien 


FROM ubuntu:18.04

ENV IDF_PATH="/opt/local/espressif/esp-idf" \
    PATH="/opt/local/espressif/xtensa-esp32-elf/bin:${PATH}" \
    IDF_TOOLS_PATH = IDF_PATH
RUN apt-get update \
    && apt-get install -y gcc git wget make libncurses-dev flex bison gperf python python-pip python-setuptools python-serial python-cryptography python-future \
	cmake ninja-build ccache libffi-dev libssl-dev && rm -r /var/lib/apt/lists/*
RUN mkdir -p /opt/local/espressif/  
RUN git clone -b v4.1 --depth 1 --recursive https://github.com/espressif/esp-idf.git $IDF_PATH \
    && cd $IDF_PATH && git submodule foreach --recursive 'git gc --aggressive --prune=all'
RUN ./install.sh
RUN . $IDF_PATH/export.sh
RUN python -m pip install --user -r $IDF_PATH/requirements.txt

