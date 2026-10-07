# Sensor simulation module

Implemented on Tymofii's tracking branch with Codex assistance on 6 October 2026. This is a C module with no Raylib dependency. `make test` runs both tracking and sensor checks.

## Behaviour

Reset establishes 20 degrees Celsius, disables heating and clears the impact count. Normal readings remain at that baseline. Triggering heat changes the target to 40 degrees; elapsed-time updates move the reading towards it at 2 degrees per second. The heat alert is active at or above 30 degrees. Stopping heat cools towards 20 degrees at the same rate. These are fictional demonstration parameters, not safe transport limits.

Each connected `SensorsTriggerImpact` call increments the count once. Subsequent time updates do not generate another impact. The count saturates at the maximum unsigned integer instead of wrapping. Reset clears all sensor state and restores the connection. Zero, negative and non-finite elapsed-time inputs are ignored.

`SensorsSetConnected` simulates a telemetry connection. Disconnected updates advance elapsed time and reading age but freeze temperature and impacts; heat and impact controls are ignored. Reconnection clears the stale age and resumes the existing heat scenario. This does not affect delivery movement.

The module retains the latest 32 sensor events in chronological order. Events capture elapsed simulation seconds, a kind and temperature; heat threshold crossings record their crossing time and 30-degree reading even for a large update. Reset clears history and records one reset event at zero. History is memory-only and does not use wall-clock timestamps.

## Integration contract for review

Create a `Sensors` instance and call `SensorsReset` before use. Call `SensorsUpdate` once per frame with elapsed seconds. The module does not depend on `Tracker.running`, so readings remain active while parcel movement is paused.

The dashboard can read `temperature_c`, `impact_count` and `SensorsHeatAlert`. Its heat, stop-heat and impact controls should call the corresponding functions once per button press. The application's Reset control should reset both the tracker and sensors.

The application initialises and updates the module each frame. The dashboard displays temperature, heat alert, impact count, connection state, stale age and the latest four events. Sensor controls are disabled while disconnected. Reset clears both tracker and sensor state. Monitoring continues while parcel movement is paused.

`SensorsWriteCSV` writes all retained events to a caller-owned stream, oldest first, with elapsed seconds, event name and Celsius reading columns. It flushes the output and reports write errors, but does not close the stream or change sensor state. The dashboard creates numbered CSV files exclusively beside the executable, checks both writing and closing, and shows success/failure feedback. Export includes at most 32 retained events, not a full-session archive.

## Verification

`make test` verifies baseline readings, gradual heating, the alert threshold, target clamping, cooling and alert resolution, one impact per trigger, invalid elapsed-time inputs, frame-size independence, connection transitions, frozen disconnected readings, bounded chronological history, threshold event snapshots, CSV columns/order/capacity/write-error handling and full reset. `make app` builds the integrated Raylib application.
