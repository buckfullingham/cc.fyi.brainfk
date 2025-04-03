FROM ubuntu

RUN apt -y update \
 && apt -y upgrade \
 && apt -y install \
    cmake \
    python3-full \
    gdb \
    g++ \
    gcovr \
    lcov \
    libzstd-dev \
    zlib1g-dev \
    valgrind \
    lsb-release \
    wget \
    software-properties-common \
    gnupg

RUN bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"

RUN python3 -m venv /venv
ENV PATH=$PATH:/venv/bin
COPY ../requirements.txt /tmp/
RUN pip3 install -U -r /tmp/requirements.txt && rm /tmp/requirements.txt

ENV BUILD_PROFILE=linux
RUN apt -y install git
