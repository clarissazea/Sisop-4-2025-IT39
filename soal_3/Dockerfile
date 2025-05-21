FROM ubuntu:latest

COPY antink_fuse.c /antink.c

RUN sudo apt update && sudo apt upgrade && sudo apt install libfuse* && sudo apt install pkg-config
RUN gcc -Wall `pkg-config fuse --cflags` antink.c -o antink `pkg-config fuse --libs`

RUN mkdir /antink_mount

CMD ["/antink", "/antink_mount"]
