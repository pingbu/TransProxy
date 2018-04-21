#!/bin/sh
kill `pidof transproxy`
./transproxy --daemon
