// Original synthetic test (winum), LGPL-2.1-or-later.
struct Payload { float3x4 m; float3x4 n; };
[shader("miss")]
void Miss(inout Payload p) { p.n = p.m; }
