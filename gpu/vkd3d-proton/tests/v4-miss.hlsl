// Original synthetic test (winum), LGPL-2.1-or-later.
struct Payload { float3x4 m; };
[shader("miss")]
void Miss(inout Payload p) { p.m[0][0] += 1.0; }
