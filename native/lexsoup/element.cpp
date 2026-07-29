// native/lexsoup/element.cpp
// Phase 5: LexSoup Engine

#include "element.h"
#include "document.h"
#include "elements.h"
#include <runtime.h>
#include <exceptions/exception_manager.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

namespace nrp::lexsoup {

// External helper functions from document.cpp
extern std::string get_outer_html(lxb_dom_node_t* node);
extern std::string get_node_text(lxb_dom_node_t* node);
extern Handle wrap_element(lxb_dom_element_t* el, Handle doc_handle);
extern Handle run_css_select(lxb_dom_node_t* root, Handle doc_handle, const std::string& query);

std::string get_own_text(lxb_dom_node_t* node) {
    std::string text;
    for (lxb_dom_node_t* child = node->first_child; child != nullptr; child = child->next) {
        if (child->type == LXB_DOM_NODE_TYPE_TEXT) {
            auto* ch_data = lxb_dom_interface_character_data(child);
            if (ch_data && ch_data->data.data) {
                text.append(reinterpret_cast<const char*>(ch_data->data.data), ch_data->data.length);
            }
        }
    }
    return text;
}

static lxb_status_t match_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t spec, void *ctx) {
    auto* matched = static_cast<bool*>(ctx);
    *matched = true;
    return LXB_STATUS_OK;
}

std::string Element::tagName() {
    size_t len = 0;
    const lxb_char_t* name = lxb_dom_element_local_name(element_ptr, &len);
    if (!name) return "";
    return std::string(reinterpret_cast<const char*>(name), len);
}

std::string Element::attr(const std::string& key) {
    size_t len = 0;
    const lxb_char_t* val = lxb_dom_element_get_attribute(element_ptr,
        reinterpret_cast<const lxb_char_t*>(key.c_str()), key.length(), &len);
    if (!val) return "";
    return std::string(reinterpret_cast<const char*>(val), len);
}

Element& Element::attr(const std::string& key, const std::string& value) {
    lxb_dom_element_set_attribute(element_ptr,
        reinterpret_cast<const lxb_char_t*>(key.c_str()), key.length(),
        reinterpret_cast<const lxb_char_t*>(value.c_str()), value.length());
    return *this;
}

bool Element::hasAttr(const std::string& key) {
    return lxb_dom_element_has_attribute(element_ptr,
        reinterpret_cast<const lxb_char_t*>(key.c_str()), key.length());
}

Element& Element::removeAttr(const std::string& key) {
    lxb_dom_element_remove_attribute(element_ptr,
        reinterpret_cast<const lxb_char_t*>(key.c_str()), key.length());
    return *this;
}

std::map<std::string, std::string> Element::attributes() {
    std::map<std::string, std::string> attrs;
    lxb_dom_attr_t* attr = lxb_dom_element_first_attribute(element_ptr);
    while (attr) {
        size_t name_len = 0;
        const lxb_char_t* name = lxb_dom_attr_qualified_name(attr, &name_len);
        size_t val_len = 0;
        const lxb_char_t* val = lxb_dom_attr_value(attr, &val_len);
        if (name) {
            std::string n(reinterpret_cast<const char*>(name), name_len);
            std::string v = val ? std::string(reinterpret_cast<const char*>(val), val_len) : "";
            attrs[n] = v;
        }
        attr = lxb_dom_element_next_attribute(attr);
    }
    return attrs;
}

std::string Element::id() {
    return attr("id");
}

std::string Element::className() {
    return attr("class");
}

std::set<std::string> Element::classNames() {
    std::set<std::string> names;
    std::string cls = className();
    size_t start = 0;
    while (start < cls.length()) {
        while (start < cls.length() && std::isspace(cls[start])) start++;
        if (start >= cls.length()) break;
        size_t end = start;
        while (end < cls.length() && !std::isspace(cls[end])) end++;
        names.insert(cls.substr(start, end - start));
        start = end;
    }
    return names;
}

