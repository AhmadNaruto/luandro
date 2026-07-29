// native/lexsoup/document.h
// Phase 5: LexSoup Engine

#pragma once

#include <handle_manager/handle.h>
#include <lexbor/html/html.h>
#include <string>
#include <unordered_set>

namespace nrp::lexsoup {

class Document {
public:
    static constexpr uint16_t type_tag = 0x0101;

    explicit Document(lxb_html_document_t* doc) : document_ptr(doc) {}
    ~Document();

    lxb_html_document_t* raw_document() const noexcept { return document_ptr; }

    std::string title();
    Handle body(Handle self_handle);
    Handle head(Handle self_handle);
    Handle select(Handle self_handle, const std::string& cssQuery);
    Handle getElementById(Handle self_handle, const std::string& id);
    Handle getElementsByTag(Handle self_handle, const std::string& tag);
    Handle getElementsByClass(Handle self_handle, const std::string& cls);
    std::string outerHtml();
    std::string text();

    void track_child(Handle h) { child_handles.insert(h); }
    void untrack_child(Handle h) { child_handles.erase(h); }
    void close();

private:
    lxb_html_document_t* document_ptr;
    std::unordered_set<Handle> child_handles;
};

} // namespace nrp::lexsoup
