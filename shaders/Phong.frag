#version 330

uniform vec4 u_color;
uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

void main() {
  // YOUR CODE HERE
  vec3 dir = u_light_pos - v_position.xyz;
  vec3 camDir = u_cam_pos - v_position.xyz;
  vec3 midDir = (dir + camDir) / 2;

  vec3 f1 = 0.05 * u_light_intensity / (dot(dir, dir)) * max(dot(v_normal.xyz, dir), 0.0);
  vec3 f2 = 0.05 * u_light_intensity / (dot(dir, dir)) * pow(max(dot(v_normal.xyz, dir), 0.0), 4);
  vec3 f3 = vec3(0.3);
  // (Placeholder code. You will want to replace it.)
  out_color.xyz = f3 + f2 + f1;
  out_color.a = 1;
}
