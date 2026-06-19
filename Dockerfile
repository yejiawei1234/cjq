
FROM --platform=linux/amd64 ubuntu:24.04

RUN apt update && apt install -y \
    build-essential \
    cmake \
    ninja-build \
    gdb

WORKDIR /src