//

#include "player.h"
#include "transform.h"
#include <string>

player::player()
{
    m_torigin = transform::get((m_origin = transform::add()));

    m_thead = transform::get((m_head = transform::add()));
    m_thead->set_pos(nya_math::vec3(0.0f, 1.5f, 0.0f));
    m_thead->set_parent(m_torigin);

    m_lhand = transform::get((m_left_hand = transform::add()));
    m_lhand->set_pos(nya_math::vec3(-0.2f, 1.2f, -0.2f));
    m_lhand->set_parent(m_torigin);

    m_rhand = transform::get((m_right_hand = transform::add()));
    m_rhand->set_pos(nya_math::vec3(0.2f, 1.2f, -0.2f));
    m_rhand->set_parent(m_torigin);
}

int player::get_transform(const char *name) const
{
    std::string sname = name;
    if (sname == "origin")
        return m_origin;
    if (sname == "head")
        return m_head;
    if (sname == "lhand")
        return m_left_hand;
    if (sname == "rhand")
        return m_right_hand;
    for(auto &t: m_trackers)
    {
        if (t.first == sname)
            return t.second;
    }
    return -1;
}

int player::add_tracker(const char *name)
{
    std::string sname = name;
    for (auto &t: m_trackers)
    {
        if (t.first == sname)
            return t.second;
    }
    const int r = transform::add();
    transform::get(r)->set_parent(m_torigin);
    m_trackers.push_back(std::make_pair(sname, r));
    return r;
}

int player::get_trackers_count() const { return (int)m_trackers.size(); }
const char *player::get_tracker_name(int idx) const { return m_trackers[idx].first.c_str(); }
int player::get_tracker(int idx) const { return m_trackers[idx].second; }
