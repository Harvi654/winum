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
