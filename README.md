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

Tracker connection status and timestamped event history are planned and are not implemented yet. Temperature limits are fictional demonstration values.

## Demo

Start the delivery, pause partway along the route, resume to reach the destination, then reset to return to the depot.
Use Trigger heat to raise the reading gradually to 40 C, Stop heat to cool to 20 C, and Trigger impact to record one impact. The heat alert appears at 30 C. Reset returns temperature to 20 C and clears impacts.

## Team

See [the team plan](docs/team-plan.md) for file ownership, each teammate's branch, tasks and the pull request workflow. Use `make test` to check delivery state transitions.

GitHub repository: https://github.com/rohen-13/delivery-parcel-tracker

Each group member maintains their own development journal. Store group screencasts in the shared OneDrive folder.
