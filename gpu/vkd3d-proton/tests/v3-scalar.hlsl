// Original synthetic test (winum), LGPL-2.1-or-later.
struct Payload { float3x4 m; };
[shader("closesthit")]
void Hit(inout Payload p, BuiltInTriangleIntersectionAttributes a) { p.m = (float3x4)0; p.m[1][2] = RayTCurrent(); }
