// native/lexsoup/elements.h
// Phase 5: LexSoup Engine

#pragma once

#include <handle_manager/handle.h>
#include <vector>
#include <string>

namespace nrp::lexsoup {

class Elements {
public:
    static constexpr uint16_t type_tag = 0x0103;

    Elements(const std::vector<Handle>& els, Handle parent_doc)
        : element_handles(els), parent_doc_handle(parent_doc) {}
    ~Elements() = default;

    const std::vector<Handle>& handles() const noexcept { return element_handles; }
    Handle parent_doc() const noexcept { return parent_doc_handle; }

    int size() const noexcept { return static_cast<int>(element_handles.size()); }
    bool isEmpty() const noexcept { return element_handles.empty(); }
    Handle first();
    Handle last();
    Handle get(int index);
    Handle select(const std::string& cssQuery);
    std::string attr(const std::string& key);
    Elements& attr(const std::string& key, const std::string& value);
    bool hasAttr(const std::string& key);
    std::string text();
    std::string outerHtml();

private:
    std::vector<Handle> element_handles;
    Handle parent_doc_handle;
};

} // namespace nrp::lexsoup
