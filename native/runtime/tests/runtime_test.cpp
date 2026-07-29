#include "../handle_manager/handle.h"
#include "../handle_manager/handle_manager.h"
#include "../object_manager/object_manager.h"
#include "../memory/memory_manager.h"
#include "../strings/string_manager.h"
#include "../converter/type_converter.h"
#include "../runtime.h"
#include "../exceptions/exception_manager.h"
#include "../utilities/jni_guard.h"
#include "../utilities/jni_utils.h"
#include "luau/luau_binding.h"
#include <lexsoup/document.h>
#include <lexsoup/element.h>
#include <lexsoup/elements.h>
#include <lua.h>
#include <lualib.h>
#include <iostream>
#include <cassert>

// Define a test class
struct DummyObject {
    static constexpr uint16_t type_tag = 0x0101;
    std::string name;
    DummyObject(std::string n) : name(std::move(n)) {}
};

int main() {
    std::cout << "Starting NRP Runtime Core Unit Tests..." << std::endl;

    // 1. Test HandleManager
    {
        nrp::HandleManager hm;
        assert(hm.live_count() == 0);

        nrp::Handle h1 = hm.allocate(0x0101);
        assert(h1 != nrp::kInvalidHandle);
        assert(hm.live_count() == 1);
        assert(hm.is_valid(h1));
        assert(hm.type_of(h1) == 0x0101);

        hm.release(h1);
        assert(!hm.is_valid(h1));
        assert(hm.live_count() == 0);
        std::cout << "HandleManager tests passed!" << std::endl;
    }

    // 2. Test MemoryManager
    {
        nrp::MemoryManager mm;
        assert(mm.clean());

        void* p = mm.allocate(100);
        assert(p != nullptr);
        assert(mm.bytes_allocated() == 100);
        assert(mm.alloc_count() == 1);
        assert(!mm.clean());

        mm.deallocate(p, 100);
        assert(mm.clean());
        std::cout << "MemoryManager tests passed!" << std::endl;
    }

    // 3. Test ObjectManager
    {
        nrp::HandleManager hm;
        nrp::ObjectManager om(hm);

        nrp::Handle h = hm.allocate(DummyObject::type_tag);
        auto obj = std::make_unique<DummyObject>("NRP Test");
        om.insert(h, std::move(obj));

        assert(om.object_count() == 1);

        DummyObject* retrieved = om.get<DummyObject>(h);
        assert(retrieved != nullptr);
        assert(retrieved->name == "NRP Test");

        om.destroy(h);
        assert(om.object_count() == 0);
        assert(!hm.is_valid(h));
        std::cout << "ObjectManager tests passed!" << std::endl;
    }

    // 4. Test StringManager
    {
        nrp::StringManager sm;
        std::string_view sv1 = sm.intern("hello");
        std::string_view sv2 = sm.intern("hello");
        std::string_view sv3 = sm.intern("world");

        assert(sv1.data() == sv2.data()); // Interned strings should have the same address
        assert(sv1.data() != sv3.data());

        sm.clear_intern_table();
        std::cout << "StringManager tests passed!" << std::endl;
    }

    // 5. Test Runtime Singleton
    {
        nrp::Runtime& rt = nrp::Runtime::get();
        nrp::Handle h = rt.handles().allocate(0x0202);
        assert(h != nrp::kInvalidHandle);
        rt.handles().release(h);
        std::cout << "Runtime singleton tests passed!" << std::endl;
    }

    // 6. Test JNI Exception Translation and Array Conversions
    {
        int called = 0;
        auto result = nrp::jni::withExceptionTranslation(nullptr, [&]() {
            called = 1;
            return 42;
        });
        assert(called == 1);
        assert(result == 42);

        called = 0;
        nrp::jni::withExceptionTranslation(nullptr, [&]() {
            called = 1;
            throw nrp::NrpException("Test Exception");
            return 0;
        });
        assert(called == 1);
        std::cout << "JNI Utilities tests passed!" << std::endl;
    }

    // 7. Test Luau Binding helpers
    {
        lua_State* L = luaL_newstate();
        assert(L != nullptr);

        // Register a metatable
        static const luaL_Reg meta[] = {
            { "__tostring", [](lua_State* L) { lua_pushstring(L, "TestObject"); return 1; } },
            { NULL, NULL }
        };
        nrp::luau::register_metatable(L, "TestType", meta, nullptr);

        // Push a handle userdata
        nrp::Handle h = 0x0101000000000003ULL;
        nrp::luau::push_handle_userdata(L, h, "TestType");

        // Verify and retrieve it
        nrp::Handle retrieved = nrp::luau::check_handle_userdata(L, -1, "TestType");
        assert(retrieved == h);

        lua_close(L);
        std::cout << "Luau Binding utilities tests passed!" << std::endl;
    }

    // 8. Test LexSoup Native Engine
    {
        std::string html = "<html><head><title>Hello Title</title></head><body><div id='content'><p class='text-p'>LexSoup Test</p></div></body></html>";
        lxb_html_document_t* document = lxb_html_document_create();
        assert(document != nullptr);
        lxb_status_t status = lxb_html_document_parse(document,
            reinterpret_cast<const lxb_char_t*>(html.c_str()), html.length());
        assert(status == LXB_STATUS_OK);

        auto doc_wrapper = std::make_unique<nrp::lexsoup::Document>(document);
        nrp::Handle doc_h = nrp::Runtime::get().handles().allocate(0x0101);
        nrp::Runtime::get().objects().insert<nrp::lexsoup::Document>(doc_h, std::move(doc_wrapper));

        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(doc_h);
        assert(doc != nullptr);
        assert(doc->title() == "Hello Title");

        nrp::Handle p_els_h = doc->select(doc_h, ".text-p");
        assert(p_els_h != nrp::kInvalidHandle);
        auto* p_els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(p_els_h);
        assert(p_els != nullptr);
        assert(p_els->size() == 1);

        nrp::Handle p_el_h = p_els->first();
        assert(p_el_h != nrp::kInvalidHandle);
        auto* p_el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(p_el_h);
        assert(p_el != nullptr);
        assert(p_el->tagName() == "p");
        assert(p_el->text() == "LexSoup Test");

        // Modify text
        p_el->text("Modified Value");
        assert(p_el->text() == "Modified Value");

        // Test close cascading (invalidation of child handles)
        nrp::Runtime::get().objects().destroy(doc_h);

        // The document and its children should now be deleted from ObjectManager
        // Trying to retrieve should throw NrpException
        bool doc_stale = false;
        try {
            nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(doc_h);
        } catch (const nrp::NrpException&) {
            doc_stale = true;
        }
        assert(doc_stale);

        bool child_stale = false;
        try {
            nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(p_el_h);
        } catch (const nrp::NrpException&) {
            child_stale = true;
        }
        assert(child_stale);

        std::cout << "LexSoup Native Engine C++ tests passed!" << std::endl;
    }

    std::cout << "All NRP Runtime Core Unit Tests Passed Successfully!" << std::endl;
    return 0;
}
