FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV QT_QPA_PLATFORM=offscreen

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    qt6-base-dev \
    libpq-dev \
    qt6-base-dev-tools \
    libqt6sql6-psql \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

WORKDIR /app/server

RUN cmake -Bbuild -S. && cmake --build build

EXPOSE 8080

CMD ["./build/TrainTicketsServer"]
