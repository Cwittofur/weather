# wx

Second endeavor into weather sensors and data acquisition. I'm designing this system a bit differently. I have really admired the way that Fractal Xaos implemented this and the design works perfectly for separating items into different VLAN's.

Heavily inspired by [Fractal Xaos Github](https://github.com/fractalxaos/weather)

# Equipment

- [ESP32 MicroMod Processor](https://www.sparkfun.com/products/16781)
- [Sparkfun Weather MicroMod Board](https://www.sparkfun.com/products/16794)
- [Weather Meter Kit](https://www.sparkfun.com/products/15901)
- [25 Watt Solar Panel and Charger](https://a.co/d/0ka89Kf)
- [12v 9ah battery](https://a.co/d/5CkOYxa)
- [Waterproof junction box](https://a.co/d/6Vuiqeo)

I did buy extra cable gland joints to have a smaller diameter hole in the junction box. It's not 100% necessary to go smaller. I have the station mounted on a 4x4x8 pressure treated wood, buried 20" in the ground and held in place with some cement.

# Architecture

The weather station itself is isolated. It will continuously collect data. The `service-relay` will query the station via REST api calls to get data. It then transforms the data (ever so slightly) and then publishes it to Kafka. From here, services can be attached and detached without data loss (for the most part). In order to graph out the data, there are a couple of helping services. `telegraf` directly connects to the Kafka queue and will consume and push data to `influxdb`. The `grafana-dashboard` then consumes from influxdb to provide chart data of weather events. In the future, there will be another consumer that will store data into a long term database (PostgreSQL).

# Getting Started

- Inside the Arduino folder, open up the `Preferences` code, fill in your information for WiFi (SSID and Password). This will be stored in the EEPROM of your ESP32. Once loaded and uploaded to your board, you can then open the `WeatherStation` project and upload that to your board. It'll read from the EEPROM to get your WiFi information without the need to hardcode it.
- Now that you have gotten the Arduino piece setup, edit the `STATION_URL` in the `docker-compose.yml` file. This is the IP address or URL for the weather station. After that `docker-compose up -d` should be all that's needed to get all the containers running and actively collecting data.
