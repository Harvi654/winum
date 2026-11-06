// Original synthetic test (winum), LGPL-2.1-or-later.
struct Payload { float3x4 m; };
[shader("closesthit")]
void Hit(inout Payload p, BuiltInTriangleIntersectionAttributes a) { p.m = ObjectToWorld3x4(); }
