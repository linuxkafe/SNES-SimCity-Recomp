#!/usr/bin/env python3
"""Find the bytes that change slowly inside a running city.

The point: nobody has mapped SimCity's date, population or treasury in WRAM, and
guessing addresses by hand in a 128 KB space is hopeless. But a city that is
running and a clock that is not advancing have a signature - the slow-moving
game state is exactly the set of bytes that change once every N frames rather
than every frame.

So: diff two WRAM dumps taken far enough apart, group the changes into runs, and
report each run with how many bytes it covers and whether it looks like a
counter. Anything that changes in one place every few dozen frames is a
candidate for a month tick; anything that changes every single frame is
animation, not state.

Usage:
    wram-diff.py <wram-A.bin> <wram-B.bin> [--frames N]
"""
import argparse
import sys


def runs(diffs):
    """Group changed byte indices into contiguous runs."""
    out = []
    start = prev = None
    for i in diffs:
        if prev is None or i == prev + 1:
            start = i if start is None else start
            prev = i
            continue
        out.append((start, prev))
        start = prev = i
    if start is not None and prev is not None:
        out.append((start, prev))
    return out


def u16(data, off):
    return data[off] | (data[off + 1] << 8)


def u32(data, off):
    return int.from_bytes(data[off:off + 4], "little")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--frames", type=int, default=0,
                    help="frames between the two dumps, for rate")
    ap.add_argument("--max", type=int, default=60, help="max runs to print")
    ap.add_argument("--city", action="store_true",
                    help="judge the result as a running city rather than a "
                         "listing of candidates")
    args = ap.parse_args()

    a = open(args.a, "rb").read()
    b = open(args.b, "rb").read()
    if len(a) != len(b):
        print("dumps differ in size: %d vs %d" % (len(a), len(b)))
        return 2

    total = len(a)
    diffs = [i for i in range(total) if a[i] != b[i]]
    print("WRAM: %d bytes, %d changed (%.2f%%)" %
          (total, len(diffs), 100.0 * len(diffs) / total))

    rr = runs(diffs)
    print("%d contiguous runs of change\n" % len(rr))
    shown = 0
    for lo, hi in rr:
        if shown >= args.max:
            print("... %d more runs" % (len(rr) - shown))
            break
        n = hi - lo + 1
        va = a[lo:hi + 1]
        vb = b[lo:hi + 1]
        detail = ""
        if n <= 4 and lo + n <= total:
            if n == 1:
                detail = "%3d -> %3d" % (va[0], vb[0])
            elif n == 2:
                detail = "%5d -> %5d" % (u16(a, lo), u16(b, lo))
            elif n == 4:
                detail = "%10d -> %10d" % (u32(a, lo), u32(b, lo))
        elif n <= 8 and lo + n <= total:
            # Adjacent counters merge into one run, so a 4-byte and a 2-byte
            # value sitting next to each other arrive as a 6-byte run and the
            # per-width decode above never fires. Print the wide readings too:
            # a jump of exactly 1 in the low word of a 4-byte field is a
            # counter, and that is the shape the month tick will have.
            detail = "u32 %d -> %d | u16 %d -> %d" % (
                u32(a, lo), u32(b, lo), u16(a, lo), u16(b, lo))
        rate = ""
        if args.frames > 0 and n <= 4:
            if n == 1:
                d = vb[0] - va[0]
                rate = "delta %+d over %d frames" % (d, args.frames)
            elif n == 2:
                d = u16(b, lo) - u16(a, lo)
                rate = "delta %+d over %d frames" % (d, args.frames)
            elif n == 4:
                d = u32(b, lo) - u32(a, lo)
                rate = "delta %+d over %d frames" % (d, args.frames)
        print("  $%04X-$%04X  %2d bytes  %-28s %s" %
              (lo, hi, n, detail, rate))
        shown += 1
    if args.city:
        # A verdict, because the listing alone is easy to misread. 27 bytes
        # over 3600 frames is not a running city: a live one moves population,
        # treasury, agents and tiles, and would change thousands. What changes
        # that little is a still attract screen - and the trap is that the
        # probe is perfectly happy either way, printing the same confident
        # table for a state that never contained a city.
        print()
        if len(diffs) < 1000:
            print("VERDICT: this does not look like a running city.")
            print("  %d bytes moved in %d frames. A live city moves thousands."
                  % (len(diffs), args.frames or 0))
            print("  Look at the screenshot before reading anything into the")
            print("  table above: this is most likely a state taken before the")
            print("  city was running, and the probe is measuring the still")
            print("  screen behind it.")
        else:
            print("VERDICT: enough moved to be a running city. The slow runs")
            print("  above are the candidates; narrow the window to separate a")
            print("  per-second counter from the month tick.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
