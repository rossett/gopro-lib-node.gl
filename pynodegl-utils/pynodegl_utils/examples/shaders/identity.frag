#version 100
precision mediump float;
varying vec2 var_tex0_coord;
uniform sampler2D tex0_sampler;
uniform float delta;
void main(void)
{
    gl_FragColor = texture2D(tex0_sampler, var_tex0_coord);
}
