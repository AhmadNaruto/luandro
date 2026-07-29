// native/binding/luau/lexsoup_binding.cpp
// Phase 5: LexSoup Engine Luau Bindings

#include "lexsoup_binding.h"
#include "luau_binding.h"
#include <lexsoup/document.h>
#include <lexsoup/element.h>
#include <lexsoup/elements.h>
#include <runtime.h>
#include <converter/type_converter.h>

namespace nrp::luau {

// ==========================================
// Document Luau Bindings
// ==========================================

static int doc_title(lua_State* L) {
    return with_lua_exceptions(L, "Document:title", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        std::string t = doc->title();
        lua_pushlstring(L, t.data(), t.size());
        return 1;
    });
}

static int doc_body(lua_State* L) {
    return with_lua_exceptions(L, "Document:body", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        Handle body_h = doc->body(h);
        if (body_h != kInvalidHandle) {
            push_handle_userdata(L, body_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int doc_head(lua_State* L) {
    return with_lua_exceptions(L, "Document:head", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        Handle head_h = doc->head(h);
        if (head_h != kInvalidHandle) {
            push_handle_userdata(L, head_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int doc_select(lua_State* L) {
    return with_lua_exceptions(L, "Document:select", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        size_t query_len = 0;
        const char* query = luaL_checklstring(L, 2, &query_len);
        Handle els_h = doc->select(h, std::string(query, query_len));
        push_handle_userdata(L, els_h, "lexsoup.Elements");
        return 1;
    });
}

static int doc_getElementById(lua_State* L) {
    return with_lua_exceptions(L, "Document:getElementById", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        size_t id_len = 0;
        const char* id = luaL_checklstring(L, 2, &id_len);
        Handle el_h = doc->getElementById(h, std::string(id, id_len));
        if (el_h != kInvalidHandle) {
            push_handle_userdata(L, el_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int doc_getElementsByTag(lua_State* L) {
    return with_lua_exceptions(L, "Document:getElementsByTag", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        size_t tag_len = 0;
        const char* tag = luaL_checklstring(L, 2, &tag_len);
        Handle els_h = doc->getElementsByTag(h, std::string(tag, tag_len));
        push_handle_userdata(L, els_h, "lexsoup.Elements");
        return 1;
    });
}

static int doc_getElementsByClass(lua_State* L) {
    return with_lua_exceptions(L, "Document:getElementsByClass", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        size_t cls_len = 0;
        const char* cls = luaL_checklstring(L, 2, &cls_len);
        Handle els_h = doc->getElementsByClass(h, std::string(cls, cls_len));
        push_handle_userdata(L, els_h, "lexsoup.Elements");
        return 1;
    });
}

static int doc_outerHtml(lua_State* L) {
    return with_lua_exceptions(L, "Document:outerHtml", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        std::string html = doc->outerHtml();
        lua_pushlstring(L, html.data(), html.size());
        return 1;
    });
}

static int doc_text(lua_State* L) {
    return with_lua_exceptions(L, "Document:text", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Document");
        auto* doc = Runtime::get().objects().get<nrp::lexsoup::Document>(h);
        if (!doc) {
            luaL_error(L, "Document is closed or invalid");
            return 0;
        }
        std::string txt = doc->text();
        lua_pushlstring(L, txt.data(), txt.size());
        return 1;
    });
}

static int doc_close(lua_State* L) {
    return close_method(L, "lexsoup.Document");
}

static int doc_gc(lua_State* L) {
    return gc_metamethod(L, "lexsoup.Document");
}

// ==========================================
// Element Luau Bindings
// ==========================================

static int el_tagName(lua_State* L) {
    return with_lua_exceptions(L, "Element:tagName", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        std::string name = el->tagName();
        lua_pushlstring(L, name.data(), name.size());
        return 1;
    });
}

static int el_attr(lua_State* L) {
    return with_lua_exceptions(L, "Element:attr", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t key_len = 0;
        const char* key = luaL_checklstring(L, 2, &key_len);
        if (lua_gettop(L) >= 3) {
            size_t val_len = 0;
            const char* val = luaL_checklstring(L, 3, &val_len);
            el->attr(std::string(key, key_len), std::string(val, val_len));
            lua_pushvalue(L, 1); // return self
            return 1;
        } else {
            std::string val = el->attr(std::string(key, key_len));
            lua_pushlstring(L, val.data(), val.size());
            return 1;
        }
    });
}

static int el_hasAttr(lua_State* L) {
    return with_lua_exceptions(L, "Element:hasAttr", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t key_len = 0;
        const char* key = luaL_checklstring(L, 2, &key_len);
        lua_pushboolean(L, el->hasAttr(std::string(key, key_len)) ? 1 : 0);
        return 1;
    });
}

static int el_removeAttr(lua_State* L) {
    return with_lua_exceptions(L, "Element:removeAttr", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t key_len = 0;
        const char* key = luaL_checklstring(L, 2, &key_len);
        el->removeAttr(std::string(key, key_len));
        lua_pushvalue(L, 1);
        return 1;
    });
}

static int el_attributes(lua_State* L) {
    return with_lua_exceptions(L, "Element:attributes", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        auto attrs = el->attributes();
        lua_newtable(L);
        for (const auto& pair : attrs) {
            lua_pushlstring(L, pair.first.data(), pair.first.size());
            lua_pushlstring(L, pair.second.data(), pair.second.size());
            lua_settable(L, -3);
        }
        return 1;
    });
}

static int el_id(lua_State* L) {
    return with_lua_exceptions(L, "Element:id", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        std::string val = el->id();
        lua_pushlstring(L, val.data(), val.size());
        return 1;
    });
}

static int el_className(lua_State* L) {
    return with_lua_exceptions(L, "Element:className", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        std::string val = el->className();
        lua_pushlstring(L, val.data(), val.size());
        return 1;
    });
}

