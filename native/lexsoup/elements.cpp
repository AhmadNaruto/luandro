// native/lexsoup/elements.cpp
// Phase 5: LexSoup Engine

#include "elements.h"
#include "element.h"
#include "document.h"
#include <runtime.h>
#include <exceptions/exception_manager.h>
#include <unordered_set>

namespace nrp::lexsoup {

// External helpers from document.cpp / element.cpp
extern Handle wrap_element(lxb_dom_element_t* el, Handle doc_handle);
extern Handle run_css_select(lxb_dom_node_t* root, Handle doc_handle, const std::string& query);

Handle Elements::first() {
    if (element_handles.empty()) return kInvalidHandle;
    return element_handles.front();
}

Handle Elements::last() {
    if (element_handles.empty()) return kInvalidHandle;
    return element_handles.back();
}

Handle Elements::get(int index) {
    if (index < 0 || index >= static_cast<int>(element_handles.size())) {
        throw NrpException("Index out of bounds in Elements collection");
    }
    return element_handles[index];
}

Handle Elements::select(const std::string& cssQuery) {
    std::vector<Handle> results;
    std::unordered_set<lxb_dom_element_t*> unique_els;

    for (Handle h : element_handles) {
        auto* el = Runtime::get().objects().get<Element>(h);
        if (el) {
            Handle sub_els_h = run_css_select(lxb_dom_interface_node(el->raw_element()), parent_doc_handle, cssQuery);
            auto* sub_els = Runtime::get().objects().get<Elements>(sub_els_h);
            if (sub_els) {
                for (Handle sh : sub_els->handles()) {
                    auto* sel = Runtime::get().objects().get<Element>(sh);
                    if (sel && unique_els.insert(sel->raw_element()).second) {
                        results.push_back(sh);
                    }
                }
                Runtime::get().objects().destroy(sub_els_h);
            }
        }
    }

    auto wrapped = std::make_unique<Elements>(results, parent_doc_handle);
    Handle final_h = Runtime::get().handles().allocate(0x0103);
    Runtime::get().objects().insert<Elements>(final_h, std::move(wrapped));
    if (parent_doc_handle != kInvalidHandle) {
        auto* doc = Runtime::get().objects().get<Document>(parent_doc_handle);
        if (doc) {
            doc->track_child(final_h);
        }
    }
    return final_h;
}

std::string Elements::attr(const std::string& key) {
    for (Handle h : element_handles) {
        auto* el = Runtime::get().objects().get<Element>(h);
        if (el && el->hasAttr(key)) {
            return el->attr(key);
        }
    }
    return "";
}

Elements& Elements::attr(const std::string& key, const std::string& value) {
    for (Handle h : element_handles) {
        auto* el = Runtime::get().objects().get<Element>(h);
        if (el) {
            el->attr(key, value);
        }
    }
    return *this;
}

bool Elements::hasAttr(const std::string& key) {
    for (Handle h : element_handles) {
        auto* el = Runtime::get().objects().get<Element>(h);
        if (el && el->hasAttr(key)) {
            return true;
        }
    }
    return false;
}

std::string Elements::text() {
    std::string accumulated;
    for (size_t i = 0; i < element_handles.size(); ++i) {
        auto* el = Runtime::get().objects().get<Element>(element_handles[i]);
        if (el) {
            accumulated += el->text();
            if (i + 1 < element_handles.size()) {
                accumulated += " ";
            }
        }
    }
    return accumulated;
}

std::string Elements::outerHtml() {
    std::string accumulated;
    for (Handle h : element_handles) {
        auto* el = Runtime::get().objects().get<Element>(h);
        if (el) {
            accumulated += el->outerHtml();
        }
    }
    return accumulated;
}

} // namespace nrp::lexsoup
