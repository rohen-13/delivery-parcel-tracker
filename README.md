# Delivery Parcel Tracker

A C and Raylib prototype that combines a five-package delivery round, OpenStreetMap road routing and independent simulated sensor monitoring for each package.

## Current features

### Linked dispatch and monitoring views

**Export round report (R)** on Dispatch saves a snapshot of all five parcels as `round-summary-001.csv` beside the app executable (inside `Contents/MacOS` for the macOS app bundle). The report includes destination, delivered/pending/excluded status, planned stop when available, live/disconnected/archived sensor state, current and peak temperature, impacts, heat exposure, unobserved seconds and active flags. Disconnected temperatures are last-known readings, not fresh measurements. Delivered readings are frozen at delivery. Repeated exports use the next available number without overwriting earlier reports; reset and sign-out do not delete saved files. The save message names the file; a failed write is reported instead. This is a snapshot, not persistent application storage.

**Next parcel to check (N)** cycles through undelivered parcels with a heat warning, disconnected sensor or recorded impact. From Dispatch it opens Monitoring for that parcel; within Monitoring it switches parcels. The counter counts parcels, not individual warnings. Delivered parcels retain their history but no longer count as active flags. Impact history remains flagged for the rest of the round; this is not an acknowledgement workflow. With no active flags the button is disabled.

Sign in using the local demo account `demo` / `parcel123`. Dispatch has the full-size road map, package selection and route controls. Select a package row or map marker, then choose **Monitor selected parcel** to open its separate Monitoring page. **View on dispatch map** returns with the same package selected. Monitoring has temperature, impacts, connection controls, history and package events; both views share the same per-package data. Dispatch rows and markers flag heat, impact history and disconnected sensors.

Each of the five fictional Waterford packages has its own readings, 60-second graph and up to 32 events. Heat, impact and disconnect controls affect only the selected package. Undelivered packages continue monitoring while another package is selected, while the profile is open and while the session is locked. Completing a delivery freezes that package's telemetry for later inspection and export.

The views are linked through package identity, not a combined map-and-sensor layout. Monitoring displays the selected parcel's delivery state and planned stop number when a route is available. Package delivery is recorded in Dispatch with **Complete selected stop**, enabled only for the planned next stop. The lime driver marker moves to that delivery; the old route is cleared and remaining stops can be replanned from there. **Reset delivery round** restores all packages, telemetry and the driver to the depot.

### OpenStreetMap and road routing

Choose **Load map (L)** to fetch the 12 tiles covering the visible area. Tiles are cached beside the executable in `map-cache/` for seven days. Attribution links to OpenStreetMap's copyright information. First load requires internet; missing tiles can be retried. No bulk region downloads are performed.

Choose **Plan driving route (P)** to request Valhalla's directional automobile distance matrix using `shortest: true`, compare all selected stop orders (at most 120), and request the road geometry for the chosen order. The route starts at the simulated driver and ends at the last stop, with no return leg. This minimises the stop order against the service's distance matrix; it is not a guarantee of a globally shortest path under every road-engine approximation. Journey time is an estimate without live traffic. Changing included packages or completing a stop invalidates the route.

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

Physical hardware integration is not implemented. Temperature limits are fictional demonstration values. Event times are elapsed simulation seconds, not wall-clock timestamps; history is not saved between runs unless exported.

## Event export

Choose **Export package events (E)** or press E to export the currently selected package's retained events, including a delivered package's archived history. Columns are `package_id,elapsed_seconds,event,temperature_c,impact_count`. Each row identifies the package; impact counts are snapshots at event time. Times are simulation seconds rather than wall-clock timestamps.

Files are numbered separately for each package, for example `PKG-003-events-001.csv` through `PKG-003-events-999.csv`, beside the executable. Existing files are never overwritten. Export does not clear history; reset and sign-out do not delete saved reports. Graph samples are not included. Earlier single-parcel exports retain their original format.

## Demo

Sign in, load the map and plan a route. Select a package and trigger heat or an impact. Select another package to see its independent readings, then return to the first package and check its graph and events. Disconnect a package to see a stale-reading flag in the list, map and inspector. Complete the planned next stop, inspect its frozen history and replan the remaining deliveries. Lock/unlock to retain the round; sign out to clear it.

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

This demonstrates the navigation and account interface only. The shared demo credentials are embedded in the prototype; there is no server, secure authentication, registration, password recovery or persistent user database. Do not use real account passwords.

The window can be resized; the complete dashboard scales to fit with matching mouse input. Press **F11** to toggle borderless full screen. Press Escape to close the app.


## Temperature history graph

The dashboard plots actual simulated telemetry from the last 60 seconds against a fixed 20–40 C scale. The amber line marks the 30 C demonstration threshold. Regular readings are sampled at most once per second; connection changes are recorded immediately. Disconnects and gaps over two seconds break the plotted line so old or missing readings do not appear as continuous fresh data.

Each package retains its own history. Undelivered packages update while the profile or lock page is open; delivery freezes that package's history. Reset delivery round and sign-out clear all histories. Each graph retains up to 128 samples; the event panel shows the latest three of up to 32 events per package.
