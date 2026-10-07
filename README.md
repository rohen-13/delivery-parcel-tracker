# Delivery Parcel Tracker

An IoT monitoring prototype written in C and Raylib. The first version simulates a parcel moving from a depot through two checkpoints to its destination.

## Current features

- A drawn delivery route and animated parcel marker.
- Parcel ID, delivery status and progress.
- Start/resume, pause and reset controls.
- A 20-second journey, with movement based on elapsed time.
- Explicit simulation mode; no physical hardware is required.
- Live simulated temperature with heat and cooling controls and a 30 C demo alert.
- Impact trigger and cumulative impact count. Reset clears delivery and sensor state.
- Sensor monitoring continues while parcel movement is paused.
- Simulated disconnect/reconnect controls, with frozen readings and stale-reading age while disconnected.
- Timestamped sensor history: the latest four events are displayed, with up to 32 retained in memory.
- Export retained sensor events to CSV using Export CSV or the E key, without overwriting earlier exports.

## Run on macOS

Install Apple's command line developer tools if needed:

```sh
xcode-select --install
```

With Homebrew installed, install the dependencies:

```sh
brew install raylib pkgconf
```

From this project folder:

```sh
make run
```

Close the window or press Escape to exit. `make clean` removes the generated build folder.

For a double-clickable Mac app, run `make app` and open `build/Delivery Parcel Tracker.app`. Raylib must remain installed on that Mac; this is a local app build rather than a standalone distributable.

## Next features

Physical hardware integration is not implemented. Temperature limits are fictional demonstration values. Event times are elapsed simulation seconds, not wall-clock timestamps; history is not saved between runs unless exported.

## Event export

Click **Export CSV (E)** or press E. Each export saves the currently retained events (at most 32), oldest first, with `elapsed_seconds`, `event` and `temperature_c` columns. Older events that have fallen out of the bounded history are not included. Export does not clear history and works while disconnected or paused.

Files are numbered `events-001.csv` through `events-999.csv` beside the executable: `build/` for `make run`, or `build/Delivery Parcel Tracker.app/Contents/MacOS/` for the Mac app. Right-click the app and choose Show Package Contents to access its exports. The dashboard confirms success or reports a failure; check directory write access or whether all 999 names are occupied before retrying. Existing exports are never overwritten. Reset clears only live history, not saved CSV files.

## Demo

Start the delivery, pause partway along the route, resume to reach the destination, then reset to return to the depot.
Use Trigger heat to raise the reading gradually to 40 C, Stop heat to cool to 20 C, and Trigger impact to record one impact. The heat alert appears at 30 C. Reset returns temperature to 20 C and clears impacts.
Disconnect tracker to freeze telemetry and disable sensor triggers. Reconnect resumes monitoring without clearing impacts or the heat scenario. Delivery movement remains independent of telemetry connection. Reset restores the connection and clears history, leaving a single reset event at time zero.

## Team

See [the team plan](docs/team-plan.md) for file ownership, each teammate's branch, tasks and the pull request workflow. Use `make test` to check delivery state transitions.

GitHub repository: https://github.com/rohen-13/delivery-parcel-tracker

Each group member maintains their own development journal. Store group screencasts in the shared OneDrive folder.
