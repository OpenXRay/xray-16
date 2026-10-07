#version 450
layout(push_constant) uniform SceneConstants {
    mat4 model_view_projection;
    vec4 light_direction_ambient;
    vec4 light_color;
} scene;
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 0) out vec3 fragment_normal;
layout(location = 1) out vec4 fragment_color;
void main()
{
    gl_Position = scene.model_view_projection * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;
    fragment_normal = normal;
    fragment_color = color;
}
