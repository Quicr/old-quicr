FROM alpine:latest as builder
LABEL description="Build QuicR Relay"

RUN apk add --no-cache cmake alpine-sdk openssl-dev

RUN mkdir -p /src/quicr
WORKDIR /src/quicr
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
WORKDIR /src/quicr/build
RUN make -j 4
RUN cp  cmd/relay/relay_app  /usr/local/bin/quicrRelay
RUN ls -lh /usr/local/bin/quicrRelay

#RUN apk add --no-cache tcsh bash
#CMD /bin/tcsh

FROM alpine:latest
RUN apk add --no-cache libstdc++
COPY --from=builder /usr/local/bin/quicrRelay /usr/local/bin/quicrRelay

RUN addgroup -S quicr
RUN adduser -D -S -S -G quicr quicr
USER quicr
WORKDIR /home/quicr
EXPOSE 5004/udp
CMD /usr/local/bin/quicrRelay

#RUN apk add --no-cache bash tcsh
#CMD /bin/tcsh

