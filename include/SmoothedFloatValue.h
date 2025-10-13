#ifndef SMOOTHEDFLOATVALUE_H
#define SMOOTHEDFLOATVALUE_H

#include <Arduino.h>
#include <math.h>

/**
 * @brief Smoothly transitions a float value toward a target over time using linear interpolation.
 *        Supports separate rates for upward and downward transitions.
 */
class SmoothedFloatValue {
private:
    float currentValue;
    float targetValue;
    float upwardRatePerSecond;
    float downwardRatePerSecond;
    unsigned long lastUpdateMillis;

public:
    SmoothedFloatValue(float initialValue = 0.0f,
                       float upwardRate = 1.0f,
                       float downwardRate = 1.0f)
        : currentValue(initialValue), targetValue(initialValue),
          upwardRatePerSecond(upwardRate), downwardRatePerSecond(downwardRate),
          lastUpdateMillis(millis()) {}

    void setTarget(float value) {
        currentValue = get();
        targetValue = value;
        lastUpdateMillis = millis();
    }

    void setUpwardRatePerSecond(float rate) {
        upwardRatePerSecond = rate;
    }

    void setDownwardRatePerSecond(float rate) {
        downwardRatePerSecond = rate;
    }

    float getUpwardRatePerSecond() const {
        return upwardRatePerSecond;
    }

    float getDownwardRatePerSecond() const {
        return downwardRatePerSecond;
    }

    void reset(float value) {
        currentValue = targetValue = value;
        lastUpdateMillis = millis();
    }

    float get() {
        unsigned long now = millis();
        float elapsedSeconds = (now - lastUpdateMillis) / 1000.0f;
        lastUpdateMillis = now;

        float delta = targetValue - currentValue;
        float rate = (delta > 0) ? upwardRatePerSecond : downwardRatePerSecond;
        float maxStep = rate * elapsedSeconds;

        if (fabs(delta) <= maxStep) {
            currentValue = targetValue;
        } else {
            currentValue += (delta > 0 ? 1 : -1) * maxStep;
        }

        return currentValue;
    }
};

#endif // SMOOTHEDFLOATVALUE_H
