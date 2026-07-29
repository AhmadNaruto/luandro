// native/lexsoup/document.cpp
// Phase 5: LexSoup Engine

#include "document.h"
#include "element.h"
#include "elements.h"
#include <runtime.h>
#include <exceptions/exception_manager.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

namespace nrp::lexsoup {

// Helper functions shared across Document/Element
lxb_status_t serialize_callback(const lxb_char_t *data, size_t len, void *ctx) {
    auto* str = static_cast<std::string*>(ctx);
    str->append(reinterpret_cast<const char*>(data), len);
    return LXB_STATUS_OK;
}

std::string get_outer_html(lxb_dom_node_t* node) {
    std::string html;
    lxb_html_serialize_cb(node, serialize_callback, &html);
    return html;
}

std::string get_node_text(lxb_dom_node_t* node) {
    size_t len = 0;
    lxb_char_t* text = lxb_dom_node_text_content(node, &len);
    if (!text) return "";
    std::string str(reinterpret_cast<const char*>(text), len);
    lxb_dom_document_destroy_text(node->owner_document, text);
    return str;
}

Handle wrap_element(lxb_dom_element_t* el, Handle doc_handle) {
    if (!el) return kInvalidHandle;
    auto wrapper = std::make_unique<Element>(el, doc_handle);
    Handle h = Runtime::get().handles().allocate(0x0102);
    Runtime::get().objects().insert<Element>(h, std::move(wrapper));
    if (doc_handle != kInvalidHandle) {
        auto* doc = Runtime::get().objects().get<Document>(doc_handle);
        if (doc) {
            doc->track_child(h);
        }
    }
    return h;
}

struct SelectContext {
    std::vector<Handle> handles;
    Handle doc_handle;
};

static lxb_status_t find_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t spec, void *ctx) {
    auto* sc = static_cast<SelectContext*>(ctx);
    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        Handle h = wrap_element(lxb_dom_interface_element(node), sc->doc_handle);
        sc->handles.push_back(h);
    }
    return LXB_STATUS_OK;
}

Handle run_css_select(lxb_dom_node_t* root, Handle doc_handle, const std::string& query) {
    lxb_status_t status;
    lxb_css_parser_t *parser = lxb_css_parser_create();
    status = lxb_css_parser_init(parser, nullptr);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(parser, true);
        throw NrpException("Failed to create CSS parser");
    }

    lxb_selectors_t *selectors = lxb_selectors_create();
    status = lxb_selectors_init(selectors);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(parser, true);
        lxb_selectors_destroy(selectors, true);
        throw NrpException("Failed to initialize CSS selectors");
    }

    lxb_css_selector_list_t *list = lxb_css_selectors_parse(parser,
        reinterpret_cast<const lxb_char_t*>(query.c_str()), query.length());

    if (parser->status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(parser, true);
        lxb_selectors_destroy(selectors, true);
        throw NrpException("CSS syntax error: invalid selector query");
    }

    SelectContext sc;
    sc.doc_handle = doc_handle;

    status = lxb_selectors_find(selectors, root, list, find_callback, &sc);

    lxb_selectors_destroy(selectors, true);
    lxb_css_parser_destroy(parser, true);
    lxb_css_selector_list_destroy_memory(list);

    if (status != LXB_STATUS_OK) {
        throw NrpException("CSS select failed execution");
    }

    auto els_wrapper = std::make_unique<Elements>(sc.handles, doc_handle);
    Handle els_handle = Runtime::get().handles().allocate(0x0103);
    Runtime::get().objects().insert<Elements>(els_handle, std::move(els_wrapper));
    if (doc_handle != kInvalidHandle) {
        auto* doc = Runtime::get().objects().get<Document>(doc_handle);
        if (doc) {
            doc->track_child(els_handle);
        }
    }
    return els_handle;
}

Document::~Document() {
    close();
}

void Document::close() {
    if (!document_ptr) return;

    // Destroy all tracked children to avoid leaks and invalid access
    auto children = child_handles;
    for (Handle h : children) {
        try {
            Runtime::get().objects().destroy(h);
        } catch (...) {}
    }
    child_handles.clear();

    lxb_html_document_destroy(document_ptr);
    document_ptr = nullptr;
}

std::string Document::title() {
    if (!document_ptr) throw NrpException("Document is closed");
    size_t len = 0;
    const lxb_char_t* title_str = lxb_html_document_title(document_ptr, &len);
    if (!title_str) return "";
    return std::string(reinterpret_cast<const char*>(title_str), len);
}

Handle Document::body(Handle self_handle) {
    if (!document_ptr) throw NrpException("Document is closed");
    lxb_html_body_element_t* body_el = lxb_html_document_body_element(document_ptr);
    return wrap_element(lxb_dom_interface_element(body_el), self_handle);
}

Handle Document::head(Handle self_handle) {
    if (!document_ptr) throw NrpException("Document is closed");
    lxb_html_head_element_t* head_el = lxb_html_document_head_element(document_ptr);
    return wrap_element(lxb_dom_interface_element(head_el), self_handle);
}

Handle Document::select(Handle self_handle, const std::string& cssQuery) {
    if (!document_ptr) throw NrpException("Document is closed");
    lxb_dom_node_t* root = lxb_dom_interface_node(document_ptr);
    return run_css_select(root, self_handle, cssQuery);
}

Handle Document::getElementById(Handle self_handle, const std::string& id) {
    if (!document_ptr) throw NrpException("Document is closed");
    Handle els_handle = select(self_handle, "#" + id);
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

Handle Document::getElementsByTag(Handle self_handle, const std::string& tag) {
    if (!document_ptr) throw NrpException("Document is closed");
    return select(self_handle, tag);
}

Handle Document::getElementsByClass(Handle self_handle, const std::string& cls) {
    if (!document_ptr) throw NrpException("Document is closed");
    return select(self_handle, "." + cls);
}

std::string Document::outerHtml() {
    if (!document_ptr) throw NrpException("Document is closed");
    lxb_dom_node_t* root = lxb_dom_interface_node(document_ptr);
    return get_outer_html(root);
}

std::string Document::text() {
    if (!document_ptr) throw NrpException("Document is closed");
    lxb_dom_node_t* root = lxb_dom_interface_node(document_ptr);
    return get_node_text(root);
}

} // namespace nrp::lexsoup
