@sampler base "diffuse"
@sampler toon "toon"
@sampler env "env"
@uniform env_p "env param"
@uniform alpha_test "alpha test"

@uniform light_dir "light dir":local_rot_scale=-0.4,0.82,0.4
@uniform light_color "light color"=0.6,0.6,0.6
@predefined cam_pos "nya camera position":local

@uniform amb_k "amb k"
@uniform diff_k "diff k"
@uniform spec_k "spec k"

@uniform shadow_tr "shadow tr"
@sampler shadow_map "shadow"
@sampler shadow_poisson "shadow poisson"
@uniform shadow_param "shadow param"

@all

varying vec2 tc;
varying vec2 env_tc;
varying vec3 normal;
varying vec3 pos;
varying vec4 shadow_tc;
varying vec3 ldir;

@vertex
uniform vec4 shadow_tr[4];
uniform vec4 light_dir;

void main()
{
    tc=gl_MultiTexCoord0.xy;
    pos=gl_Vertex.xyz;
    normal=gl_Normal.xyz;

    vec3 r=normalize((gl_ModelViewProjectionMatrix * vec4(normal,0.0)).xyz);
    r.z-=1.0;
    env_tc=0.5*r.xy/length(r) + 0.5;

    vec4 wpos = gl_ModelViewProjectionMatrix * gl_Vertex;
    shadow_tc = mat4(shadow_tr[0], shadow_tr[1], shadow_tr[2], shadow_tr[3]) * wpos;
    shadow_tc.xyz = 0.5 * (shadow_tc.xyz + shadow_tc.w);

    ldir = normalize(light_dir.xyz);

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

uniform sampler2D shadow_map;
uniform vec4 shadow_param;
uniform vec4 alpha_test;

//uniform sampler2D shadow_poisson;
//
//float random(vec3 seed, int i)
//{
//    vec4 seed4 = vec4(seed,i);
//    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
//    return fract(sin(dot_product) * 43758.5453);
//}

void main()
{
    vec4 c = texture2D(base,tc);
    c.a *= diff_k.a;
    if(c.a*alpha_test.x+alpha_test.y>0.0)
        discard;

    vec3 n = normalize(normal);
    float ndl = clamp(dot(ldir, n), 0.0, 1.0);

    float bias = 0.0025 * tan(acos(ndl));
    bias = clamp(bias, 0.0, 0.005);

    vec4 e = texture2D(env, env_tc);
    c=mix(c,c*e,env_p.x);
    c.rgb+=env_p.y*e.rgb;

    vec3 eye = normalize(cam_pos.xyz - pos);
    float ndh = dot(n, normalize(ldir + eye));
    vec3 spec = spec_k.rgb * max(pow(ndh, spec_k.a), 0.0);

    vec2 shad_xy = shadow_tc.xy / shadow_tc.w;
    float shad_z = (shadow_tc.z - bias) / shadow_tc.w;
    float shadow = 1.0;
    if (shadow_param.a > 0.0)
    {
    //        for (int i = 0; i < 6; ++i)
    //        {
    //            float index = random(floor(vpos*1000.0), i);
    //            vec2 dtc = texture2D(shadow_poisson, vec2(index, 0.5)).xy;
    //            shadow -= 0.1667 * step(texture2D(shadow_map, shad_xy + dtc).r - shad_z, 0.0);
    //        }

        shadow -= step(texture2D(shadow_map, shad_xy).r - shad_z, 0.0);

        vec2 b = step(vec2(0.0), shad_xy) * step(shad_xy, vec2(1.0));
        shadow = max(shadow, 1.0 - b.x * b.y);
    }

    vec3 t = texture2D(toon,vec2(0.0, min(ndl, shadow))).rgb;

    c.rgb *= clamp(amb_k.rgb + (diff_k.rgb) * light_color.rgb, vec3(0.0), vec3(1.0));
    c.rgb += spec * light_color.rgb;
    c.rgb *= t;
    gl_FragColor = c;
}
