#!/usr/bin/env bash

gcc -o bin/client -Wall -pedantic -Iinclude -pthread src/frontend/screen.c src/socket.c $(pkg-config --cflags --libs sdl2)