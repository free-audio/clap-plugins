# syntax=docker/dockerfile:1
# FROM debian:stable-slim AS clap-plugins-build
FROM ubuntu:22.04 AS clap-plugins-build
ARG DEBIAN_FRONTEND=noninteractive
ARG CACHE_DATE=2024-09-04

RUN apt update
RUN apt upgrade -y
RUN apt install -y gpg wget lsb-release software-properties-common locales

# Set en_US.UTF-8 as the locale
RUN locale-gen "en_US.UTF-8"
ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

RUN apt update \
    && apt install -y \
      -o Acquire::BrokenProxy="true" \
      -o Acquire::http::No-Cache="true" \
      -o Acquire::http::Pipeline-Depth="0" \
      curl zip unzip tar pkg-config \
      make autoconf autoconf-archive automake libtool \
      nasm virtualenv \
      coreutils \
      cmake \
      chrpath \
      git \
      ninja-build \
      sudo \
      fakeroot \
      util-linux \
      gcc \
      g++

RUN apt install -y \
    -o Acquire::BrokenProxy="true" \
    -o Acquire::http::No-Cache="true" \
    -o Acquire::http::Pipeline-Depth="0" \
    libc6-dev \
    libvulkan-dev \
    libx11-dev \
    libx11-xcb-dev \
    '^libxcb.*-dev' \
    libglu1-mesa-dev \
    libxrender-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libegl1-mesa-dev \
    libxi-dev \
    libxinerama-dev \
    libxmu-dev \
    mesa-common-dev \
    xorg-dev