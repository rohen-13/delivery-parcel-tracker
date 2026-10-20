# Delivery Parcel Tracker

An IoT monitoring prototype written in C and Raylib. The first version simulates a parcel moving from a depot through two checkpoints to its destination.

## Current features

### Dispatch demo

The app opens with the shared local demo sign-in: username `demo`, password `parcel123`. The same session opens dispatch, profile and sensor monitoring. These are public demonstration values, not real authentication; do not enter personal passwords.

After sign-in, five fictional package deliveries at Waterford landmarks appear beside a fixed-area OpenStreetMap view. Select a row or map marker and toggle its checkbox to include it. A planned route labels each row with its delivery order and selects the next stop. **Deliver next route stop** records that delivery and moves the blue driver marker there. Delivered packages are excluded from subsequent routes. **Reset delivery round** restores packages and the driver to the depot. State stays in memory; there is no live GPS, address search, arbitrary stop entry or per-package sensor feed.

Choose **Load OSM map (L)** to fetch just the 12 tiles covering the visible area. Tiles are cached beside the executable in `map-cache/` for seven days. Attribution remains on the map and links to OpenStreetMap copyright information. No bulk/offline-region downloads are performed. An internet connection is needed on first load; missing tiles show an empty grid and can be retried.

Choose **Plan distance route (P)** to request Valhalla's automobile distance matrix with `shortest: true`, exhaustively compare all orders of the included undelivered stops (maximum 120 permutations), then request the road geometry for the selected order. The journey starts at the driver marker and ends at the last stop, with no return leg. Initially the driver is at the depot; after completing a delivery, replan from that stop. Distances are directional, so one-way roads matter. This selects the minimum stop order against the service's reported distance matrix, not a guarantee of a globally shortest route under every road-engine approximation. Time is an estimate, not live traffic. Changing stops or recording a delivery invalidates the old route. The driver position is manually simulated, not live vehicle location.

Requests run in a background thread with HTTPS verification, timeouts, a response-size limit, an identifying User-Agent and a five-second UI request cooldown. Existing macOS `/usr/bin/curl` is executed with direct arguments, never a shell; no extra C package is installed. Public demo services have no availability guarantee and are unsuitable for production workloads. Only the fictional landmark coordinates are sent to Valhalla; tile coordinates and ordinary request metadata go to OpenStreetMap. No device location is collected.

Use Up/Down to select a package, Space to include/exclude it, and L/P for map/routing. **Sensor monitoring** opens the original single-parcel simulation, separate from dispatch packages; **Dispatch map (M)** returns. **My profile / Sign out** opens the shared profile. Signing out clears packages, routes, driver position and the sensor session. The map and its pointer controls scale with the window.

Service references: [Valhalla routing API](https://valhalla.github.io/valhalla/api/route/api-reference/), [Valhalla matrix API](https://valhalla.github.io/valhalla/api/matrix/), [public demo service](https://github.com/valhalla/valhalla), [OSM tile usage policy](https://operations.osmfoundation.org/policies/tiles/).

### Original monitoring simulation

- A drawn delivery route and animated parcel marker.
- Parcel ID, delivery status and progress.
- Start/resume, pause and reset controls.
- A 20-second journey, with movement based on elapsed time.
- Explicit simulation mode; no physical hardware is required.
- Live simulated temperature with heat and cooling controls and a 30 C demo alert.
- Impact trigger and cumulative impact count. Reset clears delivery and sensor state.
- Sensor monitoring continues while parcel movement is paused.
- Session condition summary: peak temperature, observed seconds at or above 30 C and total unobserved time while disconnected. Reset clears the summary; cooling and reconnection preserve it. These are monitoring-session metrics, not shipping safety certification.
- Simulated disconnect/reconnect controls, with frozen readings and stale-reading age while disconnected.
- Timestamped sensor history: the latest three events are displayed, with up to 32 retained in memory.
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

Click **Export CSV (E)** or press E. Each export saves the currently retained events (at most 32), oldest first, with `elapsed_seconds`, `event`, `temperature_c` and `impact_count` columns. Impact counts are snapshots taken at each event, not the total at export time. The fourth column is new; consumers of earlier three-column exports should read fields by header name. Existing saved reports are unchanged. Older events that have fallen out of the bounded history are not included. Export does not clear history and works while disconnected or paused.

Files are numbered `events-001.csv` through `events-999.csv` beside the executable: `build/` for `make run`, or `build/Delivery Parcel Tracker.app/Contents/MacOS/` for the Mac app. Right-click the app and choose Show Package Contents to access its exports. The dashboard confirms success or reports a failure; check directory write access or whether all 999 names are occupied before retrying. Existing exports are never overwritten. Reset clears only live history, not saved CSV files.

## Demo

Start the delivery, pause partway along the route, resume to reach the destination, then reset to return to the depot.
Use Trigger heat to raise the reading gradually to 40 C, Stop heat to cool to 20 C, and Trigger impact to record one impact. The heat alert appears at 30 C. Reset returns temperature to 20 C and clears impacts.
Disconnect tracker to freeze telemetry and disable sensor triggers. Reconnect resumes monitoring without clearing impacts or the heat scenario. Delivery movement remains independent of telemetry connection. Reset restores the connection and clears history, leaving a single reset event at time zero.

## Team

See [the team plan](docs/team-plan.md) for file ownership, each teammate's branch, tasks and the pull request workflow. Use `make test` to check delivery state transitions.

GitHub repository: https://github.com/rohen-13/delivery-parcel-tracker

Each group member maintains their own development journal. Store group screencasts in the shared OneDrive folder.

## Automated checks

`make test` checks delivery timing, sensor transitions, a combined delivery and monitoring scenario, and the actual CSV file-saving path. Export checks verify repeated saves, report preservation after reset, trailing-slash directories, invalid destinations and exhaustion of all 999 report names. Test reports are retained in isolated `.scratch/export-test.*` directories, not beside the app's real exports. These checks do not replace manually checking the dashboard button and E shortcut on both Macs.
## Demo login and user profile

The app opens at a local demonstration login screen. Use username **demo** and password **parcel123**. Enter the password and click **Sign in** (or press Enter); Tab switches between the username and password fields. The password is masked, with a Show/Hide control. Incorrect credentials display an error and keep the dashboard closed.

From the dashboard, choose **My profile** to view the demo account and assigned shipment. Edit the display name and click **Save changes**. Names must contain 1–48 printable ASCII characters and cannot contain only spaces. Changes remain in memory for the current session. **Back to dashboard** preserves the session; **Sign out** clears the profile, password input, current delivery and sensor state and returns to login. Previously exported CSV files remain on disk. Monitoring continues while viewing the profile, but does not advance before login.

This demonstrates the navigation and account interface only. The shared demo credentials are embedded in the prototype; there is no server, secure authentication, registration, password recovery or persistent user database. Do not use real account passwords.

The window can be resized; the complete dashboard scales to fit with matching mouse input. Press **F11** to toggle borderless full screen. Press Escape to close the app.


## Temperature history graph

The dashboard plots actual simulated telemetry from the last 60 seconds against a fixed 20–40 C scale. The amber line marks the 30 C demonstration threshold. Regular readings are sampled at most once per second; connection changes are recorded immediately. Disconnects and gaps over two seconds break the plotted line so old or missing readings do not appear as continuous fresh data.

History keeps updating while delivery is paused or the profile is open. Reset all and sign-out clear the graph. Samples stay in memory for this session, with a maximum of 128 points. The event panel displays the latest three sensor events; CSV export still includes up to 32 retained sensor events and does not export graph samples.
