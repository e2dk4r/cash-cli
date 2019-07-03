# First phase: building the app
FROM alpine AS builder
ENV TZ UTC
ENV LC_ALL C

## build deps
RUN apk add --no-cache --virtual build \
   binutils \
   gcc      \
   libc-dev \
   make

## dev deps
RUN apk add --no-cache --virtual deps \
   curl-dev \
   libcurl \
   jansson-dev \
   jansson

COPY src /app/src
COPY Makefile /app

WORKDIR /app
RUN make

# Second phase: running the app
FROM alpine

## runtime deps
RUN apk add --no-cache \
   libcurl \
   jansson

COPY --from=builder /app/cash /bin/cash

# vim:ts=3:sw=3:et:ft=Dockerfile
