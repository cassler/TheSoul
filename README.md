# The Soul

An online seed analyzer for Balatro.

## Usage

Visit https://spectralpack.github.io/TheSoul/ to use.

## Command-line search

Build the seed searcher locally to scan for jokers, vouchers, packs, and textual
hits:

```
clang++ -std=c++20 -O2 src/search_cli.cpp -Iinclude -Isrc -o search_cli
```

Example usage (find Negative Blueprint anywhere in the early game):

```
./search_cli --joker "Negative Blueprint" --early 4 --limit 3
```

Need at least a subset of matches? Use `--any N` to require at least `N`
combined `--joker`/`--find` hits without demanding every condition. For
instance,
`./search_cli --joker "Perkeo" --joker "Blue Joker" --find "Negative" --find "Foil" --any 2`
returns seeds where any two of those four criteria succeed.

Matches are reported with location context, e.g.
`Joker Name - Shop - Ante 3, Slot 12` or `Voucher Name - Ante 5`. When no
explicit range is provided, the tool now starts from a randomized seed and wraps
automatically across the full space.

CLI hits are rendered in a compact table showing the name, location, ante, slot,
and pack context for each matching event, making multi-hit seeds easy to skim.
The Details column also previews outcomes for special cards—opening **The Soul**
or playing **Judgement** will list the exact joker that appears, including
rarity and any editions or stickers, so you can decide whether to invest before
using the card.

Results are now presented with a high-contrast headline banner plus optional
ANSI color in terminals that support it. Rare and Legendary finds automatically
populate a dedicated **Highlights** table beneath the main match list so you can
spot premium pickups at a glance, even when they weren't part of the original
search criteria. To adjust styling, pass `--color` (force on) or `--no-color`
(force off); the tool also honors the standard `FORCE_COLOR` and `NO_COLOR`
environment variables.

Performance tip: criteria are cached internally, so repeated searches avoid
per-seed normalization overhead. On a 10‑core Apple M2 Max, scanning 200k seeds
with no matches
(`./search_cli --find __unlikely__ --count 200000 --no-progress`) sustains
roughly 15k seeds per second.
