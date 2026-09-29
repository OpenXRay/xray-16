#version 450
layout(push_constant) uniform SceneConstants {
    mat4 model_view_projection;
    vec4 light_direction_ambient;
    vec4 light_color;
} scene;
layout(location = 0) in vec3 fragment_normal;
layout(location = 1) in vec4 fragment_color;
layout(location = 0) out vec4 pixel_color;
void main()
{
    float diffuse = max(dot(normalize(fragment_normal),
        normalize(-scene.light_direction_ambient.xyz)), 0.0);
    vec3 illumination = vec3(scene.light_direction_ambient.w) +
        diffuse * scene.light_color.rgb;
    pixel_color = vec4(fragment_color.rgb * illumination, fragment_color.a);
}
