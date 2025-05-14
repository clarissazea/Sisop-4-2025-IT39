FROM ubuntu:latest

RUN mkdir soal3
RUN sudo apt update && sudo apt upgrade && sudo apt install libfuse* && sudo apt install pkg-config
RUN gcc -Wall `pkg-config fuse --cflags` antink.c -o antink `pkg-config fuse --libs`

CMD ./antink soal3
