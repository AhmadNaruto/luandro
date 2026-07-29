// native/lexsoup/element.h
// Phase 5: LexSoup Engine

#pragma once

#include <handle_manager/handle.h>
#include <lexbor/html/html.h>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace nrp::lexsoup {

class Element {
public:
    static constexpr uint16_t type_tag = 0x0102;

    Element(lxb_dom_element_t* el, Handle parent_doc)
        : element_ptr(el), parent_doc_handle(parent_doc) {}
    ~Element() = default;

    lxb_dom_element_t* raw_element() const noexcept { return element_ptr; }
    Handle parent_doc() const noexcept { return parent_doc_handle; }

    std::string tagName();
    std::string attr(const std::string& key);
    Element& attr(const std::string& key, const std::string& value);
    bool hasAttr(const std::string& key);
    Element& removeAttr(const std::string& key);
    std::map<std::string, std::string> attributes();
    std::string id();
    std::string className();
    std::set<std::string> classNames();
    bool hasClass(const std::string& cls);

    std::string text();
    std::string ownText();
    std::string html();
    std::string outerHtml();
    std::string innerHTML();

    Handle parent();
    Handle children();
    Handle child(int index);
    int childrenSize();
    Handle firstElementChild();
    Handle lastElementChild();
    Handle nextElementSibling();
    Handle previousElementSibling();
    Handle siblingElements();

    Handle select(const std::string& cssQuery);
    Handle selectFirst(const std::string& cssQuery);
    bool is(const std::string& cssQuery);
    Handle closest(const std::string& cssQuery);

    Element& text(const std::string& value);
    Element& html(const std::string& value);
    Element& append(const std::string& html);
    Element& prepend(const std::string& html);
    Element& after(const std::string& html);
    Element& before(const std::string& html);
    void remove();
    Element& wrap(const std::string& html);
    Handle unwrap();

private:
    lxb_dom_element_t* element_ptr;
    Handle parent_doc_handle;
};

} // namespace nrp::lexsoup
