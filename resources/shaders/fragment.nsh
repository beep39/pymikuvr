@sampler shadow_map "shadow"
@sampler shadow_poisson "shadow poisson"
@uniform shadow_param "shadow param"

@fragment

uniform sampler2D shadow_map;
uniform vec4 shadow_param;

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

    float ndl = dot(light_dir.xyz, normal);
    float s = 0.0;
    float l = 0.5 + 0.5 * ndl;
    vec3 t = texture2D(toon,vec2(s, l)).rgb;

    ndl = clamp(ndl, 0.0, 1.0);
    float bias = 0.0025 * tan(acos(ndl));
    bias = clamp(bias, 0.0, 0.005);

    vec4 e = texture2D(env, env_tc);
    c=mix(c,c*e,env_p.x);
    c.rgb+=env_p.y*e.rgb;

    vec3 eye = normalize(cam_pos.xyz - pos);
    float ndh = dot(normal, normalize(light_dir.xyz + eye));
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
//            shadow -= 0.1667 * step(texture2D(shadow_map, shad_xy + dtc).z - shad_z, 0.0);
//        }

        shadow -= step(texture2D(shadow_map, shad_xy).z - shad_z, 0.0);

        vec2 b = step(vec2(0.0), shad_xy) * step(shad_xy, vec2(1.0));
        shadow = max(shadow, 1.0 - b.x * b.y);

        shadow = min(1.0 - step(ndl, 0.0), shadow);
    }

    c.rgb *= clamp(amb_k.rgb + (diff_k.rgb) * light_color.rgb, vec3(0.0), vec3(1.0));
    c.rgb *= mix(1.0, shadow, shadow_param.a);
    c.rgb += spec * light_color.rgb;
    c.rgb *= t;

    gl_FragColor = c;
}
