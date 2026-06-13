# Pebble appstore listing — Sykerö TimeStyle

Listing material for the Pebble appstore, kept under version control so it is
ready to upload. **Watchface** listing (lighter asset set than a watchapp).

## Text fields (this directory)

| File | Field |
|---|---|
| `title.txt` | Listing title |
| `description.txt` | Listing description (EN) |
| `source_url.txt` | Source code URL |
| `support_email.txt` | Support email |

Watchfaces need **no category and no icons**.

## Assets (not text, not tracked here)

- **Banner** — optional for a watchface. `project_banner.png` exists in the repo
  root.
- **Screenshots** — ≥1 per supported platform, **unframed**, native resolution.
  Generated on the emulator via the superrepo's
  `scripts/pebble-appstore-screenshots.sh`; output is gitignored under the
  superrepo `appstore/` (regenerable).

## Status

Drafted, awaiting manual upload to the Pebble appstore.
