@sampler base_map "main"
@uniform clip "clip"

@all
varying vec2 pos;
varying vec2 tc;
varying vec4 color;

@vertex

void main()
{
    pos = gl_Vertex.xy;
    tc = gl_MultiTexCoord0.xy;
    color = gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

@fragment
uniform sampler2D base_map;
uniform vec4 clip;

void main()
{
    vec2 b = step(clip.xy, pos) * step(pos, clip.zw);
    if (b.x * b.y < 0.5)
        discard;

    gl_FragColor = texture2D(base_map,tc) * color;
}
