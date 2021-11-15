//

#pragma once

#include "animation.h"

#include "scene/texture.h"
#include "render/fbo.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "memory/mutex.h"
#include <deque>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    #include <thread>
#endif

#ifndef __APPLE__
    #include <openvr.h>
    #define USE_VR
#endif

struct GLFWwindow;

namespace nya_resources { class resources_provider; class composite_resources_provider; }
class transform;

class sys: public singleton<sys>
{
public:
    bool start_vr();
    bool start_window(int width, int height, const char *title);
    bool update();
    void exit();

    int get_dt() const;
    void reset_dt();

    void block_input(bool right, bool block);
    uint32_t get_ctrl(bool right, float *sx, float *sy, float *tx, float *ty, float *trigger, float *grip);
    void set_ctrl_pose(bool right, int anim_id);

    const char *pop_callback();
    template<typename... TT> static void push_callback(int id, TT... args)
    {
        if (id >= 0)
            instance().m_callback_events.push_back('[' + process_args(id, args...)  + ']');
    }

    void reg_desktop_texture(int idx);

    void reset_resources();
    void add_resources_folder(const char *folder);
    const std::vector<std::string> get_folders() const;
    std::string get_path(const char *path) const;

    const char *load_text(const char *filename);
    int list_folder(const char *path, bool include_path);
    const char *get_folder_item(int idx);
    void free_tmp();

    void set_znearfar(float znear, float zfar); //ToDo: should be in the scene

private:
    void update_resources();
    void emulate_vr_input();
    bool open_desktop();

private:
    GLFWwindow* m_window = 0;
    int m_width = 0, m_height = 0;
    unsigned long m_time = 0;
    int m_dt = 0;
    bool m_input_blocked[2] = {false, false};
    animation *m_controller_pose[2] = {0};
    float m_mposx = 0, m_mposy = 0;
    unsigned long m_script_time = 0;

    std::deque<std::string> m_callback_events;
    std::string m_callback_event;

    nya_resources::composite_resources_provider *m_cprov = 0;
    std::vector<nya_resources::resources_provider *> m_providers;
    std::vector<std::string> m_folders;

    std::vector<const char *> m_tmp;
    std::vector<const char *> m_list_folder;

private:
    nya_render::fbo m_lfbo, m_rfbo;
    nya_scene::texture m_ltex, m_rtex, m_ldtex, m_rdtex;
    nya_math::mat4 m_lproj_mat, m_rproj_mat;
    nya_math::vec3 m_hmd_pos;

#ifdef USE_VR
    vr::IVRSystem *m_vr = NULL;
    vr::IVRInput *m_vri = NULL;
    vr::VRActionSetHandle_t m_vr_input_action_handle = vr::k_ulInvalidActionHandle;

    struct
    {
        vr::VRActionHandle_t a_button = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t b_button = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t trigger = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t trigger_button = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t trigger_touch = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t grip = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t grip_button = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t grip_touch = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t axis = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t axis_button = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t axis_touch = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t axis2 = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t axis2_button = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t axis2_touch = vr::k_ulInvalidActionHandle;
    }
    m_handles;
#else
    const static int m_vr = 0;
#endif

    struct controller
    {
        std::string serial;
        bool tracker = false;
        bool right = false;

        float trigger = 0.0f;
        float grip = 0.0f;
        nya_math::vec2 axis;
        nya_math::vec2 axis2;

        enum btn
        {
            btn_a = 0,
            btn_b = 2,
            btn_trigger = 4,
            btn_trigger_touch = 5,
            btn_grip = 6,
            btn_grip_touch = 7,
            btn_axis = 8,
            btn_axis_touch = 9,
            btn_axis2 = 10,
            btn_axis2_touch = 11,
        };

        uint32_t buttons = 0;
        transform *origin = 0;
        animation *pose = 0;

#ifdef USE_VR
        vr::VRInputValueHandle_t input_handle = vr::k_ulInvalidActionHandle;
        vr::VRInputValueHandle_t skeleton_handle = vr::k_ulInvalidActionHandle;
        std::vector<vr::VRBoneTransform_t> bones_buf;
#endif
    };
    std::map<int, controller> m_controllers;

    controller *m_controller_left = 0;
    controller *m_controller_right = 0;

private:
    struct external_window
    {
#ifdef _WIN32
        const bool is_enabled() { return hDC != 0; }

        HDC hDC = 0;
        std::thread thrd;
#else
        const bool is_enabled() { return false; }
#endif
        nya_scene::texture tex;

        std::vector<uint32_t> active_buf;
        std::vector<uint32_t> tmp_buf;
        int rx = 0, ry = 0, rw = 0, rh = 0;
        nya_memory::mutex m;
    };
    external_window m_external_window;

private:
    template<typename T, typename... TT> static std::string process_args(T a, TT... args)
    {
        return process_args(a) + ',' + process_args(args...);
    }
    template<typename T> static std::string process_args(T a) { return std::to_string(a); }
    static std::string process_args(const char *a) { return process_args(std::string(a ? a : "")); }
    static std::string process_args(std::string a) { return '"' + a + '"'; }
    static std::string process_args(bool a) { return (a ? "true" : "false"); }
};
