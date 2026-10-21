# syntax=docker/dockerfile:1
FROM ubuntu:24.04 AS clap-plugins-build
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y \
      -o Acquire::BrokenProxy="true" \
      -o Acquire::http::No-Cache="true" \
      -o Acquire::http::Pipeline-Depth="0" \
      git \
      cmake \
      ninja-build \
      pkg-config \
      g++ \
      libwayland-dev \
      wayland-protocols \
      libxcb1-dev \
    && rm -rf /var/lib/apt/lists/*
