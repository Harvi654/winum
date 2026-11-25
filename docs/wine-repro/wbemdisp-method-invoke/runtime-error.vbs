' Original MEASURE script, AI-assisted (Codex), LGPL-2.1-or-later.
Option Explicit
WScript.Echo "vbs intentional_error_begin"
Err.Raise vbObjectError + 310, "winum-measure", "intentional runtime error"
WScript.Echo "vbs intentional_error_unreachable"
