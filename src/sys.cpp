//

#include "sys.h"
#include "GLFW/glfw3.h"
#include "render/render.h"
#include "render/bitmap.h"
#include "render/render_api.h"
#include "render/statistics.h"
#include "scene/camera.h"

#include "scene.h"
#include "player.h"
#include "texture.h"

#include "system/system.h"
#include "resources/file_resources_provider.h"
#include "resources/composite_resources_provider.h"

#include "extensions/system_openvr.h"

#ifdef _WIN32
  #include <wingdi.h>
#endif

static bool vr_available()
{
#ifdef USE_VR
    return true; //ToDo
#else
    return false;
#endif
}

bool sys::start_vr()
{
    if (m_window)
        return false;

    if (!vr_available())
        return false;

#ifdef USE_VR
    auto eError = vr::VRInitError_None;
    m_vr = vr::VR_Init(&eError, vr::VRApplication_Scene);

    if (eError != vr::VRInitError_None || !vr::VRCompositor())
    {
        m_vr = NULL;
        printf("Unable to init VR runtime: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return false;
    }

    uint32_t w, h;
    m_vr->GetRecommendedRenderTargetSize(&w, &h);
    m_width = (int)w;
    m_height = (int)h;

    m_lproj_mat = nya_system::get_proj_matrix(m_vr, nya_system::vr_eye_left, 0.1f, 300.0f);
    m_rproj_mat = nya_system::get_proj_matrix(m_vr, nya_system::vr_eye_right, 0.1f, 300.0f);
#endif

    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifndef _WIN32
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    m_window = glfwCreateWindow(32, 32, "", NULL, NULL);
    if (!m_window)
        return false;

    glfwMakeContextCurrent(m_window);

    int msaa = 2;

    m_ltex.build(0, m_width, m_height, nya_render::texture::color_rgba);
    m_rtex.build(0, m_width, m_height, nya_render::texture::color_rgba);
    m_ldtex.build(0, m_width, m_height, nya_render::texture::depth24);
    m_rdtex.build(0, m_width, m_height, nya_render::texture::depth24);
    m_lfbo.set_color_target(m_ltex.internal().get_shared_data()->tex, 0, msaa);
    m_lfbo.set_depth_target(m_ldtex.internal().get_shared_data()->tex);
    m_rfbo.set_color_target(m_rtex.internal().get_shared_data()->tex, 0, msaa);
    m_rfbo.set_depth_target(m_rdtex.internal().get_shared_data()->tex);

    nya_render::set_viewport(0, 0, m_width, m_height);

    scene::instance().init();
    scene::instance().resize(m_width, m_height);
    scene::instance().set_proj(m_lproj_mat, m_rproj_mat);
    m_time = nya_system::get_time();
    return true;
}

bool sys::start_window(int width, int height, const char *title)
{
    if (m_window)
        return false;

    if (!glfwInit())
        return false;

    //glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifndef _WIN32
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    m_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!m_window)
        return false;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
    scene::instance().init();
    m_time = nya_system::get_time();
    return true;
}

