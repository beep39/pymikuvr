@sampler base_map "diffuse"
@uniform color "color"=1,1,1,1
@uniform alpha_test "alpha test"

@all
varying vec2 tc;

@vertex

void main()
{
    tc = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

@fragment
uniform sampler2D base_map;
uniform vec4 alpha_test;
uniform vec4 color;

void main()
{
    vec4 c = texture2D(base_map,tc);
    c *= color;
    if (max(max(c.r, c.g), c.b) * c.a * alpha_test.x + alpha_test.y > 0.0)
        discard;

    gl_FragColor = c;
}
