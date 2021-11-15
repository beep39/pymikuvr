//

#include "ui.h"
#include "player.h"
#include "sys.h"
#include "scene/camera.h"
#include "imgui.h"

#ifdef _WIN32
  #include "windows.h"
  #include <sstream>
#endif

nya_scene::material ui::m_material;
nya_render::texture ui::m_font_texture;
ImGuiContext *ui::m_main_context;
ImFontAtlas* ui::m_font_atlas;
ui *ui::m_active_ui = 0;
float ui::m_active_ui_dist;
nya_math::vec3 ui::m_pointer_from;
nya_math::vec3 ui::m_pointer_to;

std::vector<nya_render::vbo> ui::m_meshes[2];
nya_render::vbo ui::m_pointer_mesh[2];
int ui::m_mesh_chain_idx = 0;
int ui::m_mesh_idx = 0;
ui::pointer_vert ui::m_pointer_verts[2];

const float ui_scale = 1000.0f;

void ui::init()
{
    IMGUI_CHECKVERSION();
    m_font_atlas = new ImFontAtlas();
    m_main_context = ImGui::CreateContext(m_font_atlas);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "nya_render";

    m_material.load("materials/ui.txt");

    for (auto &m: m_pointer_mesh)
    {
        m.set_vertices(0, 3);
        m.set_colors(3 * sizeof(float), 4, nya_render::vbo::uint8);
        m.set_element_type(nya_render::vbo::lines);
    }

    m_pointer_verts[0].color[0] = m_pointer_verts[1].color[0] = 0xff;
    m_pointer_verts[0].color[1] = m_pointer_verts[1].color[1] = 0;
    m_pointer_verts[0].color[2] = m_pointer_verts[1].color[2] = 0;
    m_pointer_verts[0].color[3] = 0;
    m_pointer_verts[1].color[3] = 0xff;

    io.IniFilename = 0;
    ImGui::SetCurrentContext(0);
}

bool ui::set_font(const char *path_, int size, float scale, const char *additional_gliphs)
{
    auto path = sys::instance().get_path(path_);
    if (path.empty())
        return false;

    ImGui::SetCurrentContext(m_main_context);
    auto& io = ImGui::GetIO();
    m_font_atlas->Clear();

    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    if (additional_gliphs && additional_gliphs[0])
        builder.AddText(additional_gliphs);
    ImVector<ImWchar> ranges;
    builder.BuildRanges(&ranges);
    auto font = io.Fonts->AddFontFromFileTTF(path.c_str(), size, NULL, ranges.begin());
    if (!font)
    {
        ImGui::SetCurrentContext(0);
        return false;
    }

    font->Scale = scale;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    m_font_texture.build_texture(pixels, width, height, nya_render::texture::color_rgba);
    io.Fonts->TexID = &m_font_texture;
    ImGui::SetCurrentContext(0);
    return true;
}

void ui::release()
{
    ImGui::DestroyContext(m_main_context);
    m_font_texture.release();
    for (auto &mm: m_meshes)
        for (auto &m: mm)
            m.release();
    m_material = nya_scene::material();
}

void ui::set_enable(bool enable) { m_visible = enable; }

int ui::get_origin() const { return m_origin; }

void ui::set_pivot(float px, float py)
{
    m_px = px;
    m_py = py;
}

void ui::set_size(float w, float h)
{
    m_w = w;
    m_h = h;
}

void ui::set_caption(const char *caption) { m_caption = caption; }

void ui::set_style(const char *style)
{
    ImGui::SetCurrentContext(m_context);
    if (strcmp(style, "dark") == 0)
        ImGui::StyleColorsDark();
    if (strcmp(style, "light") == 0)
        ImGui::StyleColorsLight();
    if (strcmp(style, "classic") == 0)
        ImGui::StyleColorsClassic();
}

void ui::set_ignore_input(bool ignore)
{
    m_ignore_input = ignore;
    if (ignore)
        m_draw_pointer = false;
}