static int el_classNames(lua_State* L) {
    return with_lua_exceptions(L, "Element:classNames", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        auto classes = el->classNames();
        lua_newtable(L);
        int idx = 1;
        for (const auto& cls : classes) {
            lua_pushinteger(L, idx++);
            lua_pushlstring(L, cls.data(), cls.size());
            lua_settable(L, -3);
        }
        return 1;
    });
}

static int el_hasClass(lua_State* L) {
    return with_lua_exceptions(L, "Element:hasClass", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t cls_len = 0;
        const char* cls = luaL_checklstring(L, 2, &cls_len);
        lua_pushboolean(L, el->hasClass(std::string(cls, cls_len)) ? 1 : 0);
        return 1;
    });
}

static int el_text(lua_State* L) {
    return with_lua_exceptions(L, "Element:text", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        if (lua_gettop(L) >= 2) {
            size_t val_len = 0;
            const char* val = luaL_checklstring(L, 2, &val_len);
            el->text(std::string(val, val_len));
            lua_pushvalue(L, 1);
            return 1;
        } else {
            std::string txt = el->text();
            lua_pushlstring(L, txt.data(), txt.size());
            return 1;
        }
    });
}

static int el_ownText(lua_State* L) {
    return with_lua_exceptions(L, "Element:ownText", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        std::string txt = el->ownText();
        lua_pushlstring(L, txt.data(), txt.size());
        return 1;
    });
}

static int el_html(lua_State* L) {
    return with_lua_exceptions(L, "Element:html", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        if (lua_gettop(L) >= 2) {
            size_t val_len = 0;
            const char* val = luaL_checklstring(L, 2, &val_len);
            el->html(std::string(val, val_len));
            lua_pushvalue(L, 1);
            return 1;
        } else {
            std::string code = el->html();
            lua_pushlstring(L, code.data(), code.size());
            return 1;
        }
    });
}

static int el_outerHtml(lua_State* L) {
    return with_lua_exceptions(L, "Element:outerHtml", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        std::string code = el->outerHtml();
        lua_pushlstring(L, code.data(), code.size());
        return 1;
    });
}

