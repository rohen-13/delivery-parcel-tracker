# Delivery Parcel Tracker project roadmap

## Assignment boundaries

Source: Ai Project 2026 Advanced Programming.pdf, pages 1 and 2, supplied by the lecturer.

The assignment allocates 40% overall: 20% for an AI-created software prototype in C and Raylib and a further 20% for a second phase whose requirements are TBA. C++ is excluded. The first phase states three weeks at five hours per week. The brief gives a demo on November 3 at 11; its year is interpreted as 2026 from the assignment context. The final demo and second-phase content remain TBA.

The group has two members and one shared GitHub repository. Each member submits a separate development journal using the lecturer's Gen AI form. One group screencast is due each Tuesday, Wednesday and Friday and is stored in OneDrive. A group presentation/demo is required each Tuesday. The brief does not provide upload links or submission times.

This roadmap is our proposed feature scope, not a lecturer-specified parcel feature list. It prioritises a small complete simulation within the stated workload. Later dates are integration/demo buffer, not an extension of the stated three-week development allocation.

## Product scope

Track one virtual parcel on a fixed fictional route. Simulate location/progress, temperature, impacts and tracker connection status. Display current state and alerts. Save a timestamped event history to CSV if the core features are complete. No actual GPS device, physical sensor or cloud account is required for this prototype.

## Complete work split

| Work package | Lead | Review partner | Completion evidence |
| --- | --- | --- | --- |
| Route, parcel marker and checkpoint indicators | Oleksandr | Tymofii | Clear route and correct marker position |
| Dashboard and Start/Pause/Resume/Reset controls | Oleksandr | Tymofii | Controls work in depot, moving, paused and delivered states |
| Temperature graph and alert presentation | Oleksandr | Tymofii | Plot uses actual simulated values; warnings have readable labels |
| Simulation scenario controls | Oleksandr | Tymofii | Heat, impact and disconnect controls call agreed module functions |
| Delivery timing and status transitions | Tymofii | Oleksandr | Automated state checks and a complete delivery demo |
| Temperature and impact simulation | Tymofii | Oleksandr | Gradual readings, reproducible heat event and one impact per trigger |
| Connection state and stale readings | Tymofii | Oleksandr | Disconnection is visible; stale data is not presented as fresh |
| Alert state and event history | Tymofii | Oleksandr | Raised/resolved/acknowledged behaviour is explained and checked |
| CSV event export | Tymofii | Oleksandr | Export opens and has time, event and value columns |
| Shared interfaces, build and integration | Both | Each other | Both Macs build the same main revision |
| README and demo script | Oleksandr leads | Tymofii | Instructions and claims match the implemented version |
| Individual journals and personal AI prompt logs | Each member | Each checks their own | One honest record per member per required day |
| Group screencasts | Oleksandr records Tuesday/Friday; Tymofii Wednesday | Other member checks | One dated recording per required day, stored in OneDrive |
| Tuesday presentations and November 3 demo | Both | Rehearse together | Each explains their module and answers code questions |

## Proposed delivery stages

| Stage | Target | Oleksandr | Tymofii | Shared checkpoint |
| --- | --- | --- | --- | --- |
| Foundation | October 6 to 9 | Refine the route display and controls | Review tracking state; begin sensor simulation | One parcel completes its journey; journals and recordings |
| Monitoring | October 13 to 16 | Temperature panel, graph and scenario controls | Temperature/impact scenarios and connection handling | Sensor events cause visible dashboard changes |
| Integration | October 20 to 23 | Alerts, history view and layout polish | Event timestamps, export and edge-case checks | Complete demo on both Macs and accurate README |
| Demo buffer | October 27 to November 3 | Fix issues and rehearse UI explanation | Fix issues and rehearse logic explanation | Demo November 3 at 11; second-phase requirements still TBA |

## Recurring evidence

For every Tuesday, Wednesday and Friday: each member updates their own journal; the group records the current product, dates the file and stores it in OneDrive. Every Tuesday both present progress. Do not mark a recording as uploaded, a form as submitted, or a presentation as delivered until it has actually happened.

## Scope control

Keep multiple parcels, real maps, real GPS, authentication, mobile apps, cloud services and predictive AI outside the first prototype. Sensor limits are demo parameters, not claims about safe shipping conditions. AI is used as a development tool; the product does not need an AI model unless the lecturer later specifies one.