bool sys::update()
{
    if (!m_window)
        return false;

    //nya_render::statistics::begin_frame();

    const unsigned long time = nya_system::get_time();
    m_dt = (int)(time - m_time);
    m_input_blocked[0] = m_input_blocked[1] = false;
    scene::instance().update(m_dt);
    m_time = time;

#ifdef USE_VR
    vr::TrackedDevicePose_t rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
#endif

    if (m_vr)
    {
#ifdef USE_VR
        nya_system::submit_texture(nya_system::vr_eye_left, m_ltex);
        nya_system::submit_texture(nya_system::vr_eye_right, m_rtex);

        vr::VRCompositor()->WaitGetPoses(rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

        if (rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
        {
            const vr::HmdMatrix34_t &mat = rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
            auto head = player::instance().head();
            auto hmd_pos = nya_system::get_pos(mat);
            head->set_local_pos(hmd_pos - m_hmd_pos);
            head->set_local_rot(nya_system::get_rot(mat));
            m_hmd_pos = hmd_pos;

            auto origin = player::instance().origin();
            auto lhand = player::instance().lhand();
            auto rhand = player::instance().rhand();
            auto fix_pos = head->get_pos();
            fix_pos.y = origin->get_pos().y;
            auto lhand_pos = lhand->get_pos();
            auto rhand_pos = rhand->get_pos();
            origin->set_pos(fix_pos);
            head->set_local_pos(nya_math::vec3(0, hmd_pos.y, 0));
            lhand->set_pos(lhand_pos);
            rhand->set_pos(rhand_pos);
        }
#endif
        m_lfbo.bind();
        nya_scene::get_camera_proxy()->set_proj(m_lproj_mat);
        scene::instance().draw();

        m_rfbo.bind();
        nya_scene::get_camera_proxy()->set_proj(m_rproj_mat);
        scene::instance().draw();

        nya_render::fbo::unbind();
    }
    else
    {
        glfwSwapBuffers(m_window);

        scene::instance().draw();
    }

    glfwPollEvents();
    if (glfwWindowShouldClose(m_window))
        return false;

    if (m_vr)
    {
#ifdef USE_VR
        for (int i = 1; i < vr::k_unMaxTrackedDeviceCount; ++i)
        {
            const auto tdc = m_vr->GetTrackedDeviceClass(i);
            if (tdc != vr::TrackedDeviceClass_Controller && tdc != vr::TrackedDeviceClass_GenericTracker)
                continue;

            if (!rTrackedDevicePose[i].bPoseIsValid)
                continue;

            auto ci = m_controllers.find(i);
            if (ci == m_controllers.end())
            {
                auto &c = m_controllers[i];

                char tmp[1024];
                vr::VRSystem()->GetStringTrackedDeviceProperty(i, vr::Prop_ControllerType_String, tmp, sizeof(tmp));
                const bool tracker = m_controllers[i].tracker = ((tdc == vr::TrackedDeviceClass_GenericTracker) ||  strstr(tmp, "tracker") != 0);

                vr::VRSystem()->GetStringTrackedDeviceProperty(i, vr::Prop_SerialNumber_String, tmp, sizeof(tmp));
                c.serial = tmp;
                if (tracker)
                {
                    c.origin = transform::get(player::instance().add_tracker(tmp));

                    nya_log::log()<<"tracker: "<<tmp<<"\n";
                }
                else
                {
                    const bool right = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(i) == vr::ETrackedControllerRole::TrackedControllerRole_RightHand;
                    c.origin = right ? player::instance().rhand() : player::instance().lhand();
                    if (right)
                        m_controller_right = &c;
                    else
                        m_controller_left = &c;
                    c.axes.resize(5);

                    nya_log::log()<<"controller: "<<tmp<<"\n";
                }
            }

            const vr::HmdMatrix34_t &mat = rTrackedDevicePose[i].mDeviceToAbsoluteTracking;
            auto &c = m_controllers[i];
            c.origin->set_local_pos(nya_system::get_pos(mat) + (player::instance().head()->get_local_pos() - m_hmd_pos));
            c.origin->set_local_rot(nya_system::get_rot(mat) * nya_math::quat(nya_math::angle_deg(-45), 0, 0));
        }

        vr::VREvent_t event;
        while (m_vr->PollNextEvent(&event, sizeof(event)))
        {
            //ToDo: process SteamVR events
        }

        for (auto &d: m_controllers)
        {
            uint32_t buttons = 0;
            vr::VRControllerState_t state;
            m_vr->GetControllerState(d.first, &state, sizeof(state));
            for (int i = 0, to = (int)d.second.axes.size(); i < to; ++i)
            {
                d.second.axes[i].x = state.rAxis[i].x;
                d.second.axes[i].y = state.rAxis[i].y;
                if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::EVRButtonId(vr::k_EButton_Axis0 + i)))
                    buttons |= (1 << (controller::btn_axis0 + i));
            }
            if (state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_Grip))
                buttons |= (1 << controller::btn_hold);
            if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip))
                buttons |= (1 << controller::btn_grip);
            if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu))
                buttons |= (1 << controller::btn_menu);
            if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Axis0))
                buttons |= (1 << controller::btn_axis0);
            if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Axis1))
                buttons |= (1 << controller::btn_axis1);
            d.second.buttons = buttons;
        }
#endif
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        if (m_width != width || m_height != height)
        {
            m_width = width;
            m_height = height;
            scene::instance().resize(width, height);
        }

        emulate_vr_input();
    }

    if (m_external_window.is_enabled())
    {
        m_external_window.m.lock();
        if (m_external_window.rw > 0)
        {
            m_external_window.tex.update_region(m_external_window.active_buf.data(), m_external_window.rx,
                                                m_external_window.ry, m_external_window.rw, m_external_window.rh, nya_render::texture::color_bgra);
            m_external_window.rw = m_external_window.rh = 0;
        }
        m_external_window.m.unlock();
    }
    return true;
}

void sys::exit()
{
    scene::instance().release();
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

#ifdef USE_VR
    if (m_vr)
        vr::VR_Shutdown();
    m_vr = 0;
#endif
}

