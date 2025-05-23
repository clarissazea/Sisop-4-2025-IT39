FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    gcc \
    fuse \
    libfuse-dev \
    pkg-config \
    make \
    sudo \
    libssl-dev \
 && rm -rf /var/lib/apt/lists/*

COPY antink.c /antink.c

RUN gcc -Wall $(pkg-config fuse --cflags) /antink.c -o /antink $(pkg-config fuse --libs)

RUN mkdir /original /antink_mount

CMD ["/antink", "/antink_mount"]
