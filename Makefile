CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib)
RAYLIB_LIBS = $(shell pkg-config --libs raylib)

.PHONY: all run app test clean
all: build/parcel-tracker

build/parcel-tracker: src/main.c src/dashboard.c src/tracker.c src/sensors.c src/account.c src/temperature_history.c src/temperature_history.h src/account.h src/dashboard.h src/tracker.h src/sensors.h
	mkdir -p build
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) src/main.c src/dashboard.c src/tracker.c src/sensors.c src/account.c src/temperature_history.c -o $@ $(RAYLIB_LIBS) -lm

run: all
	./build/parcel-tracker

app: all
	mkdir -p 'build/Delivery Parcel Tracker.app/Contents/MacOS'
	cp build/parcel-tracker 'build/Delivery Parcel Tracker.app/Contents/MacOS/parcel-tracker'
	cp packaging/Info.plist 'build/Delivery Parcel Tracker.app/Contents/Info.plist'

clean:
	rm -rf build

test:
	mkdir -p build
	$(CC) $(CFLAGS) tests/test_tracker.c src/tracker.c -o build/test-tracker
	./build/test-tracker
	$(CC) $(CFLAGS) tests/test_sensors.c src/sensors.c -o build/test-sensors -lm
	./build/test-sensors
	$(CC) $(CFLAGS) tests/test_account.c src/account.c -o build/test-account
	./build/test-account
	$(CC) $(CFLAGS) tests/test_temperature_history.c src/temperature_history.c -o build/test-temperature-history
	./build/test-temperature-history
