FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

RUN apt update && apt install -y bear make g++ git curl unzip

# Nainstaluj Premake5 ručně
RUN curl -L -o premake5.zip https://github.com/premake/premake-core/releases/download/v5.0.0-alpha16/premake-5.0.0-alpha16-linux.tar.gz && \
    tar -xzf premake5.zip && \
    mv premake5 /usr/local/bin/ && \
    rm premake5.zip

WORKDIR /project