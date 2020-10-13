@sampler base "diffuse"
@sampler toon "toon"
@sampler env "env"
@uniform env_p "env param"
@uniform alpha_test "alpha test"

@uniform light_dir "light dir":local_rot=-0.4,0.82,0.4
@uniform light_ambient "light ambient"=0.4,0.4,0.4
@uniform light_color "light color"=0.6,0.6,0.6
@predefined cam_pos "nya camera position":local

@uniform amb_k "amb k"
@uniform diff_k "diff k"
@uniform spec_k "spec k"

@all

varying vec2 tc;
varying vec2 env_tc;
varying vec3 normal;
varying vec3 pos;

@fragment

uniform sampler2D base;
uniform sampler2D toon;
uniform sampler2D env;
uniform vec4 env_p;
uniform vec4 alpha_test;

uniform vec4 light_dir;
uniform vec4 light_color;
uniform vec4 cam_pos;

uniform vec4 amb_k;
uniform vec4 diff_k;
uniform vec4 spec_k;
