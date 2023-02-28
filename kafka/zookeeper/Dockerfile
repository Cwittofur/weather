FROM openjdk:11.0.10-jre-buster

RUN apt-get update && \
    apt-get install -y curl

ENV KAFKA_VERSION 3.3.1
ENV SCALA_VERSION 2.13

RUN mkdir /tmp/kafka && \
    curl "https://archive.apache.org/dist/kafka/${KAFKA_VERSION}/kafka_${SCALA_VERSION}-${KAFKA_VERSION}.tgz" \
    -o /tmp/kafka/kafka.tgz && \
    mkdir /kafka && cd /kafka && \
    tar -xvzf /tmp/kafka/kafka.tgz --strip 1

COPY start-zookeeper.sh /usr/bin
RUN chmod +x /usr/bin/start-zookeeper.sh

CMD ["start-zookeeper.sh"]
