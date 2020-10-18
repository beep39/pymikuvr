//

#pragma once

#include <vector>
#include <list>
#include <memory>

template<typename t> class container
{
public:
    static int add()
    {
        auto &f = get_free();
        if (f.empty())
        {
            get_list().push_back(std::shared_ptr<t>(new t()));
            return (int)get_list().size() - 1;
        }

        int idx = f.back();
        f.pop_back();
        get_list()[idx] = std::shared_ptr<t>(new t());
        return idx;
    }

    static t *get(int idx) { return get_list()[idx].get(); }

    static std::shared_ptr<t> get_shared(int idx) { return get_list()[idx]; }
    static std::weak_ptr<t> get_weak(int idx) { return get_list()[idx]; }

    static int get_idx(t *obj)
    {
        int idx = 0;
        for (auto &o: get_list())
        {
            if (o.get() == obj)
                return idx;
            ++idx;
        }
        return -1;
    }

    static void remove(int idx)
    {
        get_list()[idx].reset();
        get_free().push_back(idx);
    }

protected:
    container() {}

public:
    container(container const&) = delete;
    void operator=(container const&) = delete;

private:
    static std::vector<std::shared_ptr<t>> &get_list() { static std::vector<std::shared_ptr<t>> l; return l; }
    static std::list<int> &get_free() { static std::list<int> f; return f; }
};
