#!/usr/bin/env bash
set -e

OUT=certs
mkdir -p "$OUT"

# CA
openssl genrsa -out "$OUT/ca.key" 4096
openssl req -new -x509 -days 3650 -key "$OUT/ca.key" -out "$OUT/ca.crt" \
  -subj "/CN=MyCA"

# Server
openssl genrsa -out "$OUT/server.key" 2048
openssl req -new -key "$OUT/server.key" -out "$OUT/server.csr" \
  -subj "/CN=server"
openssl x509 -req -days 3650 -in "$OUT/server.csr" \
  -CA "$OUT/ca.crt" -CAkey "$OUT/ca.key" -CAcreateserial \
  -extfile <(printf "subjectAltName=IP:127.0.0.1,DNS:localhost") \
  -out "$OUT/server.crt"

# Client (flash ca.crt + client.crt + client.key onto ESP32)
openssl genrsa -out "$OUT/client.key" 2048
openssl req -new -key "$OUT/client.key" -out "$OUT/client.csr" \
  -subj "/CN=esp32-client"
openssl x509 -req -days 3650 -in "$OUT/client.csr" \
  -CA "$OUT/ca.crt" -CAkey "$OUT/ca.key" -CAcreateserial \
  -out "$OUT/client.crt"

echo "Done. Files in $OUT/"
echo "  Flash onto ESP32: ca.crt, client.crt, client.key"
