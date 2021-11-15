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

    for (auto &folder: m_folders)
    {
        const auto path = folder + "openvr/actions.json";
        if (!nya_resources::file_resources_provider().has(path.c_str()))
            continue;

        m_vri = vr::VRInput();
        m_vri->SetActionManifestPath(path.c_str());
        m_vri->GetActionSetHandle("/actions/default", &m_vr_input_action_handle);

        m_vri->GetActionHandle("/actions/default/in/A_Button", &m_handles.a_button);
        m_vri->GetActionHandle("/actions/default/in/B_Button", &m_handles.b_button);
        m_vri->GetActionHandle("/actions/default/in/Trigger_Axis", &m_handles.trigger);
        m_vri->GetActionHandle("/actions/default/in/Trigger_Button", &m_handles.trigger_button);
        m_vri->GetActionHandle("/actions/default/in/Trigger_Touch", &m_handles.trigger_touch);
        m_vri->GetActionHandle("/actions/default/in/Grip_Squeeze", &m_handles.grip);
        m_vri->GetActionHandle("/actions/default/in/Grip_Button", &m_handles.grip_button);
        m_vri->GetActionHandle("/actions/default/in/Grip_Touch", &m_handles.grip_touch);
        m_vri->GetActionHandle("/actions/default/in/Primary2Axis_Axes", &m_handles.axis);
        m_vri->GetActionHandle("/actions/default/in/Primary2Axis_Button", &m_handles.axis_button);
        m_vri->GetActionHandle("/actions/default/in/Primary2Axis_Button", &m_handles.axis_touch);
        m_vri->GetActionHandle("/actions/default/in/Secondary2Axis_Axes", &m_handles.axis2);
        m_vri->GetActionHandle("/actions/default/in/Secondary2Axis_Button", &m_handles.axis2_button);
        m_vri->GetActionHandle("/actions/default/in/Secondary2Axis_Touch", &m_handles.axis2_touch);

        break;
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
#ifdef __APPLE__
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

    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    //glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
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
        if (m_vr_input_action_handle != vr::k_ulInvalidActionHandle)
        {
            vr::VRActiveActionSet_t t;
            t.ulActionSet = m_vr_input_action_handle;
            t.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
            t.nPriority = 0;
            m_vri->UpdateActionState(&t,sizeof(t),1);
        }

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
                    {
                        m_vri->GetInputSourceHandle("/user/hand/right", &c.input_handle);
                        m_vri->GetInputSourceHandle("/actions/default/in/SkeletonRightHand", &c.skeleton_handle);
                        m_controller_right = &c;
                        c.pose = m_controller_pose[1];
                    }
                    else
                    {
                        m_vri->GetInputSourceHandle("/user/hand/left", &c.input_handle);
                        m_vri->GetInputSourceHandle("/actions/default/in/SkeletonLeftHand", &c.skeleton_handle);
                        m_controller_left = &c;
                        c.pose = m_controller_pose[0];
                    }
                    c.right = right;

                    if (m_vri)
                    {
                        uint32_t bone_count = 0;
                        auto bc_error = m_vri->GetBoneCount(c.skeleton_handle, &bone_count);
                        if (bc_error == vr::VRInputError_None)
                        {
                            const int anim_bones_count = c.pose->anim->get_shared_data()->anim.get_bones_count();
                            if (bone_count >= anim_bones_count)
                                c.bones_buf.resize(bone_count);
                            else
                                nya_log::log()<<"bone count "<<bone_count<<" is less than expected "<<anim_bones_count<<"\n";
                        }
                        else
                            nya_log::log()<<"OpenVR GetBoneCount error: "<<bc_error<<"\n";
                    }

                    nya_log::log()<<"controller: "<<tmp<<" bones: "<<c.bones_buf.size()<<"\n";
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
            auto &c = d.second;
            if (c.tracker)
                continue;

            c.buttons = 0;
            vr::InputDigitalActionData_t ddata;
            vr::InputAnalogActionData_t adata;

            if (m_vri->GetDigitalActionData(m_handles.a_button, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_a);
            if (m_vri->GetDigitalActionData(m_handles.b_button, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_b);

            c.trigger = 0.0f;
            if (m_vri->GetAnalogActionData(m_handles.trigger, &adata, sizeof(adata), c.input_handle) == vr::VRInputError_None)
                c.trigger = adata.x;
            if (m_vri->GetDigitalActionData(m_handles.trigger_button, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
            {
                c.trigger = 1.0f;
                c.buttons |= (1 << controller::btn_trigger);
            }
            if (m_vri->GetDigitalActionData(m_handles.trigger_touch, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_trigger_touch);

            if (m_vri->GetAnalogActionData(m_handles.grip, &adata, sizeof(adata), c.input_handle) == vr::VRInputError_None)
                c.grip = adata.x;
            if (m_vri->GetDigitalActionData(m_handles.grip_button, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_grip);
            if (m_vri->GetDigitalActionData(m_handles.grip_touch, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_grip_touch);

            if (m_vri->GetAnalogActionData(m_handles.axis, &adata, sizeof(adata), c.input_handle) == vr::VRInputError_None)
            {
                c.axis.x = adata.x;
                c.axis.y = adata.y;
            }
            if (m_vri->GetDigitalActionData(m_handles.axis_button, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_axis);
            if (m_vri->GetDigitalActionData(m_handles.axis_touch, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_axis_touch);

            if (m_vri->GetAnalogActionData(m_handles.axis2, &adata, sizeof(adata), c.input_handle) == vr::VRInputError_None)
            {
                c.axis2.x = adata.x;
                c.axis2.y = adata.y;
            }
            if (m_vri->GetDigitalActionData(m_handles.axis2_button, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_axis2);
            if (m_vri->GetDigitalActionData(m_handles.axis2_touch, &ddata, sizeof(ddata), c.input_handle) == vr::VRInputError_None && ddata.bState)
                c.buttons |= (1 << controller::btn_axis2_touch);

            if (c.bones_buf.empty())
                continue;
 
            vr::InputSkeletalActionData_t action_data;
            const auto sd_error = m_vri->GetSkeletalActionData(c.skeleton_handle, &action_data, sizeof(action_data));
            if (sd_error != vr::VRInputError_None)
            {
                if (sd_error == vr::VRInputError_NoData)
                    continue;

                nya_log::log()<<"OpenVR GetSkeletalActionData error: "<<sd_error<<"\n";
                c.bones_buf.clear();
                continue;
            }

            if (!action_data.bActive)
                continue;

            if (m_vri->GetSkeletalBoneData(c.skeleton_handle, vr::VRSkeletalTransformSpace_Parent, vr::VRSkeletalMotionRange_WithoutController,
                                           c.bones_buf.data(), (uint32_t)c.bones_buf.size()) != vr::VRInputError_None)
                continue;

            auto &anim = *(nya_render::animation *)&(c.pose->anim->get_shared_data()->anim);
            for (int i = 0, count = anim.get_bones_count(); i < count; ++i)
            {
                const auto r = c.bones_buf[i].orientation;

                if (c.right)
                    anim.add_bone_rot_frame(i, 0, nya_math::quat(-r.x, r.y, r.z, r.w));
                else
                    anim.add_bone_rot_frame(i, 0, nya_math::quat(r.x, r.y, -r.z, r.w));
            }
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

const char *sys::pop_callback()
{
    if (m_callback_events.empty())
        return 0;

    m_callback_event = m_callback_events.front();
    m_callback_events.pop_front();
    return m_callback_event.c_str();
}

void sys::block_input(bool right, bool block) { m_input_blocked[right ? 1 : 0] = block; }

uint32_t sys::get_ctrl(bool right, float *sx, float *sy, float *tx, float *ty, float *trigger, float *grip)
{
    controller *ctrl = 0;
    if (!m_input_blocked[right ? 1 : 0])
        ctrl = right ? m_controller_right : m_controller_left;

    if (!ctrl)
    {
        static controller invalid;
        ctrl = &invalid;
    }

    *sx = ctrl->axis.x;
    *sy = ctrl->axis.y;

    *tx = ctrl->axis2.x;
    *ty = ctrl->axis2.y;

    *trigger = ctrl->trigger;
    *grip = ctrl->grip;

    return ctrl->buttons;
}

void sys::set_ctrl_pose(bool right, int anim_id)
{
    auto &pose = m_controller_pose[right ? 1 : 0];
    pose = animation::get(anim_id);
    pose->anim.create();

    //Reference: https://github.com/ValveSoftware/openvr/wiki/Hand-Skeleton#bone-structure

    const std::string bones[] =
    {
        "Root", "Wrist",
        "親指０", "親指１", "親指２", "親指先",
        "人指０", "人指１", "人指２", "人指３", "人指先",
        "中指０", "中指１", "中指２", "中指３", "中指先",
        "薬指０", "薬指１", "薬指２", "薬指３", "薬指先",
        "小指０", "小指１", "小指２", "小指３", "小指先",
    };
    const std::string lr = right ? "右" : "左";

    nya_scene::shared_animation a;
    for (auto &b: bones)
        a.anim.add_bone((lr + b).c_str());
    pose->anim->create(a);

    if (right && m_controller_right)
        m_controller_right->pose = pose;
    else if (m_controller_left)
        m_controller_left->pose = pose;
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

void sys::reset_dt()
{
    m_dt = 0;
    m_time = nya_system::get_time();
}

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

int ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return false;
    const size_t lenstr = strlen(str);
    const size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return false;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int sys::list_folder(const char *path, bool include_path)
{
    const size_t path_len = path ? strlen(path) : 0;
    std::string path_buf;
    if (path_len > 0)
    {
        path_buf = path;
        for(size_t i = 0; i < path_len; ++i)
        {
            if(path_buf[i]=='\\')
                path_buf[i]='/';
        }
        path = path_buf.c_str();
    }

    auto &prov = nya_resources::get_resources_provider();
    for (int i = 0, count = prov.get_resources_count(); i < count; ++i)
    {
        auto name = prov.get_resource_name(i);
        if (!name || ends_with(name, ".DS_Store") || ends_with(name, "Thumbs.db"))
            continue;

        if (!path_len)
            m_list_folder.push_back(name);
        else if (strncmp(path, name, path_len) == 0)
            m_list_folder.push_back(include_path ? name : (name + path_len));
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

        m_controller_left->pose = m_controller_pose[0];
        m_controller_right->pose = m_controller_pose[1];

        player::instance().add_tracker("TR_TEST");
    }

    float ax = 0, ay = 0, ax2 = 0;
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) ay += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) ay -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) ax -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) ax += 1.0f;
    m_controller_left->axis.x = ax;
    m_controller_left->axis.y = ay;

    ax = 0, ay = 0, ax2 = 0;
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) ay += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) ay -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) ax -= 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) ax += 1.0f;
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) ax2 = 1.0f;
    m_controller_right->axis.x = ax;
    m_controller_right->axis.y = ay;
    m_controller_right->trigger = ax2;

    uint32_t buttons = 0;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        buttons |= (1 << controller::btn_grip_touch);
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)
        buttons |= (1 << controller::btn_grip);
    if (glfwGetKey(m_window, GLFW_KEY_TAB) == GLFW_PRESS)
        buttons |= (1 << controller::btn_a);
    if (glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS)
        buttons |= (1 << controller::btn_axis);
    if(m_controller_right->trigger > 0.5f)
        buttons |= (1 << controller::btn_trigger);
    m_controller_right->buttons = buttons;

    auto &anim = *(nya_render::animation *)&m_controller_right->pose->anim->get_shared_data()->anim;
    if (buttons & (1 << controller::btn_grip_touch))
    {
        nya_math::quat q1(-0.10f, 0.05f, 0.68f, 0.72f),
                       q2(0.0f, 0.0f, 0.77f, 0.64f),
                       q3(0.0f, 0.0f, 0.75f, 0.66f);
        for (int i = 7; i < 23; i += 5)
        {
            anim.add_bone_rot_frame(i, 0, q1);
            anim.add_bone_rot_frame(i + 1, 0, q2);
            anim.add_bone_rot_frame(i + 2, 0, q3);
        }
    }
    else
    {
        nya_math::quat identity;
        for (int i = 0; i < anim.get_bones_count(); ++i)
            anim.add_bone_rot_frame(i, 0, identity);
    }
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
