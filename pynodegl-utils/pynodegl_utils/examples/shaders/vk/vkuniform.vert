#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

/* node.gl */
layout(location = 0) in vec3 ngl_position;
//layout(location = 1) in vec2 ngl_uvcoord;
//layout(location = 2) in vec3 ngl_normal;

layout(push_constant) uniform ngl_block {
    mat4 modelview_matrix;
    mat4 projection_matrix;
    //mat3 normal_matrix;
} ngl;

// should be an attribute but just for the exercise
layout(binding = 0, set = 0) uniform VertexParameter {
    vec4  color0;
    vec4  color1;
    vec4  color2;
} parameters;

// could be ubo/ssbo depending on performance
/*layout(binding = 0, set = 1) uniform GeometryInstance {
    float speed0;
    float speed1;
    float speed2;
} instances;*/

layout(location = 0) out vec4 color;

void main()
{
    gl_Position = ngl.projection_matrix * ngl.modelview_matrix * vec4(ngl_position, 1.0);
    color = parameters.color2;
}
