//

#pragma once

#include "singleton.h"
#include "transform.h"
#include <map>

class player: public singleton<player>
{
public:
    transform *origin() const { return m_torigin; }
    transform *head() const { return m_thead; }
    transform *lhand() const { return m_lhand; }
    transform *rhand() const { return m_rhand; }

    int get_transform(const char *name) const;

    int add_tracker(const char *name);
    int get_trackers_count() const;
    const char *get_tracker_name(int idx) const;
    int get_tracker(int idx) const;

public: //todo
    player();

private:
    int m_origin, m_head, m_left_hand, m_right_hand;
    transform *m_torigin, *m_thead, *m_lhand, *m_rhand;
    std::vector<std::pair<std::string, int>> m_trackers;
};
