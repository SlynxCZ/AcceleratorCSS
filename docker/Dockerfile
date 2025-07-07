FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

RUN apt update && apt install -y make g++ git curl unzip software-properties-common apt-transport-https

RUN curl https://packages.microsoft.com/config/debian/12/packages-microsoft-prod.deb -o packages-microsoft-prod.deb && \
    dpkg -i packages-microsoft-prod.deb && \
    rm packages-microsoft-prod.deb && \
    apt update && \
    apt install -y dotnet-sdk-8.0

RUN curl -L -o premake5.zip https://github.com/premake/premake-core/releases/download/v5.0.0-alpha16/premake-5.0.0-alpha16-linux.tar.gz && \
    tar -xzf premake5.zip && \
    mv premake5 /usr/local/bin/ && \
    rm premake5.zip

WORKDIR /project