bool Element::hasClass(const std::string& cls) {
    std::set<std::string> names = classNames();
    return names.find(cls) != names.end();
}

std::string Element::text() {
    return get_node_text(lxb_dom_interface_node(element_ptr));
}

std::string Element::ownText() {
    return get_own_text(lxb_dom_interface_node(element_ptr));
}

std::string Element::html() {
    std::string text_html;
    lxb_dom_node_t* node = lxb_dom_interface_node(element_ptr);
    for (lxb_dom_node_t* child = node->first_child; child != nullptr; child = child->next) {
        text_html += get_outer_html(child);
    }
    return text_html;
}

std::string Element::outerHtml() {
    return get_outer_html(lxb_dom_interface_node(element_ptr));
}

std::string Element::innerHTML() {
    return html();
}

Handle Element::parent() {
    lxb_dom_node_t* p = lxb_dom_interface_node(element_ptr)->parent;
    if (p && p->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        return wrap_element(lxb_dom_interface_element(p), parent_doc_handle);
    }
    return kInvalidHandle;
}

Handle Element::children() {
    std::vector<Handle> child_handles;
    lxb_dom_node_t* node = lxb_dom_interface_node(element_ptr);
    for (lxb_dom_node_t* child = node->first_child; child != nullptr; child = child->next) {
        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            child_handles.push_back(wrap_element(lxb_dom_interface_element(child), parent_doc_handle));
        }
    }
    auto els_wrapper = std::make_unique<Elements>(child_handles, parent_doc_handle);
    Handle els_h = Runtime::get().handles().allocate(0x0103);
    Runtime::get().objects().insert<Elements>(els_h, std::move(els_wrapper));
    if (parent_doc_handle != kInvalidHandle) {
        auto* doc = Runtime::get().objects().get<Document>(parent_doc_handle);
        if (doc) {
            doc->track_child(els_h);
        }
    }
    return els_h;
}

Handle Element::child(int index) {
    int idx = 0;
    lxb_dom_node_t* node = lxb_dom_interface_node(element_ptr);
    for (lxb_dom_node_t* child_node = node->first_child; child_node != nullptr; child_node = child_node->next) {
        if (child_node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            if (idx == index) {
                return wrap_element(lxb_dom_interface_element(child_node), parent_doc_handle);
            }
            idx++;
        }
    }
    throw NrpException("Index out of bounds for children elements");
}

int Element::childrenSize() {
    int count = 0;
    lxb_dom_node_t* node = lxb_dom_interface_node(element_ptr);
    for (lxb_dom_node_t* child_node = node->first_child; child_node != nullptr; child_node = child_node->next) {
        if (child_node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            count++;
        }
    }
    return count;
}

Handle Element::firstElementChild() {
    lxb_dom_node_t* node = lxb_dom_interface_node(element_ptr);
    for (lxb_dom_node_t* child_node = node->first_child; child_node != nullptr; child_node = child_node->next) {
        if (child_node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            return wrap_element(lxb_dom_interface_element(child_node), parent_doc_handle);
        }
    }
    return kInvalidHandle;
}

Handle Element::lastElementChild() {
    lxb_dom_node_t* node = lxb_dom_interface_node(element_ptr);
    for (lxb_dom_node_t* child_node = node->last_child; child_node != nullptr; child_node = child_node->prev) {
        if (child_node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            return wrap_element(lxb_dom_interface_element(child_node), parent_doc_handle);
        }
    }
    return kInvalidHandle;
}

Handle Element::nextElementSibling() {
    lxb_dom_node_t* node = lxb_dom_interface_node(element_ptr);
    for (lxb_dom_node_t* sibling = node->next; sibling != nullptr; sibling = sibling->next) {
        if (sibling->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            return wrap_element(lxb_dom_interface_element(sibling), parent_doc_handle);
        }
    }
    return kInvalidHandle;
}

