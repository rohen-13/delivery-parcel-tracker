# Delivery Parcel Tracker

A C and Raylib prototype that combines a five-package delivery round, OpenStreetMap road routing and independent simulated sensor monitoring for each package.

## Current features

### Delivery run mode

Plan a route, choose **1x**, **5x** or **20x demo**, then use **Start / resume drive (D)**. The simulated driver follows the returned road polyline to the next stop; the panel shows leg progress, remaining road distance and an estimated road travel time. Press D or **Pause drive** to pause. Sensors continue updating independently. Driving pauses automatically on the account/profile/lock pages and before a network request. Unlocking does not automatically restart driving.

Arrival stops the vehicle and enables the selected next parcel's delivery actions. **Select arrived parcel** focuses that parcel if another is selected. Complete it or record a failed attempt; either outcome consumes that road leg and prepares the next leg of the same route, without another network request. The driver never skips an unresolved stop, even after a large frame delay. Each leg needs an explicit Start action. Changing included parcels, retrying a deferred parcel, resetting or resuming a checkpoint invalidates the route and requires replanning.

Movement is an illustrative constant-average-speed traversal of each service-estimated leg, not GPS, turn-by-turn guidance or live traffic. The demo multiplier accelerates driving only; sensor event timestamps remain normal elapsed monitoring seconds. Checkpoints preserve the current driver position and parcel outcomes, not playback state or cached geometry; resume starts with driving stopped and requires a fresh route.

The interface uses installed Trebuchet MS regular and bold TrueType faces, a 14-pixel minimum virtual text size, stronger heading hierarchy and a restrained teal accent. Installed Arial faces and Raylib's default font are fallbacks; no proprietary font files are copied or bundled. At smaller windows the virtual layout scales down, so use a larger window for a classroom demonstration.

### Save and resume a round

In Dispatch, **Save (S)** writes `parcel-round-v1.save` beside the executable (inside `Contents/MacOS` when using the app bundle). Saving replaces the previous checkpoint only after writing and validating a temporary file successfully. Folder or write failures leave the earlier checkpoint intact. This is a single manual checkpoint, not autosave or a backup history.

After restarting, sign in normally and choose **Resume (O)**. Confirm **Resume saved round** to replace current progress, or **Keep current round** to cancel. Resume restores all five parcel states, deferred deliveries, retained attempts, sensor events, graph samples, selected parcel and simulated driver position. Simulation time does not advance while closed. The old road route is deliberately not stored: plan a fresh route after resuming. Save and Resume are unavailable during background map or route requests.

The versioned text format contains no account credentials, operator profile, map tiles or route geometry. Invalid versions, truncated data, out-of-range numbers, excessive record counts and trailing data are rejected without changing the current round. This is local, unencrypted demo data, not a secure user database or a tamper-proof record. Seeded package identities and destinations come from the app; changing those seeds requires a file-format version change. Reset, sign-out and closing do not delete the checkpoint, but unsaved changes are lost. To replace an old checkpoint, explicitly save the desired round.

### Failed deliveries and retries

After driving to the next stop, select that parcel and choose **Report failed stop**. The inline panel records one of three reasons: recipient absent, no access or address problem. Cancel leaves the round unchanged. Only the arrived next stop can be attempted. Recording a failure marks the parcel **deferred** and advances the existing road route to its next leg. Deferred parcels remain monitored and appear in the parcel-check queue; their checkbox cannot bypass the retry workflow.

Select the deferred parcel in Dispatch and choose **Retry selected parcel** to include it again, then plan a fresh route. Attempt history is retained. Completing a successful delivery also records an attempt and freezes the parcel's sensor history as before. These are manual demonstration actions, not proof of physical delivery or GPS arrival.

Monitoring shows the attempt count and latest outcome with elapsed monitoring seconds. **Export attempts (A)** saves that parcel's last eight attempts in chronological order, with original attempt numbers, elapsed seconds and reason. The total attempt count remains available even when older retained records roll off. Numbered filenames avoid overwriting earlier exports. The round report appends `attempt_count` and `last_outcome`, adds `deferred` as a delivery status and flags deferred parcels; consumers should use CSV header names. Attempt data lives in memory and is cleared by reset or sign-out; exported files remain on disk.

### Linked dispatch and monitoring views