void sys::block_input(bool right, bool block) { m_input_blocked[right ? 1 : 0] = block; }

uint32_t sys::get_ctrl(bool right, float *jx, float *jy, float *trigger)
{
    controller *ctrl = 0;
    if (!m_input_blocked[right ? 1 : 0])
        ctrl = right ? m_controller_right : m_controller_left;

    if (!ctrl)
    {
        static controller invalid;
        ctrl = &invalid;
    }

    if (ctrl->axes.size() > 0)
    {
        *jx = ctrl->axes[0].x;
        *jy = ctrl->axes[0].y;
    }
    else
        *jx = *jy = 0;

    if (ctrl->axes.size() > 1)
        *trigger = ctrl->axes[1].x;
    else
        *trigger = 0;

    return ctrl->buttons;
}

void sys::reg_desktop_texture(int idx)
{
    if (open_desktop())
        texture::get(idx)->tex.set(m_external_window.tex);
}

void sys::reset_resources()
{
    for (auto p: m_providers)
        delete p;
    m_providers.clear();
    m_folders.clear();
    update_resources();
}

int sys::get_dt() const { return m_dt; }

void sys::add_resources_folder(const char *folder)
{
    m_providers.push_back(new nya_resources::file_resources_provider(folder));
    m_folders.push_back(folder);
    update_resources();
}

void sys::update_resources()
{
    if (!m_cprov)
        m_cprov = new nya_resources::composite_resources_provider();
    m_cprov->remove_providers();
    for (auto p: m_providers)
        m_cprov->add_provider(p);
    nya_resources::set_resources_provider(m_cprov);
}

const std::vector<std::string> sys::get_folders() const { return m_folders; }

std::string sys::get_path(const char *path) const
{
    if (!path || !path[0])
        return std::string();

    auto url = std::string(path);
    for (auto &f: sys::instance().get_folders())
    {
        auto path = f + "/" + url;
#ifdef _WIN32
        for(size_t i = 0; i < path.length();++i) if(url[i] == '/') url[i] = '\\';
#endif
        FILE *exists = fopen(path.c_str(), "r");
        if (!exists)
            continue;
        fclose(exists);
        return path;
    }
    return std::string();
}

const char *sys::load_text(const char *filename)
{
    auto res = nya_resources::get_resources_provider().access(filename);
    if (!res)
        return 0;

    auto size = res->get_size();
    auto buf = new char[size + 1];
    if (!size)
    {
        buf[0] = 0;
        m_tmp.push_back(buf);
        return buf;
    }

    if (!res->read_all(buf))
    {
        res->release();
        delete[] buf;
        return 0;
    }
    res->release();
    buf[size] = 0;
    m_tmp.push_back(buf);
    return buf;
}

int sys::list_folder(const char *path, bool include_path)
{
    auto &prov = nya_resources::get_resources_provider();
    if (!path || !path[0])
    {
        for (int i = 0, count = prov.get_resources_count(); i < count; ++i)
        {
            auto name = prov.get_resource_name(i);
            if (name)
                m_list_folder.push_back(name);
        }
    }
    else
    {
        const auto len = strlen(path);
        for (int i = 0, count = prov.get_resources_count(); i < count; ++i)
        {
            auto name = prov.get_resource_name(i);
            if (name && strncmp(path, name, len) == 0)
                m_list_folder.push_back(include_path ? name : (name + len));
        }
    }

    std::sort(m_list_folder.begin(), m_list_folder.end());
    return (int)m_list_folder.size();
}

const char *sys::get_folder_item(int idx) { return m_list_folder[idx]; }

void sys::free_tmp()
{
    for (auto t: m_tmp)
        delete[] t;
    m_tmp.clear();
    m_list_folder.clear();
}

void sys::set_znearfar(float znear, float zfar)
{
    if (m_vr)
    {
#ifdef USE_VR
        m_lproj_mat = nya_system::get_proj_matrix(m_vr, nya_system::vr_eye_left, znear, zfar);
        m_rproj_mat = nya_system::get_proj_matrix(m_vr, nya_system::vr_eye_right, znear, zfar);
#endif
        scene::instance().set_proj(m_lproj_mat, m_rproj_mat);
    }
    else
        scene::instance().resize(m_width, m_height); //this updates non-vr proj matrix
}