Handle Element::previousElementSibling() {
    lxb_dom_node_t* node = lxb_dom_interface_node(element_ptr);
    for (lxb_dom_node_t* sibling = node->prev; sibling != nullptr; sibling = sibling->prev) {
        if (sibling->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            return wrap_element(lxb_dom_interface_element(sibling), parent_doc_handle);
        }
    }
    return kInvalidHandle;
}

Handle Element::siblingElements() {
    lxb_dom_node_t* p = lxb_dom_interface_node(element_ptr)->parent;
    if (!p) {
        auto empty_wrapper = std::make_unique<Elements>(std::vector<Handle>{}, parent_doc_handle);
        Handle empty_h = Runtime::get().handles().allocate(0x0103);
        Runtime::get().objects().insert<Elements>(empty_h, std::move(empty_wrapper));
        return empty_h;
    }
    std::vector<Handle> siblings;
    for (lxb_dom_node_t* child = p->first_child; child != nullptr; child = child->next) {
        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT && child != lxb_dom_interface_node(element_ptr)) {
            siblings.push_back(wrap_element(lxb_dom_interface_element(child), parent_doc_handle));
        }
    }
    auto els_wrapper = std::make_unique<Elements>(siblings, parent_doc_handle);
    Handle els_h = Runtime::get().handles().allocate(0x0103);
    Runtime::get().objects().insert<Elements>(els_h, std::move(els_wrapper));
    if (parent_doc_handle != kInvalidHandle) {
        auto* doc = Runtime::get().objects().get<Document>(parent_doc_handle);
        if (doc) {
            doc->track_child(els_h);
        }
    }
    return els_h;
}

Handle Element::select(const std::string& cssQuery) {
    return run_css_select(lxb_dom_interface_node(element_ptr), parent_doc_handle, cssQuery);
}

Handle Element::selectFirst(const std::string& cssQuery) {
    Handle els_handle = select(cssQuery);
    auto* els = Runtime::get().objects().get<Elements>(els_handle);
    if (els && els->size() > 0) {
        Handle el = els->first();
        Runtime::get().objects().destroy(els_handle);
        return el;
    }
    if (els_handle != kInvalidHandle) {
        Runtime::get().objects().destroy(els_handle);
    }
    return kInvalidHandle;
}

bool Element::is(const std::string& cssQuery) {
    lxb_status_t status;
    lxb_css_parser_t *parser = lxb_css_parser_create();
    status = lxb_css_parser_init(parser, nullptr);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(parser, true);
        return false;
    }

    lxb_selectors_t *selectors = lxb_selectors_create();
    status = lxb_selectors_init(selectors);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(parser, true);
        lxb_selectors_destroy(selectors, true);
        return false;
    }

    lxb_css_selector_list_t *list = lxb_css_selectors_parse(parser,
        reinterpret_cast<const lxb_char_t*>(cssQuery.c_str()), cssQuery.length());

    bool matched = false;
    if (parser->status == LXB_STATUS_OK && list) {
        lxb_selectors_match_node(selectors, lxb_dom_interface_node(element_ptr), list, match_callback, &matched);
    }

    lxb_selectors_destroy(selectors, true);
    lxb_css_parser_destroy(parser, true);
    if (list) {
        lxb_css_selector_list_destroy_memory(list);
    }

    return matched;
}

Handle Element::closest(const std::string& cssQuery) {
    lxb_status_t status;
    lxb_css_parser_t *parser = lxb_css_parser_create();
    status = lxb_css_parser_init(parser, nullptr);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(parser, true);
        return kInvalidHandle;
    }

    lxb_selectors_t *selectors = lxb_selectors_create();
    status = lxb_selectors_init(selectors);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(parser, true);
        lxb_selectors_destroy(selectors, true);
        return kInvalidHandle;
    }

    lxb_css_selector_list_t *list = lxb_css_selectors_parse(parser,
        reinterpret_cast<const lxb_char_t*>(cssQuery.c_str()), cssQuery.length());

    lxb_dom_element_t* found = nullptr;
    if (parser->status == LXB_STATUS_OK && list) {
        for (lxb_dom_node_t* curr = lxb_dom_interface_node(element_ptr); curr != nullptr; curr = curr->parent) {
            if (curr->type == LXB_DOM_NODE_TYPE_ELEMENT) {
                bool matched = false;
                lxb_selectors_match_node(selectors, curr, list, match_callback, &matched);
                if (matched) {
                    found = lxb_dom_interface_element(curr);
                    break;
                }
            }
        }
    }

    lxb_selectors_destroy(selectors, true);
    lxb_css_parser_destroy(parser, true);
    if (list) {
        lxb_css_selector_list_destroy_memory(list);
    }

    return found ? wrap_element(found, parent_doc_handle) : kInvalidHandle;
}