void ui::set_background(bool enabled) { m_no_background = !enabled; }

void ui::update_pre(int dt)
{
    if (!m_visible)
    {
        m_draw = false;
        return;
    }

    const auto vdir = m_torigin->get_pos() - nya_scene::get_camera().get_pos();
    m_draw = m_torigin->get_rot().rotate(nya_math::vec3::forward()).dot(vdir) > 0;
    if (!m_draw)
        return;

    m_zorder = vdir.length();

    ImGui::SetCurrentContext(m_context);
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = (dt < 1 ? 1 : dt) * 0.001f;

    ImGui::NewFrame();

    auto window_size = ImVec2(m_w * ui_scale, m_h * ui_scale);

    io.DisplaySize = ImVec2(m_w * ui_scale * (1.0f + fabsf(m_px)), m_h * ui_scale * (1.0f + fabsf(m_py)));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(m_px, m_py));
    ImGui::SetNextWindowSize(window_size);

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (m_no_background)
        flags |= ImGuiWindowFlags_NoBackground;
    if (m_caption.empty())
        ImGui::Begin(" ", NULL, flags | ImGuiWindowFlags_NoTitleBar);
    else
        ImGui::Begin(m_caption.c_str(), NULL, flags);
    
    auto top_left = ImGui::GetCursorPos();

    int hlayout = 0;
    bool hlayout_started = false;
    int scroll = 0;
    int idx = 0;
    bool tab = false;
    bool skip_tab = false;
    for (auto &w: m_widgets)
    {
        if (skip_tab && w.type != widget_tab)
            continue;
        
        if (hlayout > 0)
        {
            if (!hlayout_started)
                hlayout_started = true;
            else
                ImGui::SameLine();
            --hlayout;
        }

        switch (w.type)
        {
            case widget_label: ImGui::Text("%s", w.label.c_str()); break; //ToDo: TextColored(ImVec4(), "")
            case widget_btn:
            {
                if (ImGui::Button(w.label.c_str()))
                {
                    if (!w.boolean)
                    {
                        w.boolean = true;
                        sys::push_callback(w.callback);
                    }
                }
                else
                    w.boolean = false;
                break;
            }
            case widget_checkbox:
            {
                bool value = w.boolean;
                ImGui::Checkbox(w.label.c_str(), &value);
                if (w.boolean != value)
                {
                    w.boolean = value;
                    sys::push_callback(w.callback, value);
                }
                break;
            }
            case widget_radio:
            {
                const bool value = ImGui::RadioButton(w.label.c_str(), w.boolean);
                if (w.boolean != value)
                {
                    w.boolean = value;
                    sys::push_callback(w.callback, value);
                }
                break;
            }
            case widget_slider:
            {
                float value = w.slider.v;
                ImGui::SliderFloat(w.label.c_str(), &value, w.slider.f, w.slider.t);
                if (w.slider.v != value)
                {
                    w.slider.v = value;
                    sys::push_callback(w.callback, value);
                }
                break;
            }
            case widget_coloredit3: ImGui::ColorEdit3(w.label.c_str(), w.color); break;
            case widget_coloredit4: ImGui::ColorEdit4(w.label.c_str(), w.color); break;
            case widget_separator: ImGui::Separator(); break;
            case widget_spacing: ImGui::Dummy(ImVec2(16,16)); break;
            case widget_hlayout: hlayout = w.count; hlayout_started = false; break;
            case widget_dropdown:
            {
                if (ImGui::BeginCombo(w.label.c_str(), w.list[w.selected].c_str()))
                {
                    for (int i = 0; i < (int)w.list.size(); ++i)
                    {
                        const bool is_selected = (w.selected == i);
                        if (ImGui::Selectable(w.list[i].c_str(), is_selected))
                            w.selected = i;
                    }
                    ImGui::EndCombo();
                }
                break;
            }
            case widget_listbox:
            {
                const auto p = ImGui::GetCursorPos();
                const auto size = ImVec2(window_size.x - top_left.x * 2, window_size.y - top_left.y - p.y);
                if (ImGui::ListBoxHeader(w.label.c_str(), size))
                {
                    for (int i = 0; i < (int)w.list.size(); ++i)
                    {
                        const bool is_selected = (w.selected == i);
                        if (ImGui::Selectable(w.list[i].c_str(), is_selected))
                            w.selected = i;

                        if (w.selected == i)
                        {
                            auto p0 = ImGui::GetItemRectMin();
                            auto p1 = ImGui::GetItemRectMax();
                            if (!ImGui::IsRectVisible(ImVec2(p0.x, p1.y - 1), p1) ||
                                !ImGui::IsRectVisible(p0, ImVec2(p1.x, p0.y + 1)))
                                ImGui::SetScrollHereY();
                        }
                    }
                    ImGui::ListBoxFooter();
                }
                break;
            }
            case widget_scroll:
            {
                scroll = w.count;
                ImGui::BeginChild(("scroll" + std::to_string(idx)).c_str());
                break;
            }
            case widget_tab:
            {
                if (!tab)
                {
                    ImGui::BeginTabBar(("tab" + std::to_string(idx)).c_str());
                    tab = true;
                }
                else if (!skip_tab)
                    ImGui::EndTabItem();
                skip_tab = !ImGui::BeginTabItem(w.label.c_str());
                break;
            }
        }

        if (scroll > 0 && --scroll == 0)
            ImGui::EndChild();

        ++idx;
    }

    if (scroll < 0)
        ImGui::EndChild();

    if (tab)
    {
        if (!skip_tab)
            ImGui::EndTabItem();
        ImGui::EndTabBar();
    }

    auto size = ImGui::GetWindowSize();
    ImGui::End();

    m_aabb.origin = nya_math::vec3::zero();
    m_aabb.delta = nya_math::vec3(size.x / ui_scale * 0.5f, 0.0f, size.y / ui_scale * 0.5f);
    m_aabb = nya_math::aabb(m_aabb, m_torigin->get_pos(), m_torigin->get_rot(), nya_math::vec3(1.0f, 1.0f, 1.0f));

    if (m_ignore_input)
        return;

    m_draw_pointer = false;

    auto from = player::instance().rhand()->get_pos();
    from = m_torigin->get_rot().rotate_inv(from - m_torigin->get_pos());
    if (from.z < 0)
        return;

    auto dir = player::instance().rhand()->get_rot().rotate(nya_math::vec3::forward());
    dir = m_torigin->get_rot().rotate_inv(dir);

    const float t = -from.z / dir.z;
    if (m_active_ui && t > m_active_ui_dist)
        return;

    auto to_ = from + dir * t;
    auto to = to_ * ui_scale;
    if (to.x < size.x * (1.0f - m_px) && to.x > -size.x * m_px
        && to.y < size.y * m_py && to.y > size.y * (m_py - 1.0f))
    {
        m_active_ui = this;
        m_active_ui_dist = t;
        m_pointer_from = from;
        m_pointer_to = to_;
    }
}

