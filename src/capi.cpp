//

#include <stdio.h>
#include "animation.h"
#include "camera.h"
#include "material.h"
#include "mesh.h"
#include "navigation.h"
#include "phys.h"
#include "player.h"
#include "scene.h"
#include "shape.h"
#include "sound.h"
#include "sys.h"
#include "texture.h"
#include "transform.h"
#include "ui.h"
#include "video.h"

#ifdef _WIN32
  #define EX __declspec(dllexport)
#else
  #define EX
#endif

extern "C"
{
EX  int animation_create() { return animation::add(); }
EX  int animation_load(int id, const char *name) { return animation::get(id)->load(name); }
EX  void animation_blend(int id, int blend_id, float duration) { animation::get(id)->blend(*animation::get(blend_id), duration); }
EX  int animation_get_time(int id) { return animation::get(id)->get_time(); }
EX  void animation_set_time(int id, int time) { animation::get(id)->set_time(time); }
EX  void animation_finished(int id, int time) { animation::get(id)->is_finished(); }
EX  void animation_copy(int from, int to) { animation::copy(from, to); }
EX  void animation_set_loop(int id, bool loop) { return animation::get(id)->anim->set_loop(loop); }
EX  void animation_set_speed(int id, float speed) { return animation::get(id)->anim->set_speed(speed); }
EX  void animation_set_weight(int id, float weight) { return animation::get(id)->anim->set_weight(weight); }
EX  int animation_set_range(int id, int from, int to) { return animation::get(id)->set_range(from, to); }
EX  void animation_mirror(int id) { return animation::get(id)->mirror(); }
EX  void animation_remove(int id) { animation::remove(id); }

EX  int camera_create() { return camera::add(); }
EX  int camera_get_origin(int id) { return camera::get(id)->get_origin(); }
EX  void camera_set_enabled(int id, bool enabled) { camera::get(id)->set_enabled(enabled); }
EX  void camera_set_texture(int id, int idx) { camera::get(id)->set_texture(idx); }
EX  void camera_set_fps(int id, int fps) { camera::get(id)->set_fps(fps); }
EX  void camera_set_fov(int id, float fov) { camera::get(id)->set_fov(fov); }
EX  void camera_render_to(int id, int idx) { camera::get(id)->render_to(idx); }
EX  void camera_remove(int id) { camera::remove(id); }

EX  int material_create() { return material::add(); }
EX  void material_load(int id, const char *name) { material::get(id)->load(name); }
EX  void material_copy(int id, int copy) { material::copy(id, copy); }
EX  void material_set_texture(int id, const char *type, int tex_id) { material::get(id)->set_texture(type, tex_id); }
EX  void material_set_param(int id, const char *type, float x, float y, float z, float w) { material::get(id)->set_param(type, x, y, z, w); }
EX  void material_remove(int id) { material::remove(id); }

EX  int mesh_create() { return mesh::add(); }
EX  int mesh_get_origin(int id) { return mesh::get(id)->get_origin(); }
EX  void mesh_set_enabled(int id, bool enabled) { mesh::get(id)->set_enabled(enabled); }
EX  bool mesh_load(int id, const char *name) { return mesh::get(id)->load(name); }
EX  void mesh_set_animation(int id, int anim_id, int layer)
{
    if (anim_id < 0)
        mesh::get(id)->remove_animation(layer);
    else
        animation::get(anim_id)->set_mesh(id, layer);
}
EX  int mesh_get_bones_count(int id) { return mesh::get(id)->get_bones_count(); }
EX  const char *mesh_get_bone_name(int id, int idx) { return mesh::get(id)->get_bone_name(idx); }
EX  int mesh_get_bone(int id, const char *name) { return mesh::get(id)->get_bone(name); }
EX  void mesh_set_bone_pos(int id, const char *name, float x, float y, float z, bool additive)
    {
        mesh::get(id)->set_bone_pos(name, nya_math::vec3(x, y, z), additive);
    }
EX  void mesh_set_bone_rot(int id, const char *name, float x, float y, float z, float w, bool additive)
    {
        mesh::get(id)->set_bone_rot(name, nya_math::quat(x, y, z, w), additive);
    }
EX  int mesh_get_morphs_count(int id) { return mesh::get(id)->get_morphs_count(); }
EX  const char *mesh_get_morph_name(int id, int idx) { return mesh::get(id)->get_morph_name(idx); }
EX  float mesh_get_morph(int id, const char *name) { return mesh::get(id)->get_morph(name); }
EX  void mesh_set_morph(int id, const char *name, float value, bool override_animation)
    {
        mesh::get(id)->set_morph(name, value, override_animation);
    }
EX  int mesh_get_materials_count(int id) { return mesh::get(id)->get_groups_count(); }
EX  const char *mesh_get_material_name(int id, int idx) { return mesh::get(id)->get_group_name(idx); }
EX  void mesh_material_visible(int id, int idx, bool visible) { mesh::get(id)->set_group_visible(idx, visible); }
EX  void mesh_init_texture(int id, int idx, int tex_id) { mesh::get(id)->mesh_init_texture(idx, tex_id); }
EX  void mesh_set_texture(int id, int idx, int tex_id) { mesh::get(id)->mesh_set_texture(idx, tex_id); }
EX  void mesh_remove(int id) { mesh::remove(id); }

EX  int navigation_create() { return navigation::add(); }
EX  void navigation_set_debug(int id, bool enable) { navigation::get(id)->set_debug(enable); }
EX  void navigation_clear(int id) { navigation::get(id)->clear(); }
EX  void navigation_add_shape(int id, int idx) { navigation::get(id)->add_shape(idx); }
EX  void navigation_add_mesh(int id, int idx) { navigation::get(id)->add_mesh(idx); }
EX  void navigation_build(int id, float cell_size, float cell_height, float agent_height, float agent_radius, float max_climb, float max_slope, float min_region_size, float merged_region_size, float max_edge_length, float max_edge_error)
    {
        navigation::params p;
        p.cell_size = cell_size;
        p.cell_height = cell_height;
        p.agent_height = agent_height;
        p.agent_radius = agent_radius;
        p.max_climb = max_climb;
        p.max_slope = max_slope;
        p.min_region_size = min_region_size;
        p.merged_region_size = merged_region_size;
        p.max_edge_length = max_edge_length;
        p.max_edge_error = max_edge_error;
        navigation::get(id)->build(p);
    }
EX  bool navigation_nearest_point(int id, float x, float y, float z, float radius, float *rx, float *ry, float *rz)
    {
        nya_math::vec3 result;
        if (!navigation::get(id)->nearest_point(nya_math::vec3(x,y,z), radius, result))
            return false;
        *rx = result.x, *ry = result.y, *rz = result.z;
        return true;
    }
EX  bool navigation_farthest_point(int id, float x, float y, float z, float dx, float dz, float radius, float *rx, float *ry, float *rz)
    {
        nya_math::vec3 result;
        if (!navigation::get(id)->farthest_point(nya_math::vec3(x,y,z), nya_math::vec3(dx, 0, dz), radius, result))
            return false;
        *rx = result.x, *ry = result.y, *rz = result.z;
        return true;
    }
EX  int navigation_path(int id, float x, float y, float z, float tx, float ty, float tz, float *result, int result_array_size)
    {
        const nya_math::vec3 from(x, y, z), to(tx, ty, tz);
        return navigation::get(id)->path(from, to, result, result_array_size);
    }
EX  void navigation_remove(int id) { navigation::remove(id); }

EX  void phys_set_debug(bool enable) { phys::set_debug(enable); }
EX  int phys_get_origin(int id) { return phys::get(id)->get_origin(); }
EX  void phys_set_enabled(int id, bool enabled) { phys::get(id)->set_enabled(enabled); }
EX  void phys_set_ground(float y, bool enable) { phys::set_ground(y, enable); }
EX  int phys_create() { return phys::add(); }
EX  void phys_set_box(int id, float x, float y, float z, float m) { phys::get(id)->set_box(x, y, z, m); }
EX  void phys_set_sphere(int id, float r, float m) { phys::get(id)->set_sphere(r, m); }
EX  void phys_set_capsule(int id, float r, float h, float m) { phys::get(id)->set_capsule(r, h, m); }
EX  void phys_set_cylinder(int id, float r, float h, float m) { phys::get(id)->set_cylinder(r, h, m); }
EX  void phys_set_cone(int id, float r, float h, float m) { phys::get(id)->set_cone(r, h, m); }
EX  void phys_clear(int id) { phys::get(id)->clear(); }
EX  void phys_add_mesh(int id, int src) { phys::get(id)->add_mesh(src); }
EX  void phys_add_shape(int id, int src) { phys::get(id)->add_shape(src); }
EX  bool phys_build(int id, float mass) { return phys::get(id)->build(mass); }
EX  bool phys_trace_world(float x, float y, float z, float dx, float dy, float dz, float *result, int *id, float max_dist)
    {
        return phys::trace(nya_math::vec3(x, y, z), nya_math::vec3(dx, dy, dz), result, id, max_dist);
    }
EX  bool phys_trace(int id, float x, float y, float z, float dx, float dy, float dz, float *result, float max_dist)
    {
        return phys::get(id)->trace(nya_math::vec3(x, y, z), nya_math::vec3(dx, dy, dz), result, max_dist);
    }
EX  void phys_remove(int id) { phys::remove(id); }

EX  int player_get_transform(const char *name) { return player::instance().get_transform(name); }

EX  void render_set_znearfar(float znear, float zfar) { return scene::instance().set_znearfar(znear, zfar); }
EX  void render_light_ambient(float r, float g, float b) { return scene::instance().set_light_ambient(r, g, b); }
EX  void render_light_color(float r, float g, float b) { return scene::instance().set_light_color(r, g, b); }
EX  void render_light_dir(float x, float y, float z) { return scene::instance().set_light_dir(x, y, z); }
EX  bool render_pipeline_load(const char *name) { return scene::instance().load_postprocess(name); }
EX  bool render_pipeline_get_condition(const char *name) { return scene::instance().get_condition(name); }
EX  void render_pipeline_set_condition(const char *name, bool enable) { scene::instance().set_condition(name, enable); }
EX  void render_shadows_enabled(bool enabled) { scene::instance().set_shadows_enabled(enabled); }
EX  void render_shadows_resolution(int resolution) { scene::instance().set_shadows_resolution(resolution); }
EX  void render_shadows_cascades(float c0, float c1, float c2, float c3) { scene::instance().set_shadows_cascades(c0, c1, c2, c3); }
EX  void render_set_shadows_bias(int idx, float c, float s) { scene::instance().set_shadows_bias(idx, c, s); }
EX  int shape_create() { return shape::add(); }
EX  int shape_get_origin(int id) { return shape::get(id)->get_origin(); }
EX  void shape_set_enabled(int id, bool enabled) { shape::get(id)->set_enabled(enabled); }
EX  void shape_set_material(int id, int mat_id) { return shape::get(id)->set_material(mat_id); }
EX  void shape_set_sphere(int id, float r, float s, float t, bool ntc) { shape::get(id)->set_sphere(r, s, t, ntc); }
EX  void shape_set_cylinder(int id, float r, float h, float s, float t, bool ntc) { shape::get(id)->set_cylinder(r, h, s, t, ntc); }
EX  void shape_set_box(int id, float x, float y, float z, float s, float t, bool ntc) { shape::get(id)->set_box(x, y, z, s, t, ntc); }
EX  void shape_set_plane(int id, float w, float h, float s, float t, bool ntc) { shape::get(id)->set_plane(w, h, s, t, ntc); }
EX  void shape_set_heightmap(int id, int count_w, int count_h, float step, const float *height, float scale, float s, float t, bool ntc)
{
    shape::get(id)->set_heightmap(count_w, count_h, step, height, scale, s, t, ntc);
}
EX  void shape_set_heightmap_tex(int id, int count_w, int count_h, float step, const int height_tex_id, float scale, float s, float t, bool ntc)
{
    shape::get(id)->set_heightmap(count_w, count_h, step, texture::get(height_tex_id)->tex, scale, s, t, ntc);
}

EX  void shape_add_shape(int id, int other) { shape::get(id)->add_shape(shape::get(other)); }
EX  void shape_set_color(int id, float r, float g, float b, float a) { shape::get(id)->set_color(r,g,b,a); }
EX  bool shape_trace(int id, float x, float y, float z, float dx, float dy, float dz, float *result)
    {
        return shape::get(id)->trace(nya_math::vec3(x, y, z), nya_math::vec3(dx, dy, dz), *result);
    }
EX  void shape_remove(int id) { shape::remove(id); }

EX  int sound_create() { return sound::add(); }
EX  int sound_get_origin(int id) { return sound::get(id)->get_origin(); }
EX  void sound_set_enabled(int id, bool enabled) { sound::get(id)->set_enabled(enabled); }
EX  int sound_play(int id, const char *name, bool loop, float fade) { return sound::get(id)->play(name, loop, fade); }
EX  void sound_stop(int id, float fade) { sound::get(id)->stop(fade); }
EX  void sound_set_volume(int id, float volume) { sound::get(id)->set_volume(volume); }
EX  void sound_set_pitch(int id, float pitch) { sound::get(id)->set_pitch(pitch); }
EX  void sound_set_radius(int id, float radius) { sound::get(id)->set_radius(radius); }
EX  int sound_get_level(int id) { return sound::get(id)->get_level(); }
EX  int sound_get_time(int id) { return sound::get(id)->get_time(); }
EX  void sound_set_time(int id, int time) { sound::get(id)->set_time(time); }
EX  bool sound_play2d(const char *name, float volume) { return sound::play2d(name, volume); }
EX  bool sound_play3d(const char *name, float x, float y, float z, float volume, float pitch, float radius)
    {
        return sound::play3d(name, x, y, z, volume, pitch, radius);
    }
EX  bool sound_preload(const char *name) { return sound::preload(name); }
EX  void sound_remove(int id) { sound::remove(id); }

EX  bool sys_start_vr() { return sys::instance().start_vr(); }
EX  bool sys_start_window(int width, int height, const char *title)
    {
        return sys::instance().start_window(width, height, title);
    }
EX  bool sys_update() { return sys::instance().update(); }
EX  void sys_reset_dt() { sys::instance().reset_dt(); }
EX  void sys_exit() { sys::instance().exit(); }
EX  unsigned int sys_get_ctrl(bool right, float *jx, float *jy, float *trigger)
    {
        return sys::instance().get_ctrl(right, jx, jy, trigger);
    }
EX  int sys_get_trackers_count() { return player::instance().get_trackers_count(); }
EX  const char *sys_get_tracker(int idx, int *transform)
    {
        *transform = player::instance().get_tracker(idx);
        return player::instance().get_tracker_name(idx);
    }
EX  void sys_reg_desktop_texture(int id) { sys::instance().reg_desktop_texture(id); }
EX  void sys_reset_resources() { sys::instance().reset_resources(); }
EX  void sys_add_resources_folder(const char *folder) { sys::instance().add_resources_folder(folder); }
EX  const char *sys_load_text(const char *filename) { return sys::instance().load_text(filename); }
EX  int sys_list_folder(const char *path, bool include_path) { return sys::instance().list_folder(path, include_path); }
EX  const char *sys_get_folder_item(int idx) { return sys::instance().get_folder_item(idx); }
EX  void sys_free_tmp() { sys::instance().free_tmp(); }

EX  int texture_create() { return texture::add(); }
EX  bool texture_load(int id, const char *name) { return texture::get(id)->tex->load(name); }
EX  bool texture_save(int id, const char *name) { return texture::get(id)->save(name); }
EX  void texture_build(int id, int w, int h, float r, float g, float b, float a) { texture::get(id)->build(w, h, r, g, b, a); }
EX  int texture_get_width(int id) { return texture::get(id)->tex->get_width(); }
EX  int texture_get_height(int id) { return texture::get(id)->tex->get_height(); }
EX  void texture_copy(int from, int to) { texture::copy(from, to); }
EX  void texture_remove(int id) { texture::remove(id); }

EX  int transform_create() { return transform::add(); }
EX  bool transform_set_parent(int id, int parent)
    {
        return transform::get(id)->set_parent(parent < 0 ? 0 : transform::get(parent));
    }
EX  void transform_set_pos(int id, float x, float y, float z)
    {
        transform::get(id)->set_pos(nya_math::vec3(x,y,z));
    }
EX  void transform_get_pos(int id, float *x, float *y, float *z)
    {
        const auto &p = transform::get(id)->get_pos();
        *x = p.x; *y = p.y; *z = p.z;
    }
EX  void transform_set_rot(int id, float x, float y, float z, float w)
    {
        transform::get(id)->set_rot(nya_math::quat(x, y, z, w));
    }
EX  void transform_get_rot(int id, float *x, float *y, float *z, float *w)
    {
        const auto &r = transform::get(id)->get_rot();
        *x = r.v.x; *y = r.v.y; *z = r.v.z; *w = r.w;
    }
EX  void transform_set_local_pos(int id, float x, float y, float z)
    {
        transform::get(id)->set_local_pos(nya_math::vec3(x,y,z));
    }
EX  void transform_get_local_pos(int id, float *x, float *y, float *z)
    {
        const auto &p = transform::get(id)->get_local_pos();
        *x = p.x; *y = p.y; *z = p.z;
    }
EX  void transform_set_local_rot(int id, float x, float y, float z, float w)
    {
        transform::get(id)->set_local_rot(nya_math::quat(x, y, z, w));
    }
EX  void transform_get_local_rot(int id, float *x, float *y, float *z, float *w)
    {
        const auto &r = transform::get(id)->get_local_rot();
        *x = r.v.x; *y = r.v.y; *z = r.v.z; *w = r.w;
    }
EX  void transform_remove(int id) { transform::remove(id); }

EX  bool ui_set_font(const char *path, int size, float scale, const char *add) { return ui::set_font(path, size, scale, add); }
EX  int ui_create() { return ui::add(); }
EX  int ui_get_origin(int id) { return ui::get(id)->get_origin(); }
EX  void ui_set_enabled(int id, bool enable) { return ui::get(id)->set_enable(enable); }
EX  void ui_set_pivot(int id, float w, float h) { return ui::get(id)->set_pivot(w, h); }
EX  void ui_set_size(int id, float w, float h) { return ui::get(id)->set_size(w, h); }
EX  void ui_set_caption(int id, const char *caption) { return ui::get(id)->set_caption(caption); }
EX  void ui_set_style(int id, const char *style) { return ui::get(id)->set_style(style); }
EX  void ui_set_ignore_input(int id, bool ignore) { return ui::get(id)->set_ignore_input(ignore); }
EX  void ui_set_background(int id, bool enabled) { return ui::get(id)->set_background(enabled); }
EX  int ui_add_text(int id, const char *text) { return ui::get(id)->add_text(text); }
EX  int ui_add_btn(int id, const char *text, int c) { return ui::get(id)->add_btn(text, c); }
EX  int ui_add_checkbox(int id, const char *text, int c, bool radio) { return ui::get(id)->add_checkbox(text, c, radio); }
EX  int ui_add_slider(int id, const char *text, int c, float f, float t) { return ui::get(id)->add_slider(text, c, f, t); }
EX  int ui_add_coloredit(int id, const char *text, int c, bool alpha) { return ui::get(id)->add_coloredit(text, c, alpha); }
EX  int ui_add_dropdown(int id, int c, const char **items, int count)
    {
        return ui::get(id)->add_dropdown(c, items, count);
    }
EX  int ui_add_listbox(int id, int c, const char **items, int count)
    {
        return ui::get(id)->add_listbox(c, items, count);
    }
EX  int ui_add_tab(int id, const char *text) { return ui::get(id)->add_tab(text); }
EX  int ui_add_spacing(int id, bool separator) { return ui::get(id)->add_spacing(separator); }
EX  int ui_add_hlayout(int id, int count) { return ui::get(id)->add_hlayout(count); }
EX  int ui_add_scroll(int id, int count) { return ui::get(id)->add_scroll(count); }
EX  void ui_set_text(int id, int idx, const char *text) { ui::get(id)->set_text(idx, text); }
EX  bool ui_get_bool(int id, int idx) { return ui::get(id)->get_bool(idx); }
EX  void ui_set_bool(int id, int idx, bool v) { ui::get(id)->set_bool(idx, v); }
EX  float ui_get_slider(int id, int idx) { return ui::get(id)->get_slider(idx); }
EX  void ui_set_slider(int id, int idx, float v) { ui::get(id)->set_slider(idx, v); }
EX  const char *ui_get_list_value(int id, int idx) { return ui::get(id)->get_list_value(idx); }
EX  void ui_set_list_value(int id, int idx, const char *v) { ui::get(id)->set_list_value(idx, v); }
EX  int ui_get_list_idx(int id, int idx) { return ui::get(id)->get_list_idx(idx); }
EX  void ui_set_list_idx(int id, int idx, int v) { ui::get(id)->set_list_idx(idx, v); }
EX  void ui_get_color(int id, int idx, float *r, float *g, float *b, float *a)
    {
        ui::get(id)->get_color(idx, r, g, b, a);
    }
EX  void ui_set_color(int id, int idx, float r, float g, float b, float a)
    {
        ui::get(id)->set_color(idx, r, g, b, a);
    }
EX  void ui_set_list(int id, int idx, const char **items, int count) { ui::get(id)->set_list(idx, items, count); }
EX  void ui_remove(int id) { return ui::remove(id); }

EX  int video_create() { return video::add(); }
EX  void video_set_texture(int id, int idx) { video::get(id)->set_texture(idx); }
EX  bool video_play(int id, const char *path, const char *audio_path, bool loop) { return video::get(id)->play(path, audio_path, loop); }
EX  void video_pause(int id) { video::get(id)->pause(); }
EX  void video_stop(int id) { video::get(id)->stop(); }
EX  int video_get_time(int id) { return video::get(id)->get_time(); }
EX  void video_set_time(int id, int time) { video::get(id)->set_time(time); }
EX  void video_set_volume(int id, float volume) { video::get(id)->set_volume(volume); }
EX  int video_get_duration(int id) { return video::get(id)->get_length(); }
EX  void video_remove(int id) { video::remove(id); }
}
