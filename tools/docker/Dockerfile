FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive
ENV HOME=/home/big

VOLUME ["${HOME}"]
COPY entrypoint.sh /sbin/
RUN chmod 755 /sbin/entrypoint.sh
COPY build/src/hashahead/hashahead* /usr/bin/
COPY hashahead.conf /hashahead.conf
EXPOSE 8811 8812 8813 8814 8815
ENTRYPOINT ["/sbin/entrypoint.sh"]
CMD ["hashahead"]
