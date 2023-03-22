package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"

	kafka "github.com/segmentio/kafka-go"
)

type KafkaWXData struct {
	Battery     float32          `json:"battery"`
	Temperature float32          `json:"temperature"`
	Fahrenheit  float32          `json:"f"`
	Pressure    float32          `json:"pressure"`
	Humidity    float32          `json:"humidity"`
	KafkaWind   KafkaWindData    `json:"w"`
	Rain        RainReading      `json:"r"`
	Lightning   LightningReading `json:"l"`
}

type KafkaWindData struct {
	Speed               float32 `json:"speed"`
	Direction           float32 `json:"direction"`
	Speed2MinAvg        float32 `json:"2mavg"`
	Direction2MinAvg    float32 `json:"2mdavg"`
	GustTenMinSpeed     float32 `json:"10mgust"`
	GustTenMinDirection float32 `json:"10mgdir"`
	MaxDailyGust        float32 `json:"dailyGustMax"`
}

type ApiResponse struct {
	Battery   float32          `json:"battery"`
	Uv        UVReading        `json:"uv"`
	Wind      WindReading      `json:"wind"`
	Thp       ThpReading       `json:"thp"`
	Rain      RainReading      `json:"rain"`
	Lightning LightningReading `json:"lightning"`
}

type UVReading struct {
	A     float32 `json:"a"`
	B     float32 `json:"b"`
	Index float32 `json:"index"`
}

type WindReading struct {
	Speed                     float32 `json:"speed"`
	Direction                 float32 `json:"direction"`
	Speed2MinuteAverage       float32 `json:"speed2MinuteAverage"`
	Direction2MinuteAverage   float32 `json:"direction2MinuteAverage"`
	GustTenMinuteMaxSpeed     float32 `json:"gustTenMinuteMaxSpeed"`
	GustTenMinuteMaxDirection float32 `json:"gustTenMinueMaxDirection"`
	MaxDailyGust              float32 `json:"maxDailyGust"`
}

type ThpReading struct {
	TempC    float32 `json:"tempC"`
	TempF    float32 `json:"tempF"`
	Humidity float32 `json:"humidity"`
	Pressure float32 `json:"pressure"`
}

type RainReading struct {
	Hour  float32 `json:"hour"`
	Daily float32 `json:"daily"`
}

type LightningReading struct {
	Strike   bool `json:"strike"`
	Distance int  `json:"distance"`
}

//func GetReadings() {
//	url := "http://10.20.70.19"
//
//	var stationReading ApiResponse
//
//	start := time.Now()
//	err := GetJson(url, &stationReading)
//	duration := time.Since(start)
//
//	if err != nil {
//		fmt.Printf("Error getting weather information: %s\n", err.Error())
//	} else {
//		fmt.Printf("%s\tBattery %f v\tDuration: %dms\n", time.Now().UTC().Local(), stationReading.Battery, duration.Milliseconds())
//	}
//}

//func GetJson(url string, target interface{}) error {
//	resp, err := client.Get(url)
//	if err != nil {
//		return err
//	}
//
//	defer resp.Body.Close()
//
//	return json.NewDecoder(resp.Body).Decode(target)
//}

// func main() {
// 	client = &http.Client{ Timeout: 10 * time.Second }
// 	for true {
// 		GetReadings()
// 		time.Sleep(time.Second * 30)
// 	}

// }

func main() {
	topic := "wxTopic"
	brokerURL := "kafka-1:9092"
	// TODO : Make the apiUrl a param that gets passed in through the Dockerfile or compose file.
	// For now this is fine
	apiURL := "http://10.20.70.10"

	for {
		// Send API request
		resp, err := http.Get(apiURL)
		if err != nil {
			fmt.Println("Error sending API request:", err)
			continue
		}
		defer resp.Body.Close()

		body, err := io.ReadAll(resp.Body)
		if err != nil {
			fmt.Println("Error reading API response body:", err)
			continue
		}

		// Unmarshal response JSON
		var stationReading ApiResponse
		err = json.Unmarshal(body, &stationReading)
		if err != nil {
			fmt.Println("Error unmarshaling API response JSON:", err)
			continue
		}

		// Create new message
		message := KafkaWXData{
			Battery:     stationReading.Battery,
			Temperature: stationReading.Thp.TempC,
			Fahrenheit:  stationReading.Thp.TempF,
			Humidity:    stationReading.Thp.Humidity,
			Pressure:    stationReading.Thp.Pressure,
			KafkaWind: KafkaWindData{
				Speed:               stationReading.Wind.Speed,
				Direction:           stationReading.Wind.Direction,
				Speed2MinAvg:        stationReading.Wind.Speed2MinuteAverage,
				Direction2MinAvg:    stationReading.Wind.Direction2MinuteAverage,
				GustTenMinSpeed:     stationReading.Wind.GustTenMinuteMaxSpeed,
				GustTenMinDirection: stationReading.Wind.GustTenMinuteMaxDirection,
				MaxDailyGust:        stationReading.Wind.MaxDailyGust,
			},
			Rain:      stationReading.Rain,
			Lightning: stationReading.Lightning,
		}

		// Marshal message to JSON
		messageJSON, err := json.Marshal(message)
		if err != nil {
			fmt.Println("Error marshaling message to JSON:", err)
			continue
		}

		// Send message to Kafka
		writer := &kafka.Writer{
			Addr:                   kafka.TCP(brokerURL),
			Topic:                  topic,
			Balancer:               &kafka.LeastBytes{},
			AllowAutoTopicCreation: true,
		}

		err = writer.WriteMessages(context.Background(),
			kafka.Message{
				Value: messageJSON,
			},
		)

		if err != nil {
			fmt.Println("Error writing to Kafka:", err)
			continue
		}

		fmt.Println("Message sent to Kafka:", string(messageJSON))

		// Wait for 1 second before sending the next request
		time.Sleep(1 * time.Second)
	}
}
