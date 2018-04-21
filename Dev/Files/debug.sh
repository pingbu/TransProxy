#!/bin/sh
kill `pidof transproxy`
./transproxy --log-level=6
