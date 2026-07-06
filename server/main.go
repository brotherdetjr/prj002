package main

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"log"
	"net/http"
	"os"
)

func mustEnv(key string) string {
	v := os.Getenv(key)
	if v == "" {
		log.Fatalf("required env var %s is not set", key)
	}
	return v
}

func main() {
	caCert, err := os.ReadFile("certs/ca.crt")
	if err != nil {
		log.Fatalf("read CA cert: %v", err)
	}
	caPool := x509.NewCertPool()
	if !caPool.AppendCertsFromPEM(caCert) {
		log.Fatal("failed to parse CA cert")
	}

	serverCert, err := tls.LoadX509KeyPair("certs/server.crt", "certs/server.key")
	if err != nil {
		log.Fatalf("load server cert: %v", err)
	}

	tlsCfg := &tls.Config{
		ClientAuth:   tls.RequireAndVerifyClientCert,
		ClientCAs:    caPool,
		Certificates: []tls.Certificate{serverCert},
		MinVersion:   tls.VersionTLS12,
	}

	rdClient := newRDClient(
		mustEnv("RAILDATA_DEPARTURE_KEY"),
		mustEnv("RAILDATA_SERVICE_KEY"),
	)

	mux := http.NewServeMux()
	mux.Handle("GET /api/v1/stations/{crs}/platforms/{platform}", &stationsHandler{client: rdClient})
	mux.HandleFunc("/hello", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		fmt.Fprintln(w, "hello world")
	})

	srv := &http.Server{
		Addr:      ":8443",
		Handler:   mux,
		TLSConfig: tlsCfg,
	}

	log.Println("listening on :8443 (mTLS)")
	log.Fatal(srv.ListenAndServeTLS("", ""))
}
