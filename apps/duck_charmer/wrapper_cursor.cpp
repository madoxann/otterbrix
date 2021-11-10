#include "wrapper_cursor.hpp"
#include "convert.hpp"
#include "route.hpp"
#include <storage/result.hpp>

// The bug related to the use of RTTI by the pybind11 library has been fixed: a
// declaration should be in each translation unit.
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::intrusive_ptr<T>)

wrapper_cursor::wrapper_cursor(goblin_engineer::actor_address dispatcher, components::session::session_t session, wrapper_cursor::pointer cursor)
        : session_(session)
        , ptr_(cursor)
        , dispatcher_(dispatcher) {
}
void wrapper_cursor::close() {
    close_ = true;
    goblin_engineer::send(
        dispatcher_,
        goblin_engineer::actor_address(),
        duck_charmer::collection::close_cursor,
        session_
                );
}

bool wrapper_cursor::has_next() {
    return ptr_->has_next();
}

bool wrapper_cursor::next() {
    return ptr_->next();
}

std::size_t wrapper_cursor::size() {
    return ptr_->size();
}

py::object wrapper_cursor::get(const std::string &key) {
    return from_object(*ptr_->get(), key);
}

std::string wrapper_cursor::print() {
    return ptr_->get()->to_json();
}

void wrapper_cursor::d_() {
    cv_.notify_all();
    i = 1;
}