#version 330

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

uniform vec4 u_color;

uniform sampler2D u_texture_2;
uniform vec2 u_texture_2_size;

uniform float u_normal_scaling;
uniform float u_height_scaling;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_uv;

out vec4 out_color;

float h(vec2 uv) {
  // You may want to use this helper function...
  return texture(u_texture_2, uv).r;
}

void main() {
  // YOUR CODE HERE
  float u = v_uv.x;
  float v = v_uv.y;
  float texture_x = u_texture_2_size.x;
  float texture_y = u_texture_2_size.y;

  vec2 du_x = vec2(u + 1/texture_x, v);
  vec2 dv_y = vec2(u, v + 1/texture_y);
  vec2 base = vec2(u, v);

  float du = (h(du_x) - h(base)) * u_normal_scaling * u_height_scaling;
  float dv = (h(dv_y) - h(base)) * u_normal_scaling * u_height_scaling;

  mat3 tbn = mat3(v_tangent.xyz, cross(v_normal.xyz, v_tangent.xyz), v_normal.xyz);
  vec3 n_d = normalize(tbn * vec3(-du, -dv, 1));

  vec3 dir = normalize(u_light_pos - v_position.xyz);
  vec3 camDir = normalize(u_cam_pos - v_position.xyz);
  vec3 midDir = normalize((dir + camDir) / 2);

  vec3 f1 = 0.05 * u_light_intensity / (dot(dir, dir)) * max(dot(n_d, dir), 0.0);
  vec3 f2 = 0.05 * u_light_intensity / (dot(dir,dir)) * pow(max(0.0, dot(n_d,midDir)), 4);
  vec3 f3 = vec3(0.3);
  out_color.xyz = f1 + f2 + f3;
  out_color.a = 1;

}
