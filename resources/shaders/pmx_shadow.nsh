@include "skeleton.nsh"
@include "morph.nsh"

@sampler base "diffuse"
@uniform alpha_test "alpha test"
@uniform diff_k "diff k"

@all
varying vec2 tc;

@vertex

void main()
{
    tc = gl_MultiTexCoord0.xy;

    vec3 vpos = apply_morphs(gl_Vertex.xyz, gl_Vertex.w);
    vec3 pos = tr(vpos, int(gl_MultiTexCoord1[0])) * gl_MultiTexCoord2[0];
    for (int i = 1; i < 4; ++i)
    {
        if (gl_MultiTexCoord2[i] > 0.0)
            pos += tr(vpos, int(gl_MultiTexCoord1[i])) * gl_MultiTexCoord2[i];
    }
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}

@fragment
uniform sampler2D base;
uniform vec4 diff_k;
uniform vec4 alpha_test;

void main()
{
    vec4 c = texture2D(base,tc);
    c.a *= diff_k.a;
    if (c.a * alpha_test.x + alpha_test.y > 0.0)
        discard;

    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
