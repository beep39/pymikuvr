//

#pragma once

#include "singleton.h"

#include "scene/texture.h"
#include "render/fbo.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "memory/mutex.h"

#ifdef _WIN32
    #include <windows.h>
    #include <thread>
    #define USE_VR
#endif

#ifdef USE_VR
    #include <openvr.h>
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

    void block_input(bool right, bool block);
    uint32_t get_ctrl(bool right, float *ax, float *ay, float *triger);

    void reg_desktop_texture(int idx);

    void reset_resources();
    void add_resources_folder(const char *folder);
    const std::vector<std::string> get_folders() const;
    std::string get_path(const char *path) const;

    const char *load_text(const char *filename);
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
    bool m_input_blocked[2];
    float m_mposx = 0, m_mposy = 0;
    unsigned long m_script_time = 0;

    nya_resources::composite_resources_provider *m_cprov = 0;
    std::vector<nya_resources::resources_provider *> m_providers;
    std::vector<std::string> m_folders;
    std::vector<char *> m_tmp;

private:
    nya_render::fbo m_lfbo, m_rfbo;
    nya_scene::texture m_ltex, m_rtex, m_ldtex, m_rdtex;
    nya_math::mat4 m_lproj_mat, m_rproj_mat;
    nya_math::vec3 m_hmd_pos;

#ifdef USE_VR
    vr::IVRSystem *m_vr = NULL;
#else
    const static int m_vr = 0;
#endif

    struct controller
    {
        std::string serial;
        bool tracker = false;
        std::vector<nya_math::vec2> axes;
        enum btn
        {
            btn_hold,
            btn_grip,
            btn_menu,
            btn_axis0 = 16,
            btn_axis1,
        };

        uint32_t buttons = 0;
        transform *origin = 0;
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
};
