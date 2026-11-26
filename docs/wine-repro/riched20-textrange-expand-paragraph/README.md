# RichEdit ITextRange Expand units

Base: winum-11.22-3 (Wine 11.22). On Revit 2027.4, the first plan view
fails in TextEngine.dll after a windowless TOM range returns an
unimplemented paragraph expansion. The observed access was a read
from 0x10. This sequence motivates the test. Application testing will
establish the effect of any implementation change.

The probe runs the same synthetic TOM calls on RichEdit20W and
RICHEDIT50W. It also attempts CreateTextServices from each DLL with an
original, windowless ITextHost. The C++ host uses MinGW's native x86
virtual method calling convention. Windowless setup is recorded
separately for each DLL and architecture.

## Cases

The primary story is "Alpha beta. Next!\rGamma delta.\rLast\r". It
supplies punctuation, three explicit paragraph marks and a final story
boundary. The probe records the actual input range coordinates, since
Range may clamp the requested endpoint. Each call uses a fresh range.

- Paragraph, character, word, sentence and line units on inside,
  cross-paragraph, whole-paragraph and degenerate ranges.
- A range whose end is exactly the next paragraph's start, a collapsed
  range at that boundary, the explicit trailing carriage return and
  ranges at the end reported by tomStory.
- For every case, calls with a null delta pointer and with an output
  initialized to 0x12345678. Output has HRESULT, start, end, delta and
  the two endpoint getter results.
- For the windowed controls, line cases at 320 px and 55 px. Use these
  to identify layout-dependent results before deciding the scope.
- For each document, a new story containing "-\r" and the observed
  sequence Range(0,0), Expand(tomStory), SetStart(0), SetEnd(2),
  GetDuplicate and Expand(tomParagraph, NULL).

## Offline Windows 10

Verify SHA256SUMS after extracting the CI artifact. Copy the bundle to
a fresh folder in the offline Windows 10 VM and run run-measure.bat
from a normal console. Return only:

- result-riched-expand-x86_64.txt
- result-riched-expand-i686.txt
- exitcodes.txt

The exit code and measure_complete summarize the windowed controls.
For the windowless cases, check the separate source completion lines
and setup results. Report partial output if a setup fails. No native
HRESULTs, bounds or deltas are assumed by the probe.

## Build

~~~bash
bash scripts/build-measure-bundle.sh out/riched-expand \
    docs/wine-repro/riched20-textrange-expand-paragraph/measure.list
(cd out/riched-expand && sha256sum -c SHA256SUMS)
~~~