void ui::update_post()
{
    if (!m_draw)
        return;

    ImGui::SetCurrentContext(m_context);
    ImGuiIO& io = ImGui::GetIO();

    m_mesh_idx = 0;

    if (this != ui::m_active_ui)
    {
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        io.MouseDrawCursor = false;
        io.MouseDown[0] = false;
        ImGui::Render();
        return;
    }
    ui::m_active_ui = 0;

    io.MousePos.x = m_pointer_to.x * ui_scale + io.DisplaySize.x * 0.5f;
    io.MousePos.y = m_pointer_to.y * -ui_scale + io.DisplaySize.y * 0.5f;
    io.MouseDrawCursor = true;

    sys::instance().block_input(true, false);
    float sx, sy, tx, ty, trigger, grip;
    sys::instance().get_ctrl(true, &sx, &sy, &tx, &ty, &trigger, &grip);
    io.MouseDown[0] = trigger > 0.5f;
    sys::instance().block_input(true, true);

    m_draw_pointer = true;
    m_pointer_verts[0].pos = m_pointer_from;
    m_pointer_verts[1].pos = m_pointer_to;

    ImGui::Render();
}

void ui::draw(const char *pass)
{
    if (!m_draw)
        return;

    ImGui::SetCurrentContext(m_context);
    auto *draw_data = ImGui::GetDrawData();
    if (!draw_data)
        return;

    nya_math::mat4 mat=nya_scene::get_camera().get_view_matrix();
    mat.translate(m_torigin->get_pos()).rotate(m_torigin->get_rot());
    nya_math::mat4 ponter_mat = mat;
    mat.scale(1.0f / ui_scale, -1.0f / ui_scale, 1.0f);
    mat.translate(-draw_data->DisplaySize.x * 0.5f, -draw_data->DisplaySize.y * 0.5f, 0);

    nya_render::set_modelview_matrix(mat);

    m_material.internal().set(pass);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        if (m_mesh_idx == 0)
            m_mesh_chain_idx = (m_mesh_chain_idx + 1) % (sizeof(m_meshes) / sizeof(m_meshes[0]));
        auto &meshes = m_meshes[m_mesh_chain_idx];
        if (m_mesh_idx >= (int)meshes.size())
        {
            meshes.resize(m_mesh_idx + 1);
            auto &mesh = meshes[m_mesh_idx];
            mesh.set_vertices(IM_OFFSETOF(ImDrawVert, pos), 2);
            mesh.set_tc(0, IM_OFFSETOF(ImDrawVert, uv), 2);
            mesh.set_colors(IM_OFFSETOF(ImDrawVert, col), 4, nya_render::vbo::uint8);
        }
        auto &mesh = meshes[m_mesh_idx++];

        mesh.set_vertex_data(cmd_list->VtxBuffer.Data, (int)sizeof(ImDrawVert), cmd_list->VtxBuffer.Size, nya_render::vbo::stream_draw);
        mesh.set_index_data(cmd_list->IdxBuffer.Data, sizeof(ImDrawIdx) == 2 ? nya_render::vbo::index2b : nya_render::vbo::index4b, cmd_list->IdxBuffer.Size, nya_render::vbo::stream_draw);
        mesh.bind();

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    m_material.internal().set(pass);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                m_material.set_param("clip", pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);
                m_material.internal().set(pass);

                if (pcmd->TextureId == 0)
                    scene::instance().white_texture().internal().set();
                else
                    ((nya_render::texture *)pcmd->TextureId)->bind(0);

                mesh.draw(pcmd->IdxOffset, pcmd->ElemCount);
            }
        }
        mesh.unbind();
    }

    if (m_draw_pointer)
    {
        m_material.set_param("clip", -FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
        m_material.internal().set(pass);

        nya_render::set_modelview_matrix(ponter_mat);
        scene::instance().white_texture().internal().set();
        auto &mesh = m_pointer_mesh[m_mesh_chain_idx];
        mesh.set_vertex_data(m_pointer_verts, int(sizeof(m_pointer_verts[0])),
                             int(sizeof(m_pointer_verts) / sizeof(m_pointer_verts[0])), nya_render::vbo::stream_draw);
        mesh.bind();
        mesh.draw();
        mesh.unbind();
    }
    nya_render::texture::unbind(0);
    m_material.internal().unset();

    ImGui::SetCurrentContext(0);
}

