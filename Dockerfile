FROM devkitpro/devkitppc:latest

RUN apt-get update && apt-get -y install --no-install-recommends build-essential libbluetooth-dev && rm -rf /var/lib/apt/lists/*

WORKDIR /project
