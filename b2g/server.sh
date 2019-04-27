#!/bin/bash

adb reverse tcp:8081 tcp:8081
python -m SimpleHTTPServer 8081
