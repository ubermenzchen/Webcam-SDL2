#!/usr/bin/env bash

gcc -o bin/server -Wall -pedantic -Iinclude -pthread src/backend/getframes.c src/backend/v4l2_driver.c src/socket.c $(pkg-config --cflags --libs sdl2)