
#include <cglm/types.h>

#include <stdbool.h>

struct Camera;
struct Viewport;

bool make_camera(struct Camera** camera);
void destroy_camera(struct Camera* camera);

void camera_assign_viewport(struct Camera* camera, struct Viewport* viewport);

void camera_screen_to_world_2d(struct Camera* camera, vec2 in_screen, vec3 out_world);
void camera_world_to_clip(struct Camera* camera, vec3 in_world, vec2 out_ndc);
void camera_world_to_screen(struct Camera* camera, vec3 in_world, vec2 out_screen);

void camera_zoom_2d(struct Camera* camera, vec2 mouse_pos, float delta);
void camera_pan_2d(struct Camera* camera, vec3 delta);
void camera_mouse_look_3d(struct Camera* camera, vec2 mouse_pos);

void camera_calc_view_matrix(struct Camera* camera, mat4 view_matrix);
void camera_calc_proj_matrix(struct Camera* camera, mat4 proj_matrix);