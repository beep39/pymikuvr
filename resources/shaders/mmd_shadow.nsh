@sampler base "diffuse"
@uniform alpha_test "alpha test"
@uniform diff_k "diff k"

@all
varying vec2 tc;

@vertex

void main()
{
    tc = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
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
