@sampler base_map "diffuse"
@uniform color "color"=1,1,1,1
@uniform alpha_test "alpha test"

@uniform light_dir "light dir":local_rot=-0.4,0.82,0.4
@uniform light_ambient "light ambient"=0.4,0.4,0.4
@uniform light_color "light color"=0.6,0.6,0.6

@uniform shadow_tr "shadow tr"
@sampler shadow_map "shadow"
@sampler shadow_poisson "shadow poisson"
@predefined model_scale "nya model scale"

@all
varying vec2 tc;
varying vec3 normal;
varying vec3 vpos;
varying vec4 shadow_tc;

@vertex
uniform vec4 shadow_tr[4];
uniform vec4 model_scale;

void main()
{
    tc = gl_MultiTexCoord0.xy;
    normal = gl_Normal;

    vec4 wpos = gl_ModelViewProjectionMatrix * gl_Vertex;
    shadow_tc = mat4(shadow_tr[0], shadow_tr[1], shadow_tr[2], shadow_tr[3]) * wpos;
    shadow_tc.xyz = 0.5 * (shadow_tc.xyz + shadow_tc.w);
    vpos = (gl_Vertex * model_scale).xyz;
    gl_Position = wpos;
}

@fragment
uniform sampler2D base_map;
uniform sampler2D shadow_map;
uniform vec4 alpha_test;
uniform vec4 color;

uniform vec4 light_dir;
uniform vec4 light_ambient;
uniform vec4 light_color;

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
    vec4 c = texture2D(base_map,tc);
    c *= color;
    if (c.a * alpha_test.x + alpha_test.y > 0.0)
        discard;

    float ndl = clamp(dot(light_dir.xyz, normalize(normal)), 0.0, 1.0);
    float bias = 0.0025 * tan(acos(ndl));
    bias = clamp(bias, 0.0, 0.005);

    vec2 shad_xy = shadow_tc.xy / shadow_tc.w;
    float shad_z = (shadow_tc.z - bias) / shadow_tc.w;
    float shadow = 1.0;

//    for (int i = 0; i < 6; ++i)
//    {
//        float index = random(floor(vpos*1000.0), i);
//        vec2 dtc = texture2D(shadow_poisson, vec2(index, 0.5)).xy;
//        shadow -= 0.08 * step(texture2D(shadow_map, shad_xy + dtc).r - shad_z, 0.0);
//    }

    shadow -= 0.5 * step(texture2D(shadow_map, shad_xy).r - shad_z, 0.0);

    vec2 b = step(vec2(0.0), shad_xy) * step(shad_xy, vec2(1.0));
    shadow = max(shadow, 1.0 - b.x * b.y);

    shadow = min(1.0 - step(ndl, 0.0), shadow);

    c.rgb *= (light_ambient.rgb + light_color.rgb * ndl * shadow);
    gl_FragColor = c;
}
