# RichEdit ITextDocument default tab stop

Base: winum-11.22-3 (Wine 11.22). Revit 2027.4 (Czech) now reaches Home
with the ole32 overlay. When a new project's first plan view is built,
the journal reports failures setting the default tab size before an
access violation. The trace shows stubs for the default tab getters and
setters and for language and typography options. The trace alone leaves
the cause of the plan-view crash open.

## Native Windows 10 reference

The program loads the system RichEdit DLLs, creates a RichEdit20W control
from riched20.dll and a RICHEDIT50W control from msftedit.dll, and obtains
ITextDocument through EM_GETOLEINTERFACE. All text and controls are
synthetic.

| Check | Recorded data |
| --- | --- |
| New control | GetDefaultTabStop HRESULT and value in points, plus GetDefaultTabStop with a null output pointer |
| SetDefaultTabStop | HRESULT and read-back for 36, 18.5, 0, -12 and 100,000,000 points, in that order on each control |
| Layout | EM_POSFROMCHAR for the three letters in A-tab-B-tab-C, before setters, immediately after each setter, and after reinserting the text. Compare each tabbed character's x position with the first character's x position. DPI and formatting bounds are recorded. |
| Paragraph | EM_GETPARAFORMAT return value, mask, start indent, cTabCount and all rgxTabs slots, before and after the same changes |
| Language | Initial EM_GETLANGOPTIONS on a separate fresh control, EM_SETLANGOPTIONS to zero and IMF_AUTOFONT, then restoration, with return values and read-backs |
| Typography | EM_GETTYPOGRAPHYOPTIONS before and after EM_SETTYPOGRAPHYOPTIONS with TO_ADVANCEDTYPOGRAPHY as both value and mask, including setter return |

The output includes the getter's sentinel if a failing getter leaves the
value unwritten. Paragraph and position rows printed after
event=reinserted_text reflect the reinserted text.

Verify SHA256SUMS in the extracted two-architecture bundle. In a fresh
directory in the offline Windows 10 VM, run run-measure.bat from the
normal desktop. Return only these files:

- result-riched-tab-stop-x86_64.txt
- result-riched-tab-stop-i686.txt
- exitcodes.txt

Both report files should end with measure_complete=1 and the exit codes
should be zero. If setup fails, the report identifies the class and step.

## Build

~~~bash
bash scripts/build-measure-bundle.sh out/riched-tab-stop \
    docs/wine-repro/riched20-default-tab-stop/measure.list
(cd out/riched-tab-stop && sha256sum -c SHA256SUMS)
~~~

## Round 1 result (2026-11-26)

Both Windows 10 architectures completed all observations with exit 0.
The [normalized observation](results/windows-10/round-1.txt) is shared.

| Observation | RichEdit20W | RICHEDIT50W |
| --- | --- | --- |
| New default, points | 36 | 36 |
| Null getter output | E_INVALIDARG | E_INVALIDARG |
| Set 36 and 18.5 | S_OK, exact read-back | S_OK, exact read-back |
| Set zero and -12 | E_INVALIDARG | E_INVALIDARG |
| Set 100,000,000 | S_OK, exact read-back | E_INVALIDARG |
| Initial language flags | 0x82 | 0x182 |

At 96 DPI, the first and second tabbed characters moved from 48 and 96
pixels relative to the first character to 25 and 50 immediately after
setting 18.5 points. Reinserting text through WM_SETTEXT restored the
original spacing. The report did not query the value after reinsertion.
Explicit paragraph tab count and start indent stayed zero.
Both classes returned 1 for the measured language and typography setters
and returned the flags on read-back.

The follow-up probes the fractional storage and accepted range on empty
controls, and queries the value after EM_REPLACESEL and WM_SETTEXT.

## Round 2 bundle

Run the new bundle in another fresh directory of the same offline VM.
Reports for this run include round=2. Return the same three output file
names. The follow-up records first the edits and then the range probes
on an empty document. When 36 succeeds and FLT_MAX is rejected with
E_INVALIDARG, it reports neighboring floats across an observed
acceptance transition. The endpoint result alone does not establish
monotonicity for all inputs.