int ui::add_text(const char *text)
{
    return add_widget(widget(widget_label, text));
}

int ui::add_btn(const char *text, int callback)
{
    return add_widget(widget(widget_btn, text, callback));
}

int ui::add_checkbox(const char *text, int callback, bool radio)
{
    return add_widget(widget(radio ? widget_radio : widget_checkbox, text, callback));
}

int ui::add_slider(const char *text, int callback, float f, float t)
{
    auto w = widget(widget_slider, text, callback);
    w.slider.v = 0.0f;
    w.slider.f = f;
    w.slider.t = t;
    return add_widget(w);
}

int ui::add_coloredit(const char *text, int callback, bool alpha)
{
    auto w = widget(alpha ? widget_coloredit4 : widget_coloredit3, text, callback);
    w.color[0] = w.color[1] = w.color[2] = w.color[3] = 1.0f;
    return add_widget(w);
}

int ui::add_dropdown(int callback, const char ** values, int count)
{
    auto w = widget(widget_dropdown);
    w.list.resize(count);
    for (int i = 0; i < count; ++i)
        w.list[i] = values[i];
    w.selected = 0;
    return add_widget(w);
}

int ui::add_listbox(int callback, const char ** values, int count)
{
    auto w = widget(widget_listbox);
    w.list.resize(count);
    for (int i = 0; i < count; ++i)
        w.list[i] = values[i];
    w.selected = 0;
    return add_widget(w);
}

