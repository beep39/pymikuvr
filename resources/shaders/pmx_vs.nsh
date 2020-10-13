@include "pmx_vars.nsh"
@include "skeleton.nsh"
@include "morph.nsh"

@uniform shadow_tr "shadow tr"
@predefined model_scale "nya model scale"

@all
varying vec3 vpos;
varying vec4 shadow_tc;

@vertex
uniform vec4 shadow_tr[4];
uniform vec4 model_scale;

void main()
{
    tc = gl_MultiTexCoord0.xy;

    vec3 v = apply_morphs(gl_Vertex.xyz, gl_Vertex.w);
    pos = tr(v, int(gl_MultiTexCoord1[0])) * gl_MultiTexCoord2[0];
    normal = trn(gl_Normal, int(gl_MultiTexCoord1[0])) * gl_MultiTexCoord2[0];
    for (int i = 1; i < 4; ++i)
    {
        if (gl_MultiTexCoord2[i] > 0.0)
        {
            pos += tr(v, int(gl_MultiTexCoord1[i])) * gl_MultiTexCoord2[i];
            normal += trn(gl_Normal, int(gl_MultiTexCoord1[i])) * gl_MultiTexCoord2[i];
        }
    }

    vec3 r = normalize((gl_ModelViewProjectionMatrix * vec4(normal, 0.0)).xyz);
    r.z -= 1.0;
    env_tc = 0.5 * r.xy / length(r) + 0.5;

    vec4 wpos = gl_ModelViewProjectionMatrix * vec4(pos,1.0);
    shadow_tc = mat4(shadow_tr[0], shadow_tr[1], shadow_tr[2], shadow_tr[3]) * wpos;
    shadow_tc.xyz = 0.5 * (shadow_tc.xyz + shadow_tc.w);
    vpos = pos * model_scale.xyz;

    gl_Position = wpos;
}