void sys::emulate_vr_input()
{
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        const float rot_speed = 0.01f;

        auto head = player::instance().head();
        auto origin = player::instance().origin();

        const float x = nya_math::clamp(head->get_local_rot().get_euler().x + (m_mposy - (float)ypos) * rot_speed, -1.5f, 1.5f);
        const float y = origin->get_rot().get_euler().y + (m_mposx - (float)xpos) * rot_speed;

        origin->set_local_rot(nya_math::quat(0, y, 0));
        head->set_local_rot(nya_math::quat(x, 0, 0));
    }
    m_mposx = (float)xpos, m_mposy = (float)ypos;

    if (!m_controller_right)
    {
        m_controllers[0] = controller();
        m_controllers[1] = controller();
        m_controller_left = &m_controllers[0];
        m_controller_right = &m_controllers[1];
        m_controller_left->axes.resize(2);
        m_controller_right->axes.resize(2);

        player::instance().add_tracker("TR_TEST");
    }

    float ax = 0, ay = 0, ax2 = 0;
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) ay += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) ay -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) ax -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) ax += 1.0f;
    m_controller_left->axes[0].x = ax;
    m_controller_left->axes[0].y = ay;

    ax = 0, ay = 0, ax2 = 0;
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) ay += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) ay -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) ax -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) ax += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) ax2 = 1.0f;
    m_controller_right->axes[0].x = ax;
    m_controller_right->axes[0].y = ay;
    m_controller_right->axes[1].x = ax2;

    uint32_t buttons = 0;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        buttons |= (1 << controller::btn_hold);
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)
        buttons |= (1 << controller::btn_grip);
    if (glfwGetKey(m_window, GLFW_KEY_TAB) == GLFW_PRESS)
        buttons |= (1 << controller::btn_menu);
    if (glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS)
        buttons |= (1 << controller::btn_axis0);
    if(m_controller_right->axes[1].x>0.5f)
        buttons |= (1 << controller::btn_axis1);
    m_controller_right->buttons = buttons;
}

bool sys::open_desktop()
{
    if (m_external_window.is_enabled())
        return true;

#ifdef _WIN32
    m_external_window.hDC = CreateDC("DISPLAY",0,0,0);

    BITMAP structBitmapHeader;
    memset( &structBitmapHeader, 0, sizeof(BITMAP) );

    HGDIOBJ hBitmap = GetCurrentObject(m_external_window.hDC, OBJ_BITMAP);
    GetObject(hBitmap, sizeof(BITMAP), &structBitmapHeader);

    auto width = structBitmapHeader.bmWidth, height = structBitmapHeader.bmHeight;

    m_external_window.active_buf.resize(width*height);
    m_external_window.tmp_buf.resize(width*height);

    auto &buf = m_external_window.tmp_buf;

    GetBitmapBits((HBITMAP)hBitmap, buf.size()*4, buf.data());
    m_external_window.tex.build(buf.data(), width, height, nya_render::texture::color_bgra);

    auto * w = &m_external_window;
    m_external_window.thrd = std::thread([w]
    {
        int width = GetDeviceCaps(w->hDC, HORZRES);
        int height = GetDeviceCaps(w->hDC, VERTRES);

        std::vector<uint32_t> buf(width*height);

        int rw = width, rh = height, rx = 0, ry = 0; //rect

        HDC hdcMemory = CreateCompatibleDC(w->hDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(w->hDC, rw, rh);

        while(w->hDC)
        {
            HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcMemory, hBitmap);
            BitBlt(hdcMemory, 0, 0, rw, rh, w->hDC, rx, ry, SRCCOPY);
            hBitmap = (HBITMAP)SelectObject(hdcMemory, hBitmapOld);

            GetBitmapBits((HBITMAP)hBitmap, buf.size()*4, buf.data());

            int rx = width, ry = height;
            int rw = 0, rh = 0;

            const uint32_t *prev = w->tmp_buf.data(), *curr = buf.data();
            for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                if (*prev++ == *curr++)
                    continue;

                rx = std::min(x, rx);
                ry = std::min(y, ry);

                rw = std::max(x, rw);
                rh = y;
            }

            if (!rw || !rh)
            {
                Sleep(90);
                continue;
            }

            rw -= rx;
            rh -= ry;

            w->m.lock();

            nya_render::bitmap_crop((const unsigned char *)buf.data(),width,height,rx,ry,rw,rh,4,(unsigned char *)w->active_buf.data());

            w->rx = rx;
            w->ry = ry;
            w->rw = rw;
            w->rh = rh;

            w->m.unlock();

            w->tmp_buf.swap(buf);
            Sleep(30);
        }
        DeleteDC(hdcMemory);
    });
#endif
    return true;
}
