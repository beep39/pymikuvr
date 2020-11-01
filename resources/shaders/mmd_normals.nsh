@sampler base "diffuse"
@uniform alpha_test "alpha test"
@uniform diff_k "diff k"

@all
varying vec2 tc;
varying vec3 normal;

@vertex

void main()
{
    tc = gl_MultiTexCoord0.xy;
    normal = normalize((gl_ModelViewMatrix * vec4(gl_Normal, 0.0)).xyz);
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

@fragment
uniform sampler2D base;
uniform vec4 alpha_test;
uniform vec4 diff_k;

void main()
{
    vec4 c = texture2D(base,tc);
    c.a *= diff_k.a;
    if (c.a * alpha_test.x + alpha_test.y > 0.0)
        discard;

    gl_FragColor = vec4((gl_FrontFacing ? normal : -normal) * 0.5 + vec3(0.5), 1.0);
}
