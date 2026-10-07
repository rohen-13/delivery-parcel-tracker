# Tymofii development plan

Prepared on 6 October 2026 for Tymofii (TymofiiZuren). This is a proposed personal work plan, building on the existing team plan. Oleksandr owns the dashboard; Tymofii owns tracking, sensor simulation, connection state and event history. Shared interface changes need agreement between both members.

## Progress update — 7 October 2026

The original evidence below is a 6 October snapshot, not the current implementation status. The tracking branch now includes temperature/impact simulation, independent monitoring while delivery is paused, connection/stale-reading handling and bounded timestamped sensor history. CSV export is implemented as the next planned tracking feature: Export CSV saves up to 32 retained events without overwriting existing reports. See README for the columns and output location.

CSV verification: `make test`, `make app`, address/undefined-behaviour sensor sanitizer checks and `git diff --check` pass. The dashboard export button produced a CSV whose reset, impact and disconnect rows were inspected. Runtime verification of repeated exports and the E shortcut was interrupted by app-control errors; those checks remain pending. No journal, upload, teammate review or second-Mac validation is claimed by this update.

## Original evidence — 6 October

- Pulled `main` with `git pull --ff-only`, bringing in the C/Raylib prototype and team workflow through `eb2ba03`.
- Switched to the existing remote branch `tymofii/tracking`.
- Reviewed `src/tracker.c`, `src/tracker.h`, `tests/test_tracker.c`, `src/main.c` and the dashboard consumer. Delivery lasts 20 simulated seconds; pause freezes progress; arrival clamps progress to 100%; reset returns to the depot.
- `make test` passed. It checks depot, movement, pause/resume, negative elapsed increments, arrival, restart prevention after arrival, and reset.
- `make` failed because Raylib is unavailable through pkg-config and `raylib.h` cannot be found. No local graphical runtime check or screencast has been completed in this session.
- Temperature, impacts, connection state and event history are not implemented in this revision. The starting implementation belongs to the earlier teammate/AI session, not to Tymofii's independent authorship.

## Assignment requirements

The supplied brief allocates 40% overall: 20% for the C and Raylib prototype, and another 20% whose content and final demo are TBA. Use C, not C++. Work in a two-person group with one GitHub repository. The prototype allocation is three weeks at five hours per week; confirm whether the lecturer means that allocation per person or per group.

Each member must upload a separate journal for work actually done on each Tuesday, Wednesday and Friday. The group must provide one current-product screencast on those days and store the recordings in OneDrive. Both members present each Tuesday. The first demo is 3 November at 11:00, interpreted as 2026 from the current course context. Submission times and the second phase remain unconfirmed.

The project selection page lists IoT Monitoring System; the repository describes a parcel-monitoring prototype under that theme. Confirm that your group has registered that selection. Repository access alone does not prove project registration.

## Your next actions

1. Resolve the documented Raylib prerequisite using the README setup instructions, then run `make`, `make test` and `make run`. Installation is pending. Demonstrate Start, Pause, Resume, delivery and Reset on your Mac.
2. Discuss the sensor interface with Oleksandr. Agree units, a demonstration temperature threshold, one event per impact trigger, and whether monitoring continues while parcel motion is paused. Recommendation: keep monitoring active while motion is paused and reset both modules together. This decision is proposed, not agreed.
3. Write the first failing sensor test: reset establishes a known baseline; a heat scenario crosses the chosen threshold; one impact trigger produces one event. Add `src/sensors.c` and `src/sensors.h` using C and existing project tools. Make the smallest implementation that passes each case.
4. Integrate the module with `src/main.c` and dashboard consumers together. Check normal, heat, impact, paused and reset states visibly. A unit test alone does not establish that dashboard values and controls work.
5. Commit only your intended files on `tymofii/tracking`. Push and open a PR when the change is ready and that external publication is authorised; Oleksandr reviews before merge.

## Three week schedule

Budget five hours per week for this proposed personal schedule, pending clarification of the assignment's allocation. Use approximately two hours Tuesday, one hour Wednesday and two hours Friday, including evidence preparation.

| Week | Tuesday | Wednesday | Friday | Completion evidence |
| --- | --- | --- | --- | --- |
| 6 to 9 October | Pull, review code, run tracking tests, prepare plan and today's journal | Resolve local build; agree sensor API and write first failing sensor cases | Implement baseline, heat and impact simulation | Passing sensor tests and a visible scenario integrated with Oleksandr |
| 13 to 16 October | Present monitoring progress; integrate temperature and impact panels | Add disconnect/reconnect behaviour and stale-reading tests | Verify scenario controls and reset across modules | Disconnection is visible and old readings are labelled stale |
| 20 to 23 October | Present integrated monitoring; add bounded event history | Test event order, timestamps and repeated triggers | Complete integration and regression checks; CSV export only if time permits | Both Macs run the same revision and complete the demo |

27 October and 3 November are proposed rehearsal/demo checkpoints, not an assumed fourth implementation week. Confirm the teaching schedule and whether evidence is still required on 27, 28 and 30 October. Prepare for the 3 November 11:00 demo. Do not invent work for future journals.

## Evidence checklist for each required day

- Write one individual journal with the date, actual contribution, relevant AI prompts, what you accepted or changed, exact checks and results, problems, and next action. Record actual time if required; do not substitute planned hours.
- Review the Word document and upload it through the lecturer's form. The form fields could not be verified in this session, so check them before submission.
- Record one group screencast of the current working product, save it as a dated file in the shared OneDrive folder, and verify it plays. The existing roadmap proposes Tymofii records Wednesday; confirm that split with Oleksandr.
- On Tuesday, both present and explain the code each owns. Record submission, upload and presentation status only after each happens.

## Review limits and pending decisions

Current tracking tests establish the core contract for finite elapsed-time increments. The graphical app has not run locally, and sensor behaviour has no implementation or tests yet. No code bug was confirmed in the reviewed core delivery path. Do not treat the missing local dependency as a tracking bug.

Pending: project registration, exact form fields, OneDrive folder/link, actual hours worked, pause semantics for sensors, and the second-phase brief. The repository's existing CI was pulled from the teammate; this session adds no CI or deployment work.

## Sources

- Lecturer brief supplied in this chat on 6 October 2026.
- Project selection: https://noelohara.github.io/pointersquiz/advancedprog.html (source retrieved; live team registration not checked).
- Journal submission: https://forms.cloud.microsoft/Pages/ResponsePage.aspx?id=OcL4BRfLVUejRQSHUgY8O2ySLwjsnElNlnFmGMhi3aZUMUdYRksyQ0hXNDdEN0U3NDVDWENZTEZGNi4u
- Repository files and tracking test output at `eb2ba03`.
