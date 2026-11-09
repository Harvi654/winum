#!/usr/bin/env python3
"""Original, AI-assisted (Codex), LGPL-2.1-or-later.
Regenerate our own IDL metadata with Wine 11.20 WIDL, no Microsoft compiler.
"""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

if len(sys.argv) != 3:
    sys.exit('Usage: regenerate.py /path/to/widl-11.20 /path/to/wine-11.20/include')
widl, include = (str(Path(p).resolve()) for p in sys.argv[1:])
version = subprocess.check_output([widl, '-V'], stderr=subprocess.STDOUT, text=True)
if 'version 11.20\n' not in version:
    sys.exit('Use WIDL from the pinned Wine 11.20 source')
if (Path(include).parent/'VERSION').read_text().strip() != 'Wine version 11.20':
    sys.exit('Use the pinned Wine 11.20 include directory')
root = Path(__file__).resolve().parent
for arch, bits in (('x86_64', 64), ('i686', 32)):
    with tempfile.TemporaryDirectory() as temporary:
        work = Path(temporary)
        shutil.copyfile(root/'probe.idl', work/'probe.idl')
        subprocess.run([widl, f'-m{bits}', '-Oif', '-I', include, '-h', '-p', '-u', 'probe.idl'], cwd=work, check=True)
        proxy = work/'probe_p.c'
        text = proxy.read_text()
        table = 'static const unsigned short IWinumNdrProbe_FormatStringOffsetTable[] =\n{\n'
        if text.count(table) != 1 or text.count('&IWinumNdrProbe_FormatStringOffsetTable[-3]') != 2:
            sys.exit('Unexpected generated layout; review before updating metadata')
        # Preserve procedure 3 while avoiding an out-of-bounds biased pointer.
        text = text.replace(table, table+'    0, 0, 0, /* Unbiased IUnknown slots: keep method 3 at its legal array index. */\n')
        text = text.replace('&IWinumNdrProbe_FormatStringOffsetTable[-3]', 'IWinumNdrProbe_FormatStringOffsetTable')
        text = ('/* Own IDL metadata: WIDL 11.20 output with equivalent unbiased offset table.\n'
                ' * Original fixture, AI-assisted (Codex), LGPL-2.1-or-later. */\n')+text
        proxy.write_text(text)
        destination = root/'generated'/arch
        destination.mkdir(parents=True, exist_ok=True)
        for name in ('probe.h', 'probe_i.c', 'probe_p.c'):
            shutil.copyfile(work/name, destination/name)
