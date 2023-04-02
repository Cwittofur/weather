package main

import (
	"context"
	"encoding/json"
	"fmt"
	"github.com/go-co-op/gocron"
	"github.com/segmentio/kafka-go"
	"io"
	"net/http"
	"os"
	"strings"
	"time"

	_ "github.com/go-co-op/gocron"
	_ "github.com/segmentio/kafka-go"
)

type KafkaWXData struct {
	//Battery     float32          `json:"battery"`
	Timestamp   int64            `json:"timestamp"`
	Temperature float32          `json:"temperature"`
	Fahrenheit  float32          `json:"f"`
	Pressure    float32          `json:"pressure"`
	Humidity    float32          `json:"humidity"`
	KafkaWind   KafkaWindData    `json:"w"`
	Rain        RainReading      `json:"r"`
	Lightning   LightningReading `json:"l"`
}

type KafkaWxDataV2 struct {
	T     int64   `json:"timestamp"`
	TC    float32 `json:"tempC""`
	TF    float32 `json:"tempF"`
	H     float32 `json:"humidity"`
	P     float32 `json:"pressure"`
	WS    float32 `json:"windSpeed"`
	WD    float32 `json:"windDirection"`
	WS2M  float32 `json:"2mavg"`
	WD2M  float32 `json:"2mdavg"`
	G10M  float32 `json:"10mgust"`
	G10MD float32 `json:"10mgdir"`
	MDG   float32 `json:"maxDailyGust"`
	R1H   float32 `json:"rain1Hour"`
	RD    float32 `json:"rain24H"`
	UVI   float32 `json:"uvIndex"`
	L     int     `json:"lightning"`
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

func GetWeatherData(isMidnight bool) ApiResponse {
	apiURL := os.Getenv("STATION_URL")

	if isMidnight == true {
		apiURL = apiURL + "/m"
	}

	resp, err := http.Get(apiURL)
	if err != nil {
		fmt.Println("Error sending API request:", err)
		return ApiResponse{}
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		fmt.Println("Error reading API response body:", err)
		return ApiResponse{}
	}

	// Unmarshal response JSON
	var stationReading ApiResponse
	err = json.Unmarshal(body, &stationReading)
	if err != nil {
		fmt.Println("Error unmarshaling API response JSON:", err)
		return ApiResponse{}
	}

	return stationReading
}

func SendDataToKafka(data []byte, topic string) {
	brokerURL := os.Getenv("KAFKA_BROKERS")

	writer := &kafka.Writer{
		Addr:                   kafka.TCP(strings.Split(brokerURL, ",")...),
		Topic:                  topic,
		Balancer:               &kafka.LeastBytes{},
		AllowAutoTopicCreation: true,
	}

	err := writer.WriteMessages(context.Background(),
		kafka.Message{
			Value: data,
		},
	)

	if err != nil {
		fmt.Println("Error writing to Kafka:", err)
		return
	}

	fmt.Println("Message sent to Kafka:", string(data))
}

func Wind() {
	stationReading := GetWeatherData(false)

	message := KafkaWindData{
		Speed:               stationReading.Wind.Speed,
		Direction:           stationReading.Wind.Direction,
		Speed2MinAvg:        stationReading.Wind.Speed2MinuteAverage,
		Direction2MinAvg:    stationReading.Wind.Direction2MinuteAverage,
		GustTenMinSpeed:     stationReading.Wind.GustTenMinuteMaxSpeed,
		GustTenMinDirection: stationReading.Wind.GustTenMinuteMaxDirection,
		MaxDailyGust:        stationReading.Wind.MaxDailyGust,
	}

	marshaledMessage := MarshalToJSON(message)

	if marshaledMessage != nil {
		SendDataToKafka(marshaledMessage, "wxWind")
	}
}

func Rain() {
	stationReading := GetWeatherData(false)

	message := RainReading{
		Hour:  stationReading.Rain.Hour,
		Daily: stationReading.Rain.Daily,
	}

	marshaledMessage := MarshalToJSON(message)

	if marshaledMessage != nil {
		SendDataToKafka(marshaledMessage, "wxRain")
	}
}

func THP() {
	stationReading := GetWeatherData(false)

	message := ThpReading{
		TempC:    stationReading.Thp.TempC,
		TempF:    stationReading.Thp.TempF,
		Humidity: stationReading.Thp.Humidity,
		Pressure: stationReading.Thp.Pressure,
	}

	marshaledMessage := MarshalToJSON(message)

	if marshaledMessage != nil {
		SendDataToKafka(marshaledMessage, "wxTHP")
	}
}

func MarshalToJSON(message interface{}) []byte {
	// Marshal message to JSON
	messageJSON, err := json.Marshal(message)
	if err != nil {
		fmt.Println("Error marshaling message to JSON:", err)
		return nil
	}

	return messageJSON
}

func fetchAndPushDataToKafka(isMidnight bool) {

	// TODO : Make the apiUrl a param that gets passed in through the Dockerfile or compose file.
	// For now this is fine

	// Send API request

	// Create new message
	// message := KafkaWXData{
	//	Timestamp:   time.Now().Unix(),
	//	Temperature: stationReading.Thp.TempC,
	//	Fahrenheit:  stationReading.Thp.TempF,
	//	Humidity:    stationReading.Thp.Humidity,
	//	Pressure:    stationReading.Thp.Pressure,
	//	KafkaWind: KafkaWindData{
	//		Speed:               stationReading.Wind.Speed,
	//		Direction:           stationReading.Wind.Direction,
	//		Speed2MinAvg:        stationReading.Wind.Speed2MinuteAverage,
	//		Direction2MinAvg:    stationReading.Wind.Direction2MinuteAverage,
	//		GustTenMinSpeed:     stationReading.Wind.GustTenMinuteMaxSpeed,
	//		GustTenMinDirection: stationReading.Wind.GustTenMinuteMaxDirection,
	//		MaxDailyGust:        stationReading.Wind.MaxDailyGust,
	//	},
	//	Rain:      stationReading.Rain,
	//	Lightning: stationReading.Lightning,
	//}
	stationReading := GetWeatherData(isMidnight)

	message := KafkaWxDataV2{
		T:     time.Now().Unix(),
		TC:    stationReading.Thp.TempC,
		TF:    stationReading.Thp.TempF,
		H:     stationReading.Thp.Humidity,
		P:     stationReading.Thp.Pressure,
		WS:    stationReading.Wind.Speed,
		WD:    stationReading.Wind.Direction,
		WS2M:  stationReading.Wind.Speed2MinuteAverage,
		WD2M:  stationReading.Wind.Direction2MinuteAverage,
		G10M:  stationReading.Wind.GustTenMinuteMaxSpeed,
		G10MD: stationReading.Wind.GustTenMinuteMaxDirection,
		MDG:   stationReading.Wind.MaxDailyGust,
		R1H:   stationReading.Rain.Hour,
		RD:    stationReading.Rain.Daily,
		UVI:   stationReading.Uv.Index,
		L:     stationReading.Lightning.Distance,
	}

	marshaledMessage := MarshalToJSON(message)

	SendDataToKafka(
		marshaledMessage,
		"wxTopic",
	)
	// Send message to Kafka
}

func main() {
	s := gocron.NewScheduler(time.Local)

	s.Every(1).Seconds().Do(func() {
		Wind()
	})

	s.Every(5).Seconds().Do(func() {
		THP()
	})

	s.Every(1).Minutes().Do(func() {
		Rain()
	})

	s.Every(1).Day().At("00:00").Do(func() {
		fetchAndPushDataToKafka(true)
	})

	s.StartBlocking()
}