Element& Element::text(const std::string& value) {
    lxb_dom_node_t* parent_node = lxb_dom_interface_node(element_ptr);
    lxb_dom_node_destroy_deep(parent_node->first_child);
    parent_node->first_child = nullptr;
    parent_node->last_child = nullptr;

    lxb_dom_document_t* doc = parent_node->owner_document;
    lxb_dom_text_t* text_node = lxb_dom_document_create_text_node(doc,
        reinterpret_cast<const lxb_char_t*>(value.c_str()), value.length());
    if (text_node) {
        lxb_dom_node_append_child(parent_node, lxb_dom_interface_node(text_node));
    }
    return *this;
}

Element& Element::html(const std::string& value) {
    lxb_html_element_inner_html_set(reinterpret_cast<lxb_html_element_t*>(element_ptr),
        reinterpret_cast<const lxb_char_t*>(value.c_str()), value.length());
    return *this;
}

Element& Element::append(const std::string& html_str) {
    if (html_str.empty()) return *this;
    lxb_dom_node_t* parent = lxb_dom_interface_node(element_ptr);
    lxb_html_document_t* doc = reinterpret_cast<lxb_html_document_t*>(parent->owner_document);
    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(doc,
        element_ptr,
        reinterpret_cast<const lxb_char_t*>(html_str.c_str()), html_str.length());

    if (fragment) {
        lxb_dom_node_t* child = fragment->first_child;
        while (child) {
            lxb_dom_node_t* next = child->next;
            lxb_dom_node_append_child(parent, child);
            child = next;
        }
        lxb_dom_node_destroy(fragment);
    }
    return *this;
}

Element& Element::prepend(const std::string& html_str) {
    if (html_str.empty()) return *this;
    lxb_dom_node_t* parent = lxb_dom_interface_node(element_ptr);
    lxb_html_document_t* doc = reinterpret_cast<lxb_html_document_t*>(parent->owner_document);
    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(doc,
        element_ptr,
        reinterpret_cast<const lxb_char_t*>(html_str.c_str()), html_str.length());

    if (fragment) {
        lxb_dom_node_t* child = fragment->last_child;
        while (child) {
            lxb_dom_node_t* prev = child->prev;
            if (parent->first_child) {
                lxb_dom_node_insert_before(parent->first_child, child);
            } else {
                lxb_dom_node_append_child(parent, child);
            }
            child = prev;
        }
        lxb_dom_node_destroy(fragment);
    }
    return *this;
}

Element& Element::after(const std::string& html_str) {
    lxb_dom_node_t* ref_node = lxb_dom_interface_node(element_ptr);
    lxb_dom_node_t* parent = ref_node->parent;
    if (html_str.empty() || !parent) return *this;

    lxb_html_document_t* doc = reinterpret_cast<lxb_html_document_t*>(ref_node->owner_document);
    lxb_dom_element_t* ctx_el = (parent->type == LXB_DOM_NODE_TYPE_ELEMENT) ?
        lxb_dom_interface_element(parent) :
        lxb_dom_interface_element(lxb_html_document_body_element(doc));

    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(doc, ctx_el,
        reinterpret_cast<const lxb_char_t*>(html_str.c_str()), html_str.length());

    if (fragment) {
        lxb_dom_node_t* child = fragment->last_child;
        while (child) {
            lxb_dom_node_t* prev = child->prev;
            if (ref_node->next) {
                lxb_dom_node_insert_before(ref_node->next, child);
            } else {
                lxb_dom_node_append_child(parent, child);
            }
            child = prev;
        }
        lxb_dom_node_destroy(fragment);
    }
    return *this;
}

