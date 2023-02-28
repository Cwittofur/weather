# This is a simple way to get Kafka running with Docker Compose

- The code and ideas all originate from [That DevOps Guy](https://www.youtube.com/@MarcelDempers) on YouTube. More specifically his video [Running Kafka on Docker with Compose](https://www.youtube.com/watch?v=ncTosfaZ5cQ)

This Readme will contain the necessary information in order to get this running and operational using Kafka version 3.3.1

## Docker Compose

The `docker-compose.yml` file has everything needed in order to properly spin up the containers. The Consumer and Producer aren't necessary as they're just there to help demonstrate potential applications. 

`docker-compose build && docker-compose up` will build and start the containers. It may take a bit on the first run.

Once the containers are running you can exec into one of the kafka brokers. This is where you'll create your topic.

`docker exec -it kafka-1 bash`

Once inside there you can run:

`./kafka-topics.sh --create --topic TestTopicName --bootstrap-server kafka-1:9092`

This will create the topic and now you can connect and send messages!

## Sending Messages

Using the `kafka-producer` 
