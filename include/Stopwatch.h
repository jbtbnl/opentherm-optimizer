#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <Arduino.h>

class Stopwatch {
private:
    bool running;
    unsigned long startTime;
    unsigned long elapsed;

public:
    Stopwatch() : running(false), startTime(0), elapsed(0) {}

    void start() {
        if (!running) {
            startTime = millis();
            running = true;
        }
    }

    void stop() {
        if (running) {
            elapsed += millis() - startTime;
            running = false;
        }
    }

    void reset() {
        running = false;
        elapsed = 0;
        startTime = 0;
    }

    bool isRunning() const {
        return running;
    }

    unsigned long getElapsedMilliseconds() const {
        return running ? elapsed + (millis() - startTime) : elapsed;
    }

    unsigned long getElapsedSeconds() const {
        return getElapsedMilliseconds() / 1000;
    }

    unsigned long getElapsedMinutes() const {
        return getElapsedMilliseconds() / 60000;
    }

    String getFormattedTime() const {
        unsigned long totalSeconds = getElapsedSeconds();
        unsigned long minutes = totalSeconds / 60;
        unsigned long seconds = totalSeconds % 60;

        char buffer[6]; // "MM:SS" + null terminator
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu", minutes, seconds);
        return String(buffer);
    }
};

#endif // STOPWATCH_H
