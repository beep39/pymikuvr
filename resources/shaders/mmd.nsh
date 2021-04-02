@include "shadow.nshi"

@sampler base "diffuse"
@sampler toon "toon"
@sampler env "env"
@uniform env_p "env param"
@uniform alpha_test "alpha test"

@uniform light_dir "light dir":local_rot_scale=-0.4,0.82,0.4
@uniform light_color "light color"=0.6,0.6,0.6
@predefined cam_pos "nya camera position":local
@predefined cam_rot "nya camera rotation":local

@uniform amb_k "amb k"
@uniform diff_k "diff k"
@uniform spec_k "spec k"

@uniform shadow_param "shadow param"

@all

varying vec2 tc;
varying vec2 env_tc;
varying vec3 normal;
varying vec3 pos;
varying vec3 ldir;

@vertex
uniform vec4 light_dir;
uniform vec4 cam_rot;

void main()
{
    tc = gl_MultiTexCoord0.xy;
    pos = gl_Vertex.xyz;
    normal = gl_Normal.xyz;
    env_tc = 0.5 * normal.xy + 0.5
           + cross(cross(normal.xyz, cam_rot.xyz)
           + normal.xyz * cam_rot.w, cam_rot.xyz).xy;
    ldir = normalize(light_dir.xyz);

    vec4 wpos = gl_ModelViewProjectionMatrix * gl_Vertex;
    shadow(wpos);

    gl_Position = wpos;
}

@fragment
uniform sampler2D base;
uniform sampler2D toon;
uniform sampler2D env;
uniform vec4 env_p;

uniform vec4 light_color;
uniform vec4 cam_pos;

uniform vec4 amb_k;
uniform vec4 diff_k;
uniform vec4 spec_k;

uniform vec4 shadow_param;
uniform vec4 alpha_test;

void main()
{
    vec4 c = texture2D(base, tc);
    c.a *= diff_k.a;
    if (c.a * alpha_test.x + alpha_test.y > 0.0)
        discard;

    vec3 n = normalize(normal);
    float ndl = clamp(dot(ldir, n), 0.0, 1.0);

    vec3 eye = normalize(cam_pos.xyz - pos);
    float ndh = dot(n, normalize(ldir + eye));
    vec3 spec = spec_k.rgb * max(pow(ndh, spec_k.a), 0.0);

    c.rgb *= clamp((diff_k.rgb + spec) * light_color.rgb + amb_k.rgb, vec3(0.0), vec3(1.0));

    vec4 e = texture2D(env, env_tc);
    c = mix(c, c * e, env_p.x);
    c.rgb += env_p.y * e.rgb;

    vec3 t = texture2D(toon,vec2(0.0, ndl)).rgb;
    if (shadow_param.a > 0.0)
    {
        //ToDo: pass shadow color as uniform
        t = min(t, mix(texture2D(toon, vec2(0.0, 0.0)).rgb, vec3(1.0), shadow(ndl)));
    }
    c.rgb *= t;

    gl_FragColor = c;
}
