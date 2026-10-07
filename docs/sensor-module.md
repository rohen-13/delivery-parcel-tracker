# Sensor simulation module

Implemented on Tymofii's tracking branch with Codex assistance on 6 October 2026. This is a C module with no Raylib dependency. `make test` runs both tracking and sensor checks.

## Behaviour

Reset establishes 20 degrees Celsius, disables heating and clears the impact count. Normal readings remain at that baseline. Triggering heat changes the target to 40 degrees; elapsed-time updates move the reading towards it at 2 degrees per second. The heat alert is active at or above 30 degrees. Stopping heat cools towards 20 degrees at the same rate. These are fictional demonstration parameters, not safe transport limits.

Each `SensorsTriggerImpact` call increments the count once. Subsequent time updates do not generate another impact. The count saturates at the maximum unsigned integer instead of wrapping. Reset clears all sensor state. Zero, negative and non-finite elapsed-time inputs are ignored.

## Integration contract for review

Create a `Sensors` instance and call `SensorsReset` before use. Call `SensorsUpdate` once per frame with elapsed seconds. The module does not depend on `Tracker.running`, so the caller can keep readings active while parcel movement is paused. This is the proposed policy; agree it with Oleksandr before dashboard integration.

The dashboard can read `temperature_c`, `impact_count` and `SensorsHeatAlert`. Its heat, stop-heat and impact controls should call the corresponding functions once per button press. The application's Reset control should reset both the tracker and sensors.

The application now initialises and updates the module each frame. The dashboard displays temperature, heat alert and impact count, and provides heat, stop-heat and impact controls. Reset clears both tracker and sensor state. Monitoring continues while parcel movement is paused. Connection status, timestamped event history and CSV export remain future work.

## Verification

The new test first failed because the sensor implementation did not exist. After implementation, `make test` verifies baseline readings, gradual heating, the alert threshold, target clamping, cooling and alert resolution, one impact per trigger, invalid elapsed-time inputs, frame-size independence and full reset. Raylib is now installed and the integrated application builds successfully.