static int el_parent(lua_State* L) {
    return with_lua_exceptions(L, "Element:parent", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        Handle p_h = el->parent();
        if (p_h != kInvalidHandle) {
            push_handle_userdata(L, p_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int el_children(lua_State* L) {
    return with_lua_exceptions(L, "Element:children", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        Handle els_h = el->children();
        push_handle_userdata(L, els_h, "lexsoup.Elements");
        return 1;
    });
}

static int el_child(lua_State* L) {
    return with_lua_exceptions(L, "Element:child", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        int index = luaL_checkinteger(L, 2);
        Handle ch_h = el->child(index);
        push_handle_userdata(L, ch_h, "lexsoup.Element");
        return 1;
    });
}

static int el_childrenSize(lua_State* L) {
    return with_lua_exceptions(L, "Element:childrenSize", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        lua_pushinteger(L, el->childrenSize());
        return 1;
    });
}

static int el_firstElementChild(lua_State* L) {
    return with_lua_exceptions(L, "Element:firstElementChild", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        Handle ch_h = el->firstElementChild();
        if (ch_h != kInvalidHandle) {
            push_handle_userdata(L, ch_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int el_lastElementChild(lua_State* L) {
    return with_lua_exceptions(L, "Element:lastElementChild", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        Handle ch_h = el->lastElementChild();
        if (ch_h != kInvalidHandle) {
            push_handle_userdata(L, ch_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int el_nextElementSibling(lua_State* L) {
    return with_lua_exceptions(L, "Element:nextElementSibling", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        Handle sh_h = el->nextElementSibling();
        if (sh_h != kInvalidHandle) {
            push_handle_userdata(L, sh_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int el_previousElementSibling(lua_State* L) {
    return with_lua_exceptions(L, "Element:previousElementSibling", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        Handle sh_h = el->previousElementSibling();
        if (sh_h != kInvalidHandle) {
            push_handle_userdata(L, sh_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int el_siblingElements(lua_State* L) {
    return with_lua_exceptions(L, "Element:siblingElements", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        Handle els_h = el->siblingElements();
        push_handle_userdata(L, els_h, "lexsoup.Elements");
        return 1;
    });
}

static int el_select(lua_State* L) {
    return with_lua_exceptions(L, "Element:select", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t query_len = 0;
        const char* query = luaL_checklstring(L, 2, &query_len);
        Handle els_h = el->select(std::string(query, query_len));
        push_handle_userdata(L, els_h, "lexsoup.Elements");
        return 1;
    });
}

static int el_selectFirst(lua_State* L) {
    return with_lua_exceptions(L, "Element:selectFirst", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t query_len = 0;
        const char* query = luaL_checklstring(L, 2, &query_len);
        Handle el_h = el->selectFirst(std::string(query, query_len));
        if (el_h != kInvalidHandle) {
            push_handle_userdata(L, el_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int el_is(lua_State* L) {
    return with_lua_exceptions(L, "Element:is", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t query_len = 0;
        const char* query = luaL_checklstring(L, 2, &query_len);
        lua_pushboolean(L, el->is(std::string(query, query_len)) ? 1 : 0);
        return 1;
    });
}

static int el_closest(lua_State* L) {
    return with_lua_exceptions(L, "Element:closest", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t query_len = 0;
        const char* query = luaL_checklstring(L, 2, &query_len);
        Handle el_h = el->closest(std::string(query, query_len));
        if (el_h != kInvalidHandle) {
            push_handle_userdata(L, el_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int el_append(lua_State* L) {
    return with_lua_exceptions(L, "Element:append", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t val_len = 0;
        const char* val = luaL_checklstring(L, 2, &val_len);
        el->append(std::string(val, val_len));
        lua_pushvalue(L, 1);
        return 1;
    });
}

static int el_prepend(lua_State* L) {
    return with_lua_exceptions(L, "Element:prepend", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t val_len = 0;
        const char* val = luaL_checklstring(L, 2, &val_len);
        el->prepend(std::string(val, val_len));
        lua_pushvalue(L, 1);
        return 1;
    });
}

static int el_after(lua_State* L) {
    return with_lua_exceptions(L, "Element:after", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t val_len = 0;
        const char* val = luaL_checklstring(L, 2, &val_len);
        el->after(std::string(val, val_len));
        lua_pushvalue(L, 1);
        return 1;
    });
}

static int el_before(lua_State* L) {
    return with_lua_exceptions(L, "Element:before", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t val_len = 0;
        const char* val = luaL_checklstring(L, 2, &val_len);
        el->before(std::string(val, val_len));
        lua_pushvalue(L, 1);
        return 1;
    });
}

static int el_remove(lua_State* L) {
    return with_lua_exceptions(L, "Element:remove", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        el->remove();
        return 0;
    });
}

static int el_wrap(lua_State* L) {
    return with_lua_exceptions(L, "Element:wrap", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        size_t val_len = 0;
        const char* val = luaL_checklstring(L, 2, &val_len);
        el->wrap(std::string(val, val_len));
        lua_pushvalue(L, 1);
        return 1;
    });
}

static int el_unwrap(lua_State* L) {
    return with_lua_exceptions(L, "Element:unwrap", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Element");
        auto* el = Runtime::get().objects().get<lexsoup::Element>(h);
        if (!el) {
            luaL_error(L, "Element is closed or invalid");
            return 0;
        }
        Handle parent = el->unwrap();
        if (parent != kInvalidHandle) {
            push_handle_userdata(L, parent, "lexsoup.Element");
            return 1;
        }
        lua_pushnil(L);
        return 1;
    });
}

static int el_close(lua_State* L) {
    return close_method(L, "lexsoup.Element");
}

static int el_gc(lua_State* L) {
    return gc_metamethod(L, "lexsoup.Element");
}

// ==========================================
// Elements Luau Bindings
// ==========================================

static int els_size(lua_State* L) {
    return with_lua_exceptions(L, "Elements:size", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        lua_pushinteger(L, els->size());
        return 1;
    });
}

static int els_isEmpty(lua_State* L) {
    return with_lua_exceptions(L, "Elements:isEmpty", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        lua_pushboolean(L, els->isEmpty() ? 1 : 0);
        return 1;
    });
}

static int els_first(lua_State* L) {
    return with_lua_exceptions(L, "Elements:first", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        Handle el_h = els->first();
        if (el_h != kInvalidHandle) {
            push_handle_userdata(L, el_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int els_last(lua_State* L) {
    return with_lua_exceptions(L, "Elements:last", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        Handle el_h = els->last();
        if (el_h != kInvalidHandle) {
            push_handle_userdata(L, el_h, "lexsoup.Element");
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
}

static int els_get(lua_State* L) {
    return with_lua_exceptions(L, "Elements:get", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        int index = luaL_checkinteger(L, 2);
        Handle el_h = els->get(index);
        push_handle_userdata(L, el_h, "lexsoup.Element");
        return 1;
    });
}

static int els_select(lua_State* L) {
    return with_lua_exceptions(L, "Elements:select", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        size_t query_len = 0;
        const char* query = luaL_checklstring(L, 2, &query_len);
        Handle res_h = els->select(std::string(query, query_len));
        push_handle_userdata(L, res_h, "lexsoup.Elements");
        return 1;
    });
}

static int els_attr(lua_State* L) {
    return with_lua_exceptions(L, "Elements:attr", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        size_t key_len = 0;
        const char* key = luaL_checklstring(L, 2, &key_len);
        if (lua_gettop(L) >= 3) {
            size_t val_len = 0;
            const char* val = luaL_checklstring(L, 3, &val_len);
            els->attr(std::string(key, key_len), std::string(val, val_len));
            lua_pushvalue(L, 1);
            return 1;
        } else {
            std::string val = els->attr(std::string(key, key_len));
            lua_pushlstring(L, val.data(), val.size());
            return 1;
        }
    });
}

static int els_hasAttr(lua_State* L) {
    return with_lua_exceptions(L, "Elements:hasAttr", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        size_t key_len = 0;
        const char* key = luaL_checklstring(L, 2, &key_len);
        lua_pushboolean(L, els->hasAttr(std::string(key, key_len)) ? 1 : 0);
        return 1;
    });
}

static int els_text(lua_State* L) {
    return with_lua_exceptions(L, "Elements:text", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        std::string txt = els->text();
        lua_pushlstring(L, txt.data(), txt.size());
        return 1;
    });
}

static int els_outerHtml(lua_State* L) {
    return with_lua_exceptions(L, "Elements:outerHtml", [&]() {
        Handle h = check_handle_userdata(L, 1, "lexsoup.Elements");
        auto* els = Runtime::get().objects().get<lexsoup::Elements>(h);
        if (!els) {
            luaL_error(L, "Elements is closed or invalid");
            return 0;
        }
        std::string html = els->outerHtml();
        lua_pushlstring(L, html.data(), html.size());
        return 1;
    });
}

static int els_close(lua_State* L) {
    return close_method(L, "lexsoup.Elements");
}

static int els_gc(lua_State* L) {
    return gc_metamethod(L, "lexsoup.Elements");
}

// ==========================================
// Global lexsoup module
// ==========================================

static int lexsoup_parse(lua_State* L) {
    return with_lua_exceptions(L, "lexsoup.parse", [&]() {
        size_t html_len = 0;
        const char* html = luaL_checklstring(L, 1, &html_len);

        lxb_html_document_t* document = lxb_html_document_create();
        if (!document) {
            luaL_error(L, "Failed to create Lexbor HTML Document");
            return 0;
        }
        lxb_status_t status = lxb_html_document_parse(document,
            reinterpret_cast<const lxb_char_t*>(html), html_len);

        if (status != LXB_STATUS_OK) {
            lxb_html_document_destroy(document);
            luaL_error(L, "Failed to parse HTML");
            return 0;
        }

        auto doc_wrapper = std::make_unique<lexsoup::Document>(document);
        nrp::Handle handle = Runtime::get().handles().allocate(0x0101);
        Runtime::get().objects().insert<lexsoup::Document>(handle, std::move(doc_wrapper));

        push_handle_userdata(L, handle, "lexsoup.Document");
        return 1;
    });
}

int luaopen_lexsoup(lua_State* L) {
    // 1. Register Document metatable
    static const luaL_Reg doc_meta[] = {
        {"__gc", doc_gc},
        {"__tostring", [](lua_State* L) { lua_pushstring(L, "Document"); return 1; }},
        {"__newindex", [](lua_State* L) { return read_only_newindex(L, "lexsoup.Document"); }},
        {nullptr, nullptr}
    };
    static const luaL_Reg doc_methods[] = {
        {"title", doc_title},
        {"body", doc_body},
        {"head", doc_head},
        {"select", doc_select},
        {"getElementById", doc_getElementById},
        {"getElementsByTag", doc_getElementsByTag},
        {"getElementsByClass", doc_getElementsByClass},
        {"outerHtml", doc_outerHtml},
        {"text", doc_text},
        {"close", doc_close},
        {nullptr, nullptr}
    };
    register_metatable(L, "lexsoup.Document", doc_meta, doc_methods);

    // 2. Register Element metatable
    static const luaL_Reg el_meta[] = {
        {"__gc", el_gc},
        {"__tostring", [](lua_State* L) { lua_pushstring(L, "Element"); return 1; }},
        {"__newindex", [](lua_State* L) { return read_only_newindex(L, "lexsoup.Element"); }},
        {nullptr, nullptr}
    };
    static const luaL_Reg el_methods[] = {
        {"tagName", el_tagName},
        {"attr", el_attr},
        {"hasAttr", el_hasAttr},
        {"removeAttr", el_removeAttr},
        {"attributes", el_attributes},
        {"id", el_id},
        {"className", el_className},
        {"classNames", el_classNames},
        {"hasClass", el_hasClass},
        {"text", el_text},
        {"ownText", el_ownText},
        {"html", el_html},
        {"outerHtml", el_outerHtml},
        {"innerHTML", el_html},
        {"parent", el_parent},
        {"children", el_children},
        {"child", el_child},
        {"childrenSize", el_childrenSize},
        {"firstElementChild", el_firstElementChild},
        {"lastElementChild", el_lastElementChild},
        {"nextElementSibling", el_nextElementSibling},
        {"previousElementSibling", el_previousElementSibling},
        {"siblingElements", el_siblingElements},
        {"select", el_select},
        {"selectFirst", el_selectFirst},
        {"is", el_is},
        {"closest", el_closest},
        {"append", el_append},
        {"prepend", el_prepend},
        {"after", el_after},
        {"before", el_before},
        {"remove", el_remove},
        {"wrap", el_wrap},
        {"unwrap", el_unwrap},
        {"close", el_close},
        {nullptr, nullptr}
    };
    register_metatable(L, "lexsoup.Element", el_meta, el_methods);

    // 3. Register Elements metatable
    static const luaL_Reg els_meta[] = {
        {"__gc", els_gc},
        {"__tostring", [](lua_State* L) { lua_pushstring(L, "Elements"); return 1; }},
        {"__newindex", [](lua_State* L) { return read_only_newindex(L, "lexsoup.Elements"); }},
        {nullptr, nullptr}
    };
    static const luaL_Reg els_methods[] = {
        {"size", els_size},
        {"isEmpty", els_isEmpty},
        {"first", els_first},
        {"last", els_last},
        {"get", els_get},
        {"select", els_select},
        {"attr", els_attr},
        {"hasAttr", els_hasAttr},
        {"text", els_text},
        {"outerHtml", els_outerHtml},
        {"close", els_close},
        {nullptr, nullptr}
    };
    register_metatable(L, "lexsoup.Elements", els_meta, els_methods);

    // 4. Register the global module
    static const luaL_Reg lexsoup_lib[] = {
        {"parse", lexsoup_parse},
        {nullptr, nullptr}
    };
    luaL_register(L, "lexsoup", lexsoup_lib);
    return 1;
}

} // namespace nrp::luau
