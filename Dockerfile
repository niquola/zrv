FROM ubuntu

RUN apt-get update && apt-get install -y git cmake zlib1g-dev pkg-config vim curl

WORKDIR /zrv

CMD exec /bin/bash -c "trap : TERM INT; sleep infinity & wait"
