#ifndef DAMPENEDBOOLVALUE_H
#define DAMPENEDBOOLVALUE_H

#include <Arduino.h>

/**
 * @brief Delays boolean transitions to avoid abrupt or noisy changes.
 *        The value only flips after a specified duration has passed.
 */
class DampenedBoolValue {
private:
    bool currentValue;
    bool targetValue;
    float transitionDelaySeconds;
    unsigned long lastTargetChangeMillis;

    bool isTransitioned() const {
        return currentValue != targetValue &&
               (millis() - lastTargetChangeMillis >= transitionDelaySeconds * 1000.0f);
    }

public:
    DampenedBoolValue(bool initialValue = false, float delaySeconds = 1.0f)
        : currentValue(initialValue), targetValue(initialValue),
          transitionDelaySeconds(delaySeconds), lastTargetChangeMillis(millis()) {}

    void setTarget(bool value) {
        currentValue = get(); // Apply any pending transition
        if (value != targetValue) {
            targetValue = value;
            lastTargetChangeMillis = millis();
        }
    }

    void reset(bool value) {
        currentValue = targetValue = value;
        lastTargetChangeMillis = millis();
    }

    void setTransitionDelaySeconds(float delaySeconds) {
        transitionDelaySeconds = delaySeconds;
    }

    float getTransitionDelaySeconds() const {
        return transitionDelaySeconds;
    }

    bool get() {
        if (isTransitioned()) {
            currentValue = targetValue;
            lastTargetChangeMillis = millis();
        }
        return currentValue;
    }
};

#endif // DAMPENEDBOOLVALUE_H