**Export round report (R)** on Dispatch saves a snapshot of all five parcels as `round-summary-001.csv` beside the app executable (inside `Contents/MacOS` for the macOS app bundle). The report includes destination, delivered/pending/excluded status, planned stop when available, live/disconnected/archived sensor state, current and peak temperature, impacts, heat exposure, unobserved seconds and active flags. Disconnected temperatures are last-known readings, not fresh measurements. Delivered readings are frozen at delivery. Repeated exports use the next available number without overwriting earlier reports; reset and sign-out do not delete saved files. The save message names the file; a failed write is reported instead. This is a snapshot, not persistent application storage.

**Next parcel to check (N)** cycles through undelivered parcels with a heat warning, disconnected sensor, recorded impact or deferred delivery. From Dispatch it opens Monitoring for that parcel; within Monitoring it switches parcels. The counter counts parcels, not individual warnings. Delivered parcels retain their history but no longer count as active flags. Impact history remains flagged for the rest of the round; this is not an acknowledgement workflow. With no active flags the button is disabled.

Sign in using the local demo account `demo` / `parcel123`. Dispatch has the full-size road map, package selection and route controls. Select a package row or map marker, then choose **Monitor selected parcel** to open its separate Monitoring page. **View on dispatch map** returns with the same package selected. Monitoring has temperature, impacts, connection controls, history and package events; both views share the same per-package data. Dispatch rows and markers flag heat, impact history and disconnected sensors.

Each of the five fictional Waterford packages has its own readings, 60-second graph and up to 32 events. Heat, impact and disconnect controls affect only the selected package. Undelivered packages continue monitoring while another package is selected, while the profile is open and while the session is locked. Completing a delivery freezes that package's telemetry for later inspection and export.

The views are linked through package identity, not a combined map-and-sensor layout. Monitoring displays the selected parcel's delivery state, planned stop number and driver status when it is the next stop. Package delivery is recorded in Dispatch with **Complete selected stop**, enabled only after driving to the selected next stop. The teal driver marker follows the road; completing a stop prepares the next leg. **Reset delivery round** restores all packages, telemetry and the driver to the depot.

### OpenStreetMap and road routing

Choose **Load map (L)** to fetch the 12 tiles covering the visible area. Tiles are cached beside the executable in `map-cache/` for seven days. Attribution links to OpenStreetMap's copyright information. First load requires internet; missing tiles can be retried. No bulk region downloads are performed.

Choose **Plan driving route (P)** to request Valhalla's directional automobile distance matrix using `shortest: true`, compare all selected stop orders (at most 120), and request the road geometry for the chosen order. The route starts at the simulated driver and ends at the last stop, with no return leg. This minimises the stop order against the service's distance matrix; it is not a guarantee of a globally shortest path under every road-engine approximation. Journey time is an estimate without live traffic. Changing included packages or retrying a deferred parcel invalidates the route; recording an outcome at an arrived stop advances the remaining legs.

Requests run in a background thread using HTTPS, timeouts, bounded responses, a User-Agent and a five-second request cooldown. Existing macOS `/usr/bin/curl` is executed directly; there are no additional C dependencies. The public services have no availability guarantee. Only fictional landmark coordinates are submitted; no device location is collected.

Use Up/Down to select packages, Space to include/exclude a selected package, and L/P for map and routing. **Profile** and **Lock** are available in the workspace header. No live GPS, address search, custom stop entry or physical sensor hardware is implemented.

