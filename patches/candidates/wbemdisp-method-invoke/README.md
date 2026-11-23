# Direct WMI method calls in wbemdisp

The [offline Windows reference and L1](../../../docs/wine-repro/wbemdisp-method-invoke/README.md)
show that both method flag combinations dispatch the method, pass
positional inputs in signature order, copy outputs back by reference,
and return the provider status as a separate value.

The patch routes only dynamic method members to the WMI method
implementation. It preserves the type-library and ordinary property
paths. The dispatcher uses signature IDs when the provider supplies them
and signature enumeration order otherwise. Named arguments are outside
the measured scope.

Apply the candidate after the Winum 11.22-2 release series with
zero fuzz. The release series does not select it.

| Level | Status |
| --- | --- |
| L0 | Candidate builds pending. |
| L1 | Offline Windows 10. 135 checks per architecture, zero failures, todos and skips. |
| L2 | Paired wbemdisp, wbemprox and vbscript suites pending. |
| L3 | Paired C and VBScript measurement against the offline Windows reference pending. |
| L4 | Pending maintainer run of the Nastran 2027 installer and mesh/run/results in a separate copy of the Inventor 2027.2 prefix and runtime. |
| Cross review | Pending Claude review. |
