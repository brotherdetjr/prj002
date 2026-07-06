package main

import (
	"encoding/json"
	"net/http"
	"sort"
	"time"
)

type stationsHandler struct {
	client *rdClient
}

// --- Response types ---

type platformResp struct {
	Rows        []rowEntry       `json:"rows"`
	CallingAt   []callingAtEntry `json:"callingAt"`
	GeneratedAt string           `json:"generatedAt"`
}

type rowEntry struct {
	Std         string `json:"std"`
	Etd         string `json:"etd"`
	Destination string `json:"destination"`
}

type callingAtEntry struct {
	LocationName string `json:"locationName"`
	Et           string `json:"et"`
}

// --- Handler ---

func (h *stationsHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	crs := r.PathValue("crs")
	platform := r.PathValue("platform")

	board, err := h.client.fetchDepartureBoard(r.Context(), crs)
	if err != nil {
		http.Error(w, "upstream error", http.StatusBadGateway)
		return
	}

	// Collect services for the requested platform — trains and buses merged.
	var svcs []rdService
	for _, s := range board.TrainServices {
		if s.Platform == platform {
			svcs = append(svcs, s)
		}
	}
	for _, s := range board.BusServices {
		if s.Platform == platform {
			svcs = append(svcs, s)
		}
	}

	// Find the effective etd of the first non-cancelled service.
	var refEtd string
	for _, s := range svcs {
		if !svcCancelled(s) {
			refEtd = effectiveEtd(s)
			break
		}
	}

	// Drop cancelled services whose std is before the first non-cancelled's effective etd.
	if refEtd != "" {
		refT, refOK := parseHHMM(refEtd)
		var kept []rdService
		for _, s := range svcs {
			if svcCancelled(s) && refOK {
				if t, ok := parseHHMM(s.Std); ok && t.Before(refT) {
					continue
				}
			}
			kept = append(kept, s)
		}
		svcs = kept
	}

	// Sort remaining ascending by effective etd.
	sort.SliceStable(svcs, func(i, j int) bool {
		return etdSortKey(svcs[i]).Before(etdSortKey(svcs[j]))
	})

	// Build rows.
	rows := make([]rowEntry, 0, len(svcs))
	for _, s := range svcs {
		dest := ""
		if len(s.Destination) > 0 {
			dest = s.Destination[0].LocationName
		}
		rows = append(rows, rowEntry{Std: s.Std, Etd: s.Etd, Destination: dest})
	}

	// Fetch calling points for the earliest non-cancelled service.
	callingAt := []callingAtEntry{}
	for _, s := range svcs {
		if svcCancelled(s) {
			continue
		}
		if details, err := h.client.fetchServiceDetails(r.Context(), s.ServiceID); err == nil {
			callingAt = extractCallingPoints(details)
		}
		break
	}

	generatedAt := time.Now().UTC().Format(time.RFC3339)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(platformResp{
		Rows:        rows,
		CallingAt:   callingAt,
		GeneratedAt: generatedAt,
	})
}

// --- Helpers ---

func svcCancelled(s rdService) bool {
	return s.IsCancelled || s.Etd == "Cancelled"
}

// effectiveEtd returns std when etd is "On time", otherwise etd.
func effectiveEtd(s rdService) string {
	if s.Etd == "On time" {
		return s.Std
	}
	return s.Etd
}

func parseHHMM(s string) (time.Time, bool) {
	t, err := time.Parse("15:04", s)
	return t, err == nil
}

// etdSortKey returns a comparable time for sorting. For unparseable etd values
// (e.g. "Cancelled", "Delayed"), falls back to std; if that also fails, sorts last.
func etdSortKey(s rdService) time.Time {
	if t, ok := parseHHMM(effectiveEtd(s)); ok {
		return t
	}
	if t, ok := parseHHMM(s.Std); ok {
		return t
	}
	return time.Date(0, 1, 1, 23, 59, 59, 0, time.UTC)
}

func extractCallingPoints(resp *serviceDetailsResp) []callingAtEntry {
	var result []callingAtEntry
	for _, list := range resp.SubsequentCallingPoints {
		for _, cp := range list.CallingPoint {
			result = append(result, callingAtEntry{
				LocationName: cp.LocationName,
				Et:           resolveCallingPointTime(cp),
			})
		}
	}
	return result
}

// resolveCallingPointTime returns the best HH:mm time for a calling point.
// Priority: at (train already passed) → et resolved to HH:mm → st.
func resolveCallingPointTime(cp rdCallingPoint) string {
	if cp.At != "" {
		return cp.At
	}
	if cp.Et == "On time" || cp.Et == "" {
		return cp.St
	}
	return cp.Et
}
