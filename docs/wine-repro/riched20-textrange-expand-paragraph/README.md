# RichEdit TOM paragraph navigation

The first plan view in Revit 2027.4 reaches a windowless TOM range. With
the combined default-tab-stop and Expand overlay, Expand(tomParagraph)
returns successfully. The next unsupported operation in the trace is
MoveEnd(tomParagraph). The read at TextEngine.dll+0x2ac70 still occurs.
This is the third application attempt. A new combined overlay will be
checked after the six paragraph operations have been measured and
tested. If the same crash persists, record the trace and defer Revit
work on this problem.

## Current probe

The probe runs on RichEdit20W, RICHEDIT50W, and CreateTextServices
from both DLLs using the same original windowless host as the first
probe. Each source runs the following cases:

- Expand, StartOf, EndOf, Move, MoveStart and MoveEnd with tomParagraph.
- A short story "-\r", one paragraph without a trailing carriage return,
  two paragraphs with and without a trailing carriage return, and the
  original "Alpha beta. Next!\rGamma delta.\rLast\r" story.
- Carets at the start, within paragraphs, at a paragraph boundary and
  at the story end. Ranges include complete paragraphs, ranges ending
  at the next boundary, ranges spanning paragraphs and the final marks.
- StartOf and EndOf with tomMove and tomExtend. The other three
  movement methods use counts -32, -2, -1, 0, 1, 2 and 32.
- Fresh ranges for each call, with null and sentinel-initialized delta.
  Each row records the actual input coordinates, HRESULT, output
  coordinates, delta and output getter HRESULTs.
- A separate sequence measures MoveEnd(+1) on the same duplicate after
  Expand(tomParagraph, NULL) on the short story.

This round measures tomParagraph only. The previous Expand unit matrix
is retained in the [first probe at d7132a0](https://github.com/Harvi654/winum/tree/d7132a0/docs/wine-repro/riched20-textrange-expand-paragraph)
and the Windows results below.

## Offline Windows 10

Verify SHA256SUMS after extracting the CI artifact. In a fresh directory
in the offline VM, run `run-measure.bat` and return these three files:

- result-riched-paragraph-navigation-x86_64.txt
- result-riched-paragraph-navigation-i686.txt
- exitcodes.txt

Both architecture runs should report all four source completion lines
and end with `measure_complete=1`. A failed setup or getter is recorded
in the output. Partial results are still useful.

## Build

~~~bash
bash scripts/build-measure-bundle.sh out/riched-paragraph-navigation \
    docs/wine-repro/riched20-textrange-expand-paragraph/measure.list
(cd out/riched-paragraph-navigation && sha256sum -c SHA256SUMS)
~~~

## First probe: Windows 10 Expand results (2026-11-26)

Both offline executables exited 0. Both window classes and both
CreateTextServices hosts completed. The x86_64 and i686 results agree
on every recorded call. [Normalized results](results/windows-10/ranges.tsv)
use `all` where the four sources agree for units 1–4 and list the
individual source and fixture for line expansion.

| Call | Native result |
| --- | --- |
| Paragraph, range (2,5) | (0,18), delta 15, S_OK |
| Paragraph, range (5,23) | (0,31), delta 13, S_OK |
| Paragraph, range (6,18) ending at the next paragraph | (0,18), delta 6, S_OK |
| Paragraph, collapsed (18,18) at the same boundary | (18,31), delta 13, S_OK |
| Paragraph, collapsed (36,36) at the implicit final mark | (36,37), delta 1, S_OK |
| Paragraph, already complete (0,18) or (36,37) | Unchanged, delta 0, S_FALSE |
| Paragraph, duplicate (0,2) of the short story "-\r", null delta | Unchanged, S_FALSE |
| Line, range (2,5) on the long story | (0,18) in the windowed controls, (0,37) in the windowless hosts |

Passing a null delta pointer gave the same paragraph bounds and return
code. Wine 11.22 returns E_NOTIMPL for paragraph expansion. The
candidate implements only paragraph expansion. Character, word, and
sentence results are retained as reference measurements. The measured
line result depends on the host and is outside this candidate.

## Windows 10 paragraph navigation results (2026-11-26)

Both offline runs exited 0. Each has 10,408 observations, 2,602 per
host. The x86_64 and i686 output agrees after the bitness header. Both
RichEdit classes and both CreateTextServices hosts agree for each case.

| Case | Result |
| --- | --- |
| Expand the short duplicate (0,2) in "-\\r" | S_FALSE, unchanged |
| MoveEnd(+1) immediately after that Expand | S_OK, (0,3) |
| MoveStart(+32) from (0,0) in "Alpha beta. Next!\\rGamma delta." | S_OK, (30,30), delta 2 |
| MoveEnd(+32) from (0,0) in the same story | S_OK, (0,31), delta 2 |
| StartOf(tomMove) from (1,5) in the same story | S_OK, (0,0), delta -2 |
| EndOf(tomMove) from (1,20) in the same story | S_OK, (30,30), delta 11 |
| EndOf(tomExtend) from (1,20) in the same story | S_OK, (1,31), delta 11 |
| Move(+1) from (30,31) in the same story | S_OK, (30,30), delta -1 |

The focused Wine test records representative native cases alongside
the original Expand cases, including the short call sequence.
