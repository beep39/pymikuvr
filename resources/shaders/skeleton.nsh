@uniform bones_pos "mmd bones pos"
@uniform bones_rot "mmd bones rot"

@vertex

uniform vec4 bones_pos[240];
uniform vec4 bones_rot[240];

vec3 tr(vec3 v, vec4 q) { return v + cross(q.xyz, cross(q.xyz, v) + v * q.w) * 2.0; }

vec3 tr(vec3 p, int idx)
{
    vec4 q = bones_rot[idx];
    return bones_pos[idx].xyz + p + cross(q.xyz, cross(q.xyz, p) + p * q.w) * 2.0;
}

vec3 trn(vec3 n, int idx)
{
    vec4 q = bones_rot[idx];
    return n + cross(q.xyz, cross(q.xyz, n) + n * q.w) * 2.0;
}
