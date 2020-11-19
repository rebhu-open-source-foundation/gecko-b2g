#!/bin/bash

set -x -e

if [[ ! $1 ]]; then
  echo "Usage ./update_fingerprint.sh API_DAEMON_ROOT"
  exit 1
fi

API_DAEMON_ROOT=$1

function copy() {
cp $API_DAEMON_ROOT/services/$1/src/generated/gecko_client.rs ./src/services/$2/messages.rs
}

copy time time
copy settings settings
copy contacts contacts
copy geckobridge bridge

cargo fmt
