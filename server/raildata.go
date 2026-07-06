package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"time"
)

var debugRaildata = os.Getenv("RAILDATA_DEBUG") == "1"

const (
	departureBoardBase = "https://api1.raildata.org.uk/1010-live-departure-board-dep1_2/LDBWS/api/20220120/GetDepartureBoard"
	serviceDetailsBase = "https://api1.raildata.org.uk/1010-service-details1_2/LDBWS/api/20220120/GetServiceDetails"
)

type rdClient struct {
	departureKey string
	serviceKey   string
	hc           *http.Client
}

func newRDClient(departureKey, serviceKey string) *rdClient {
	return &rdClient{
		departureKey: departureKey,
		serviceKey:   serviceKey,
		hc:           &http.Client{Timeout: 10 * time.Second},
	}
}

// --- Departure board types ---

type departureBoardResp struct {
	TrainServices []rdService `json:"trainServices"`
	BusServices   []rdService `json:"busServices"`
}

type rdService struct {
	Std         string `json:"std"`
	Etd         string `json:"etd"`
	Platform    string `json:"platform"`
	IsCancelled bool   `json:"isCancelled"`
	ServiceID   string `json:"serviceID"`
	Destination []struct {
		LocationName string `json:"locationName"`
	} `json:"destination"`
}

// --- Service details types ---
// NOTE: callingPointList nesting is a guess — adjust if the raw response differs.

type serviceDetailsResp struct {
	SubsequentCallingPoints []struct {
		CallingPoint []rdCallingPoint `json:"callingPoint"`
	} `json:"subsequentCallingPoints"`
}

type rdCallingPoint struct {
	LocationName string `json:"locationName"`
	St           string `json:"st"`
	Et           string `json:"et"`
	At           string `json:"at"`
}

// --- API calls ---

func (c *rdClient) fetchDepartureBoard(ctx context.Context, crs string) (*departureBoardResp, error) {
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, departureBoardBase+"/"+url.PathEscape(crs), nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("x-apikey", c.departureKey)
	resp, err := c.hc.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("departure board returned %d", resp.StatusCode)
	}
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}
	if debugRaildata {
		log.Printf("departure board raw response:\n%s", prettyJSON(body))
	}
	var out departureBoardResp
	return &out, json.Unmarshal(body, &out)
}

func (c *rdClient) fetchServiceDetails(ctx context.Context, serviceID string) (*serviceDetailsResp, error) {
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, serviceDetailsBase+"/"+url.PathEscape(serviceID), nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("x-apikey", c.serviceKey)
	resp, err := c.hc.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("service details returned %d", resp.StatusCode)
	}
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}
	if debugRaildata {
		log.Printf("service details raw response:\n%s", prettyJSON(body))
	}
	var out serviceDetailsResp
	return &out, json.Unmarshal(body, &out)
}

func prettyJSON(b []byte) []byte {
	var buf bytes.Buffer
	if err := json.Indent(&buf, b, "", "  "); err != nil {
		return b
	}
	return buf.Bytes()
}
