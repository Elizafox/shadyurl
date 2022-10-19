#ifndef SQLITE_HELPER_H
#define SQLITE_HELPER_H

#include <sqlite3.h>
#include <memory>

namespace sqlite_helper
{

template <typename Func> struct scope_exit {
        explicit scope_exit(Func f) : f_{f} {}
        ~scope_exit() { f_(); }
private:
        Func f_;
};

struct sqlite3_handle_deleter
{
        void operator () (sqlite3* db) const { sqlite3_close(db); }
};

using sqlite3_handle = std::unique_ptr<sqlite3, sqlite3_handle_deleter>;

static inline auto
make_sqlite3_handle(char const* db_name)
{
        sqlite3* p;
        int rc = sqlite3_open(db_name, &p);
        sqlite3_handle h{p};
        if(rc)
		h.reset();
        return h;
}

} // namespace sqlite_helper

#endif // SQLITE_HELPER_H
