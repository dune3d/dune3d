# Choose your Debian/Ubuntu version, default is Trixie
FROM debian:unstable-slim

ARG TARGET=all
ARG LANG=C.UTF-8
ARG BUILD_PY=0

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=${LANG}
ENV LC_ALL=${LANG}

# Base system + locales
RUN apt-get update && apt-get install -y \
    locales \
    ca-certificates \
    git \
    build-essential \
    cmake \
    pkg-config \
    meson \
    ninja-build \
    python3 \
    python3-gi \
    python3-gi-cairo \
    gir1.2-rsvg-2.0 \
    libglm-dev \
    libocct-ocaf-dev \
    libocct-data-exchange-dev \
    libgtkmm-4.0-dev \
    uuid-dev \
    libeigen3-dev \
    libspnav-dev \
    && rm -rf /var/lib/apt/lists/*

# Generate locale (like in CI)
RUN sed -i "s/# ${LANG} UTF-8/${LANG} UTF-8/" /etc/locale.gen || true && \
    locale-gen ${LANG} || true

# Optional Python binding deps
RUN if [ "$BUILD_PY" = "1" ]; then \
        apt-get update && apt-get install -y pybind11-dev && \
        rm -rf /var/lib/apt/lists/*; \
    fi

WORKDIR /src

RUN git config --global --add safe.directory /src
