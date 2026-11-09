/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Host serializer checks only; no Windows compatibility claim. */
#include <assert.h>
#include <stdio.h>
#include "fixture.h"
int main(void)
{
    unsigned char seed[512] = {0}, before[512], *out;
    size_t size;
    uint32_t directory = 0x58;
    memcpy(seed, "MSFT", 4);
    offsets_put32(seed + 0x20, 1);
    offsets_put32(seed + directory, 384);
    offsets_put32(seed + 384, 6);
    offsets_put32(seed + directory + 9 * 16, 496);
    offsets_put32(seed + directory + 9 * 16 + 4, 8);
    offsets_put32(seed + 496, 0x4003001a);
    offsets_put32(seed + 500, 0x80030003);
    memcpy(before, seed, sizeof(seed));
    for (uint32_t boundary = 0x8000; boundary <= 0x10000; boundary *= 2)
    {
        uint32_t desc, arrays;
        assert(offsets_fixture(seed, sizeof(seed), boundary, &out, &size));
        assert(!memcmp(seed, before, sizeof(seed)));
        desc = offsets_get32(out + directory + 9 * 16);
        arrays = offsets_get32(out + directory + 10 * 16);
        assert(offsets_span(desc, boundary + 24, size));
        assert(offsets_span(arrays, boundary + 32, size));
        assert(desc + boundary + 24 <= arrays);
        assert(offsets_get32(out + desc + 4) == boundary);
        assert(offsets_get32(out + desc + boundary + 4) == boundary);
        assert(offsets_get32(out + arrays + boundary) == boundary + 8);
        assert(offsets_get32(out + desc + boundary + 12) == boundary + 16);
        assert(offsets_get32(out + desc + boundary + 20) == boundary + 16);
        assert(offsets_get32(out + arrays + boundary + 16) == 0x80030003);
        assert(offsets_get32(out + arrays + boundary + 8) == 2);
        assert(offsets_get32(out + arrays + boundary + 24) == 2);
        assert(offsets_get32(out + arrays) == 0x80030003);
        assert(offsets_get32(out + arrays + 4) == 0x00080001);
        free(out);
    }
    assert(!offsets_fixture(seed, 83, 0x8000, &out, &size) && !out && !size);
    assert(!offsets_fixture(seed, sizeof(seed), 3, &out, &size) && !out && !size);
    offsets_put32(seed + 500, 0x80030004);
    assert(!offsets_fixture(seed, sizeof(seed), 0x8000, &out, &size) && !out && !size);
    memcpy(seed, before, sizeof(seed));
    offsets_put32(seed + directory + 9 * 16, 0xffffffff);
    assert(!offsets_fixture(seed, sizeof(seed), 0x8000, &out, &size) && !out && !size);
    puts("Serializer boundary and rejection checks passed; native semantics unmeasured.");
    return 0;
}
