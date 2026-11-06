// Original synthetic test (winum), LGPL-2.1-or-later.
struct Payload { float3x4 m; };
[shader("closesthit")]
void Hit(inout Payload p, BuiltInTriangleIntersectionAttributes a) { float3x4 t = ObjectToWorld3x4(); p.m = t; }