Element& Element::before(const std::string& html_str) {
    lxb_dom_node_t* ref_node = lxb_dom_interface_node(element_ptr);
    lxb_dom_node_t* parent = ref_node->parent;
    if (html_str.empty() || !parent) return *this;

    lxb_html_document_t* doc = reinterpret_cast<lxb_html_document_t*>(ref_node->owner_document);
    lxb_dom_element_t* ctx_el = (parent->type == LXB_DOM_NODE_TYPE_ELEMENT) ?
        lxb_dom_interface_element(parent) :
        lxb_dom_interface_element(lxb_html_document_body_element(doc));

    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(doc, ctx_el,
        reinterpret_cast<const lxb_char_t*>(html_str.c_str()), html_str.length());

    if (fragment) {
        lxb_dom_node_t* child = fragment->first_child;
        while (child) {
            lxb_dom_node_t* next = child->next;
            lxb_dom_node_insert_before(ref_node, child);
            child = next;
        }
        lxb_dom_node_destroy(fragment);
    }
    return *this;
}

void Element::remove() {
    lxb_dom_node_remove(lxb_dom_interface_node(element_ptr));
}

Element& Element::wrap(const std::string& html_str) {
    lxb_dom_node_t* ref_node = lxb_dom_interface_node(element_ptr);
    lxb_dom_node_t* parent = ref_node->parent;
    if (html_str.empty() || !parent) return *this;

    lxb_html_document_t* doc = reinterpret_cast<lxb_html_document_t*>(ref_node->owner_document);
    lxb_dom_element_t* ctx_el = (parent->type == LXB_DOM_NODE_TYPE_ELEMENT) ?
        lxb_dom_interface_element(parent) :
        lxb_dom_interface_element(lxb_html_document_body_element(doc));

    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(doc, ctx_el,
        reinterpret_cast<const lxb_char_t*>(html_str.c_str()), html_str.length());

    if (fragment && fragment->first_child) {
        lxb_dom_node_t* wrap_root = nullptr;
        for (lxb_dom_node_t* c = fragment->first_child; c != nullptr; c = c->next) {
            if (c->type == LXB_DOM_NODE_TYPE_ELEMENT) {
                wrap_root = c;
                break;
            }
        }
        if (wrap_root) {
            lxb_dom_node_t* deepest = wrap_root;
            while (true) {
                lxb_dom_node_t* next_deepest = nullptr;
                for (lxb_dom_node_t* c = deepest->first_child; c != nullptr; c = c->next) {
                    if (c->type == LXB_DOM_NODE_TYPE_ELEMENT) {
                        next_deepest = c;
                        break;
                    }
                }
                if (!next_deepest) break;
                deepest = next_deepest;
            }

            lxb_dom_node_insert_before(ref_node, wrap_root);
            lxb_dom_node_remove(ref_node);
            lxb_dom_node_append_child(deepest, ref_node);
        }
        lxb_dom_node_destroy(fragment);
    }
    return *this;
}

Handle Element::unwrap() {
    lxb_dom_node_t* ref_node = lxb_dom_interface_node(element_ptr);
    lxb_dom_node_t* parent = ref_node->parent;
    if (!parent) return kInvalidHandle;

    lxb_dom_node_t* child = ref_node->first_child;
    while (child) {
        lxb_dom_node_t* next = child->next;
        lxb_dom_node_insert_before(ref_node, child);
        child = next;
    }
    lxb_dom_node_remove(ref_node);
    if (parent->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        return wrap_element(lxb_dom_interface_element(parent), parent_doc_handle);
    }
    return kInvalidHandle;
}

} // namespace nrp::lexsoup