Service references: [Valhalla route API](https://valhalla.github.io/valhalla/api/route/api-reference/), [Valhalla matrix API](https://valhalla.github.io/valhalla/api/matrix/), [OSM tile policy](https://operations.osmfoundation.org/policies/tiles/).

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

Physical hardware integration is not implemented. Temperature limits are fictional demonstration values. Event times are elapsed monitoring seconds, not wall-clock timestamps; retained history can be restored from a manual round checkpoint or exported as a report.

## Event export

Choose **Export package events (E)** or press E to export the currently selected package's retained events, including a delivered package's archived history. Columns are `package_id,elapsed_seconds,event,temperature_c,impact_count`. Each row identifies the package; impact counts are snapshots at event time. Times are simulation seconds rather than wall-clock timestamps.

Files are numbered separately for each package, for example `PKG-003-events-001.csv` through `PKG-003-events-999.csv`, beside the executable. Existing files are never overwritten. Export does not clear history; reset and sign-out do not delete saved reports. Graph samples are not included. Earlier single-parcel exports retain their original format.

## Demo

Sign in, load the map and plan a route. Choose 20x demo and start driving. Pause and resume to show the moving driver and leg progress. On arrival, complete the parcel; start the next leg and record a failure at its arrival. Continue the remaining legs without replanning. In Monitoring, trigger heat or an impact and inspect the independent histories. Save the round, reset it and confirm Resume to restore progress. A resumed round needs a fresh route. Lock/unlock preserves the round but leaves driving paused; sign-out clears memory, not the saved checkpoint.

## Team

See [the team plan](docs/team-plan.md) for file ownership, each teammate's branch, tasks and the pull request workflow. Use `make test` to check delivery state transitions.

GitHub repository: https://github.com/rohen-13/delivery-parcel-tracker

Each group member maintains their own development journal. Store group screencasts in the shared OneDrive folder.

## Automated checks

`make test` checks delivery timing, sensor transitions, a combined delivery and monitoring scenario, and the actual CSV file-saving path. Export checks verify repeated saves, report preservation after reset, trailing-slash directories, invalid destinations and exhaustion of all 999 report names. Test reports are retained in isolated `.scratch/export-test.*` directories, not beside the app's real exports. These checks do not replace manually checking the dashboard button and E shortcut on both Macs.
## Demo login and user profile

The app opens with a two-step operator sign-in. Confirm username **demo** and choose **Continue to password** (or press Enter). Enter **parcel123** and choose **Open workspace** (or press Enter). **Change** returns to the username step and clears the password. The password is masked, with a Show/Hide control. Incorrect details display an inline error, clear the attempted password and keep the workspace closed.

Choose **Profile** in the workspace header. Edit the display name and click **Save changes**. Names must contain 1–48 printable ASCII characters and cannot contain only spaces. **Back to workspace** preserves the session. **Sign out** clears the profile, password input, packages, routes and sensor state and returns to the account step. Previously exported CSV files remain on disk. Monitoring continues while viewing the profile, but does not advance before login.

Choose **Lock** in the workspace header or **Lock workspace** in the profile. Locking hides the workspace while preserving the delivery round, selected package and operator name. Undelivered packages continue sensor monitoring in the background. Enter the same demo password to unlock; incorrect attempts keep the session locked and do not reset the round. **Sign out instead** on the lock screen ends and clears the session. This demonstrates session navigation, not production access protection: the shared credentials remain public and local.

This demonstrates the navigation and account interface only. The shared demo credentials are embedded in the prototype; there is no server, secure authentication, registration, password recovery or persistent user database. Round checkpoints do not restore a signed-in session. Do not use real account passwords.

The window can be resized; the complete dashboard scales to fit with matching mouse input. Press **F11** to toggle borderless full screen. Press Escape to close the app.


## Temperature history graph

The dashboard plots actual simulated telemetry from the last 60 seconds against a fixed 20–40 C scale. The amber line marks the 30 C demonstration threshold. Regular readings are sampled at most once per second; connection changes are recorded immediately. Disconnects and gaps over two seconds break the plotted line so old or missing readings do not appear as continuous fresh data.

Each package retains its own history. Undelivered packages update while the profile or lock page is open; delivery freezes that package's history. Reset delivery round and sign-out clear all histories. Each graph retains up to 128 samples; the event panel paginates up to 32 events per package, five per page.


## Package event filters

On Package monitoring, the event panel now lists the newest matching events first. Choose All, Heat (scenario changes and alerts), Impact, or Link (disconnect/reconnect). Newer and Older navigate five events per page through the selected parcel's retained history. Switching parcel or filter returns to the newest page. Empty categories show No matching events; unavailable navigation is disabled. At most 32 events remain available, so events already evicted from memory cannot be recovered here. Filters affect only this view: package CSV export still includes the full retained history. Archived parcels can be filtered too.

## Delivery attempt history

On Package monitoring, choose **Delivery attempts** in the history panel to inspect the selected parcel's latest eight retained delivery attempts, newest first. Each entry shows its original attempt number, elapsed monitoring seconds and outcome (delivered, recipient absent, no access or address problem). The total attempt count remains visible when older records roll off; a parcel with no attempts shows an empty-state message. Switch parcels to inspect their independent histories, including delivered archives. Choose **Sensor events** to return to the existing event filters and pagination. **Export attempts (A)** still exports retained attempts chronologically. Reset and sign-out clear the in-memory history; manual checkpoints restore it.
