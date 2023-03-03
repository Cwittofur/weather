package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"time"
)

var client *http.Client

type ApiResponse struct {
	Battery float32 `json:"battery"`
	Uv UVReading `json:"uv"`
	Wind WindReading `json:"wind"`
	Thp ThpReading `json:"thp"`
	Rain RainReading `json:"rain"`
}

type UVReading struct {
	A float32 `json:"a"`
	B float32 `json:b"`
	Index float32 `json:"index"`
}

type WindReading struct {
	Speed float32 `json:"speed"`
	Direction float32 `json:"direction"`
	Speed2MinuteAverage float32 `json:"speed2MinuteAverage"`
	Direction2MinuteAverage float32 `json:"direction2MinuteAverage"`
	GustTenMinuteMaxSpeed float32 `json:"gustTenMinuteMaxSpeed"`
	GustTenMinuteMaxDirection float32 `json:"gustTenMinueMaxDirection"`
}

type ThpReading struct {
	TempC float32 `json:"tempC"`
	TempF float32 `json:"tempF"`
	Humidity float32 `json:"humidity"`
	Pressure float32 `json:"pressure"`
}

type RainReading struct {
	Hour float32 `json:"hour"`
	Daily float32 `json:"daily"`
}

func GetReadings() {
	url := "http://10.20.70.19"

	var stationReading ApiResponse


	start := time.Now()
	err := GetJson(url, &stationReading)
	duration := time.Since(start)
	
	if err != nil {
		fmt.Printf("Error getting weather information: %s\n", err.Error())
	} else {
		fmt.Printf("%s\tBattery %f v\tDuration: %dms\n", time.Now().UTC().Local() ,stationReading.Battery, duration.Milliseconds())
	}
}

func GetJson(url string, target interface{}) error {
	resp, err := client.Get(url)
	if err != nil {
		return err
	}

	defer resp.Body.Close()

	return json.NewDecoder(resp.Body).Decode(target)
}

func main() { 
	client = &http.Client{ Timeout: 10 * time.Second }
	for true {
		GetReadings()
		time.Sleep(time.Second * 30)
	}
	
}