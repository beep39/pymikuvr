@include "shadow.nshi"

@sampler base_map "diffuse"
@uniform alpha_test "alpha test"
@uniform amb_k "amb k"=0.4,0.4,0.4
@uniform diff_k "diff k"=0.6,0.6,0.6

@uniform light_dir "light dir":local_rot_scale=-0.4,0.82,0.4
@uniform light_color "light color"=1.0,1.0,1.0

@all
varying vec2 tc;
varying vec3 normal;

@vertex

void main()
{
    tc = gl_MultiTexCoord0.xy;
    normal = gl_Normal;

    vec4 wpos = gl_ModelViewProjectionMatrix * gl_Vertex;
    shadow(wpos);
    gl_Position = wpos;
}

@fragment
uniform sampler2D base_map;
uniform vec4 alpha_test;
uniform vec4 amb_k;
uniform vec4 diff_k;

uniform vec4 light_dir;
uniform vec4 light_color;

void main()
{
    vec4 c = texture2D(base_map,tc);
    c.a *= diff_k.a;
    if (c.a * alpha_test.x + alpha_test.y > 0.0)
        discard;

    float ndl = clamp(dot(normalize(light_dir.xyz), normalize(normal)), 0.0, 1.0);
    ndl = min(ndl, shadow(ndl));

    c.rgb *= (amb_k.rgb + light_color.rgb * diff_k.rgb * ndl);
    gl_FragColor = c;
}
