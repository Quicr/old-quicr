FROM ubuntu:20.04 
LABEL description="Build and test quicr"

ENV TZ=America/Vancouver
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update
RUN apt-get install -y tzdata build-essential pkg-config wget tcsh git curl

# Build CMake
RUN apt-get install -y libssl-dev 
WORKDIR /tmp 
RUN wget  https://github.com/Kitware/CMake/releases/download/v3.19.4/cmake-3.19.4.tar.gz
RUN tar xvfz cmake-3.19.4.tar.gz
WORKDIR /tmp/cmake-3.19.4
RUN ./bootstrap
RUN make -j 8
RUN make install


WORKDIR /usr/src/quicr
COPY . .

RUN mkdir -p /tmp/build
WORKDIR /tmp/build
RUN cmake /usr/src/quicr
RUN make -j 4 
RUN cp cmd/qbroadcast /usr/local/bin/.

#CMD /user/bin/tcsh

EXPOSE 5004/udp
CMD /usr/local/bin/qbroadcast

