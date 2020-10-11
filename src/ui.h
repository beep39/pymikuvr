//

#pragma once

#include "transform.h"
#include "scene.h"
#include "render/vbo.h"
#include "scene/material.h"

struct ImGuiContext;
struct ImFontAtlas;

class ui: public container<ui>, private scene::iobject
{
public:
    static void init();
    static bool set_font(const char *path, int size, float scale);
    static void release();

public:
    void set_enable(bool enable);
    void set_pivot(float px, float py);
    void set_size(float w, float h);
    void set_caption(const char *caption);
    void set_style(const char *style);
    void set_ignore_input(bool ignore);
    void set_background(bool enabled);

    int get_origin() const;

    ui();
    ~ui();

private:
    void draw(const char *pass);
    void update_pre(int dt);
    void update_post();
    float zorder() const;

public:
    int add_text(const char *text);
    int add_btn(const char *text, int callback);
    int add_checkbox(const char *text, int callback, bool radio);
    int add_slider(const char *text, int callback, float f, float t);
    int add_coloredit(const char *text, int callback, bool alpha);
    int add_dropdown(int callback, const char ** values, int count);
    int add_listbox(int callback, const char ** values, int count);
    int add_tab(const char *text);
    int add_spacing(bool separator);
    int add_hlayout(int count);
    int add_scroll(int count);

    void set_text(int idx, const char *text);
    bool get_bool(int idx);
    void set_bool(int idx, bool b);
    float get_slider(int idx);
    void set_slider(int idx, float v);
    const char *get_list_value(int idx);
    void set_list_value(int idx, const char *v);
    int get_list_idx(int idx);
    void set_list_idx(int idx, int i);
    void get_color(int idx, float *r, float *g, float *b, float *a);
    void set_color(int idx, float r, float g, float b, float a);
    void set_list(int idx, const char ** values, int count);

private:
    enum widget_type
    {
        widget_label,
        widget_btn,
        widget_checkbox,
        widget_radio,
        widget_slider,
        widget_coloredit3,
        widget_coloredit4,
        widget_dropdown,
        widget_listbox,
        widget_tab,
        widget_separator,
        widget_spacing,
        widget_hlayout,
        widget_scroll,
    };

    struct widget
    {
        widget_type type;
        std::string label;
        int callback;

        union
        {
            bool boolean;
            struct { float v, f, t; } slider;
            float color[4];
            uint32_t count;
            int selected;
        };

        std::vector<std::string> list;

        widget(widget_type type, const char *label = 0, int callback = -1)
        {
            this->type = type;
            if (label)
                this->label = label;
            this->callback = callback;
        }
    };
    std::vector<widget> m_widgets;

    int add_widget(const widget &w)
    {
        m_widgets.push_back(w);
        return (int)m_widgets.size() - 1;
    }

private:
    float m_px = 0.0f, m_py = 0.0f;
    float m_w = 0.0f, m_h = 0.0f;
    std::string m_caption;
    bool m_no_background = false;

private:
    bool m_visible = true;
    bool m_draw = true;
    int m_origin;
    float m_zorder = 0;
    transform *m_torigin;
    bool m_ignore_input = false;

private:
    ImGuiContext *m_context;
    static ImGuiContext *m_main_context;
    static ImFontAtlas* m_font_atlas;

private:
    bool m_draw_pointer = false;

    static nya_math::vec3 m_pointer_from;
    static nya_math::vec3 m_pointer_to;
    static ui *m_active_ui;
    static float m_active_ui_dist;

    struct pointer_vert
    {
        nya_math::vec3 pos;
        uint8_t color[4];
    };
    static pointer_vert m_pointer_verts[2];

private:
    static nya_scene::material m_material;
    static nya_render::texture m_font_texture;
    static std::vector<nya_render::vbo> m_meshes[2];
    static nya_render::vbo m_pointer_mesh[2];
    static int m_mesh_chain_idx;
    static int m_mesh_idx;
};
