// native/regex/pattern.cpp
// Phase 6: Regex Engine Pattern

#include "pattern.h"
#include "matcher.h"
#include "match_result.h"
#include <runtime.h>
#include <exceptions/exception_manager.h>
#include <libregexp/libregexp.h>
#include <libregexp/cutils.h>

namespace nrp::regex {

// Define required libregexp glue functions if they are not already defined elsewhere.
// Wait, we defined them in jsregexp.c but standalone tests won't link jsregexp.c!
// So let's define them in a way that doesn't conflict, e.g. under weak linkage or check if we can declare them.
// Actually, since they are standard functions called by libregexp, defining them here is safe!
#ifndef LRE_ALLOC_DEFINED
#define LRE_ALLOC_DEFINED
extern "C" {
void *lre_realloc(void *opaque, void *ptr, size_t size) {
    if (size == 0) {
        std::free(ptr);
        return nullptr;
    }
    return std::realloc(ptr, size);
}

int lre_check_stack_overflow(void *opaque, size_t alloca_size) {
    return 0;
}

int lre_check_timeout(void *opaque) {
    return 0;
}
}
#endif

Pattern::~Pattern() {
    close();
}

Handle Pattern::matcher(Handle self_handle, const std::string& input) {
    if (is_closed) throw NrpException("Pattern is closed");
    auto matcher_obj = std::make_unique<Matcher>(self_handle, input);
    Handle h = Runtime::get().handles().allocate(0x0202);
    Runtime::get().objects().insert<Matcher>(h, std::move(matcher_obj));
    track_child(h);
    return h;
}

bool Pattern::matches(const std::string& input) {
    if (is_closed) throw NrpException("Pattern is closed");
    Handle matcher_h = matcher(reinterpret_cast<Handle>(this), input);
    auto* m = Runtime::get().objects().get<Matcher>(matcher_h);
    bool result = m->matches();
    Runtime::get().objects().destroy(matcher_h);
    return result;
}

Handle Pattern::find(Handle self_handle, const std::string& input) {
    if (is_closed) throw NrpException("Pattern is closed");
    Handle matcher_h = matcher(self_handle, input);
    auto* m = Runtime::get().objects().get<Matcher>(matcher_h);
    if (m->find()) {
        Handle res_h = m->toMatchResult(matcher_h);
        Runtime::get().objects().destroy(matcher_h);
        return res_h;
    }
    Runtime::get().objects().destroy(matcher_h);
    return kInvalidHandle;
}

std::vector<Handle> Pattern::findAll(Handle self_handle, const std::string& input) {
    if (is_closed) throw NrpException("Pattern is closed");
    std::vector<Handle> results;
    Handle matcher_h = matcher(self_handle, input);
    auto* m = Runtime::get().objects().get<Matcher>(matcher_h);
    while (m->find()) {
        Handle res_h = m->toMatchResult(matcher_h);
        results.push_back(res_h);
    }
    Runtime::get().objects().destroy(matcher_h);
    return results;
}

std::string Pattern::replace(const std::string& input, const std::string& replacement) {
    if (is_closed) throw NrpException("Pattern is closed");
    Handle matcher_h = matcher(reinterpret_cast<Handle>(this), input);
    auto* m = Runtime::get().objects().get<Matcher>(matcher_h);
    std::string result = m->replaceFirst(replacement);
    Runtime::get().objects().destroy(matcher_h);
    return result;
}

std::string Pattern::replaceAll(const std::string& input, const std::string& replacement) {
    if (is_closed) throw NrpException("Pattern is closed");
    Handle matcher_h = matcher(reinterpret_cast<Handle>(this), input);
    auto* m = Runtime::get().objects().get<Matcher>(matcher_h);
    std::string result = m->replaceAll(replacement);
    Runtime::get().objects().destroy(matcher_h);
    return result;
}

std::vector<std::string> Pattern::split(const std::string& input) {
    if (is_closed) throw NrpException("Pattern is closed");
    std::vector<std::string> result;
    Handle matcher_h = matcher(reinterpret_cast<Handle>(this), input);
    auto* m = Runtime::get().objects().get<Matcher>(matcher_h);
    int last_copied = 0;
    while (m->find()) {
        int s = m->start();
        result.push_back(input.substr(last_copied, s - last_copied));
        last_copied = m->end();
    }
    result.push_back(input.substr(last_copied));
    Runtime::get().objects().destroy(matcher_h);
    return result;
}

void Pattern::close() {
    if (is_closed) return;
    is_closed = true;

    // Destroy children Matchers
    auto children = child_handles;
    for (Handle h : children) {
        try {
            Runtime::get().objects().destroy(h);
        } catch (...) {}
    }
    child_handles.clear();

    if (bc) {
        lre_realloc(nullptr, bc, 0);
        bc = nullptr;
    }
}

void Pattern::track_child(Handle h) {
    child_handles.insert(h);
}

void Pattern::untrack_child(Handle h) {
    child_handles.erase(h);
}

} // namespace nrp::regex
