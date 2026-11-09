# Batch 1 L1 run (2026-11-09)

The L1 bundle of `test-run/batch1-1` (run 42642179431, artifact
`tests-series-g1/l1-bundle/`): Wine 11.20 test programs built with the
series and the six batch 1 v2 candidates, run by the maintainer in the
offline Windows 10 VM, x86_64 and i386. Text results only.

| Unit | Result (both architectures) | New test |
| --- | --- | --- |
| comctl32:toolbar | 2655 tests, 0 failures | `test_padding_height` |
| comctl32:header | 1208 tests, 0 failures | `test_hdm_layout_null` |
| comctl32:button | 14224 tests, 0 failures | `test_ownerdraw_capture` |
| riched20:richole | 183232 tests, 0 failures | `test_caret_after_settext` |
| riched20:editor | 10847 tests; x86_64 2 failures, i386 0 | `test_EM_SETCHARFORMAT_word_point` (SCF_WORD v2) |
| msftedit:richole | 121 tests, 0 failures | `test_textfont_applytmp` |

The two x86_64 failures are in Wine's existing `test_WM_PASTE`
(`editor.c:5642`, `:5650`, Ctrl+V/Ctrl+Y paste and redo), not in a new
test; they depend on keyboard and clipboard in the VM. The SCF_WORD v2
test passed, but v2 is superseded by v3 (msftedit's rule), whose test
(`msftedit:editor`) needs its own L1.
