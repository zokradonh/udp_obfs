FROM debian:bullseye AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        apt-transport-https \
        build-essential \
        ca-certificates \
        curl \
        libc6-dev \
        && \
    rm -rf /var/cache/apt /var/lib/apt/lists/*

COPY udp_obfs.c .

RUN mkdir /app && \
    gcc -o /app/udp_obfs udp_obfs.c

FROM debian:bullseye

COPY --from=builder /app /app

ENTRYPOINT [ "/app/udp_obfs" ]

