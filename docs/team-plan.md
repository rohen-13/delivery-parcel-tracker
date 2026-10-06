# Working together

## Your branches

| Team member | Branch | Owned files | Responsibility |
| --- | --- | --- | --- |
| Oleksandr (rohen-13) | `rohen/dashboard` | `src/dashboard.c`, `src/dashboard.h` | Route display, parcel marker, status panels, buttons, graphs and alert presentation |
| Tymofii (TymofiiZuren) | `tymofii/tracking` | `src/tracker.c`, `tests/test_tracker.c`; future `src/sensors.c` and `src/sensors.h` | Delivery timing and state, temperature/impact simulation, connection status and event history |
| Both, by agreement | `main` | `src/tracker.h`, `src/main.c`, `Makefile`, `README.md` | Shared interface, app startup, build instructions and integration |

The starting prototype is already provided. Read and understand the relevant files before changing them. Do not claim the generated starting code as work you wrote independently; record AI assistance in your individual journals.

## Today's individual tasks

### Oleksandr: dashboard branch

1. Build and run the starting prototype.
2. Read `DashboardDraw` and the button helper; explain the drawing loop in your journal.
3. Make your own visible improvement: add an on-screen route legend or checkpoint arrival indicators based on delivery progress.
4. Add room for a temperature panel and an alert panel. Label placeholder readings as unavailable until real simulated values are connected.
5. Check that the labels remain readable and the controls still work.
6. Commit your work and open a pull request for Tymofii to review.

### Tymofii: tracking branch

1. Accept the repository invitation, clone the repository, build and run it.
2. Read the tracking state transitions and run `make test`.
3. Add a small sensor simulation module: a temperature that changes gradually, a heat scenario, and an impact event.
4. Agree the new function declarations and data fields with Oleksandr before changing the shared header. Suggested functions: `SensorsReset`, `SensorsUpdate`, `SensorsTriggerHeat`, `SensorsTriggerImpact`.
5. Verify that a heat event crosses a chosen demo threshold and that an impact records one event per trigger. Decide together whether readings continue while delivery is paused.
6. Commit your work and open a pull request for Oleksandr to review.

Sensor monitoring is the next development step; it is not part of the current working prototype.

## Get your own copy on a Mac

```sh
git clone https://github.com/rohen-13/delivery-parcel-tracker.git
cd delivery-parcel-tracker
brew install raylib pkgconf
```

Oleksandr switches to:

```sh
git switch rohen/dashboard
```

Tymofii switches to:

```sh
git switch tymofii/tracking
```

Both can build and run with:

```sh
make run
```

## Daily branch workflow

Start work on your own branch. Pull its latest changes:

```sh
git pull --ff-only
```

After making and checking your changes, stage only your intended files, commit, then push. Examples:

```sh
# Oleksandr
git add src/dashboard.c src/dashboard.h
git commit -m "Add dashboard route legend"
git push
```

```sh
# Tymofii, after creating these sensor files
git add src/tracker.c src/sensors.c src/sensors.h
git commit -m "Add temperature and impact simulation"
git push
```

On GitHub, open a pull request with `main` as the base and your own branch as the comparison. Explain your change and how you checked it. Your teammate reviews it before you merge it.

After a teammate's pull request merges, save or commit your own work, then update your branch:

```sh
git fetch origin
git merge origin/main
make
make test
```

Resolve any conflicts together. Do not force-push or overwrite each other's changes. Keep these two named branches for ongoing work.

## Group deliverables

Combine a working version on `main`, record one group screencast, and maintain separate individual journals showing each person's actual work, AI use, verification and next steps.
