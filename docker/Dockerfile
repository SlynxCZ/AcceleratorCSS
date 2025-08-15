FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

WORKDIR /app

RUN apt update && apt install -y \
    make g++ git curl unzip software-properties-common apt-transport-https \
    && curl -sSL https://packages.microsoft.com/config/debian/11/packages-microsoft-prod.deb -o packages-microsoft-prod.deb \
    && dpkg -i packages-microsoft-prod.deb \
    && rm packages-microsoft-prod.deb \
    && apt update && apt install -y dotnet-sdk-8.0

RUN curl -fsSL https://xmake.io/shget.text | bash
ENV PATH="/root/.local/bin:${PATH}"

RUN git config --global --add safe.directory /app

COPY docker/docker-entrypoint.sh ./docker-entrypoint.sh
RUN sed -i 's/\r$//' ./docker-entrypoint.sh && chmod +x ./docker-entrypoint.sh

ENV HL2SDKCS2=/app/source/vendor/hl2sdk-cs2
ENV MMSOURCE112=/app/source/vendor/metamod-source

WORKDIR /app/source

CMD ["/bin/bash", "../docker-entrypoint.sh"]