int ui::add_tab(const char *text)
{
    return add_widget(widget(widget_tab, text));
}

int ui::add_spacing(bool separator)
{
    return add_widget(widget(separator ? widget_separator : widget_spacing));
}

int ui::add_hlayout(int count)
{
    auto w = widget(widget_hlayout);
    w.count = count;
    return add_widget(w);
}

int ui::add_scroll(int count)
{
    auto w = widget(widget_scroll);
    w.count = count;
    return add_widget(w);
}

void ui::set_text(int idx, const char *text) { m_widgets[idx].label = text; }
bool ui::get_bool(int idx) { return m_widgets[idx].boolean; }
void ui::set_bool(int idx, bool v) { m_widgets[idx].boolean = v; }
float ui::get_slider(int idx) { return m_widgets[idx].slider.v; }
void ui::set_slider(int idx, float v) { m_widgets[idx].slider.v = v; }

const char *ui::get_list_value(int idx)
{
    auto &w = m_widgets[idx];
    auto s = w.selected;
    if (s < 0 || s >= (int)w.list.size())
        return "";

    return w.list[s].c_str();
}

void ui::set_list_value(int idx, const char *v)
{
    auto &w = m_widgets[idx];
    for (int i = 0; i < (int)w.list.size(); ++i)
    {
        if (w.list[i] == v)
        {
            w.selected = i;
            break;
        }
    }
}

int ui::get_list_idx(int idx) { return m_widgets[idx].selected; }
void ui::set_list_idx(int idx, int i) { m_widgets[idx].selected = i; }

void ui::get_color(int idx, float *r, float *g, float *b, float *a)
{
    auto &w = m_widgets[idx];
    *r = w.color[0];
    *g = w.color[1];
    *b = w.color[2];
    *a = w.color[3];
}

void ui::set_color(int idx, float r, float g, float b, float a)
{
    auto &w = m_widgets[idx];
    w.color[0] = r;
    w.color[1] = g;
    w.color[2] = b;
    if (w.type == widget_coloredit4)
        w.color[3] = a;
}

void ui::set_list(int idx, const char ** values, int count)
{
    auto &w = m_widgets[idx];
    w.list.resize(count);
    for (int i = 0; i < count; ++i)
        w.list[i] = values[i];
    w.selected = 0;
}

ui::ui()
{
    if (!m_font_texture.get_width())
    {
        ImGui::SetCurrentContext(m_main_context);
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        m_font_texture.build_texture(pixels, width, height, nya_render::texture::color_rgba);
        io.Fonts->TexID = &m_font_texture;
    }

    scene::instance().reg_object(this);
    m_torigin = transform::get((m_origin = transform::add()));

    m_context = ImGui::CreateContext(m_font_atlas);
    ImGui::SetCurrentContext(m_context);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1000, 1000);
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.IniFilename = 0;
}

ui::~ui()
{
    scene::instance().unreg_object(this);
    ImGui::DestroyContext(m_context);
}
