# Dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    fuse \
    libfuse-dev \
    pkg-config \
    gcc \
    make \
    vim

WORKDIR /app
COPY antink.c /app/
RUN gcc -Wall antink.c -o antink `pkg-config fuse --cflags --libs`

RUN mkdir -p /antink__mount /it24__host

CMD ["/app/antink", "/antink__mount"]
