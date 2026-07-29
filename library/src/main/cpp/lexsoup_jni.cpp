// library/src/main/cpp/lexsoup_jni.cpp
// Phase 5: LexSoup Engine JNI Bindings

#include <jni.h>
#include <lexsoup/document.h>
#include <lexsoup/element.h>
#include <lexsoup/elements.h>
#include <runtime.h>
#include <converter/type_converter.h>
#include <utilities/jni_utils.h>

extern "C" {

// ==========================================
// LexSoup JNI
// ==========================================

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_LexSoup_nativeParse(JNIEnv* env, jclass clazz, jstring html) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        nrp::jni::JStringUTF html_guard(env, html);
        std::string html_str = html_guard.str();

        lxb_html_document_t* document = lxb_html_document_create();
        if (!document) {
            throw nrp::NrpException("Failed to create HTML Document in Lexbor");
        }
        lxb_status_t status = lxb_html_document_parse(document,
            reinterpret_cast<const lxb_char_t*>(html_str.c_str()), html_str.length());

        if (status != LXB_STATUS_OK) {
            lxb_html_document_destroy(document);
            throw nrp::NrpException("Failed to parse HTML");
        }

        auto doc_wrapper = std::make_unique<nrp::lexsoup::Document>(document);
        nrp::Handle handle = nrp::Runtime::get().handles().allocate(0x0101);
        nrp::Runtime::get().objects().insert<nrp::lexsoup::Document>(handle, std::move(doc_wrapper));
        return static_cast<jlong>(handle);
    });
}

// ==========================================
// Document JNI
// ==========================================

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Document_nativeTitle(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        return env->NewStringUTF(doc->title().c_str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_nativeBody(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        return doc->body(handle);
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_nativeHead(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        return doc->head(handle);
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_nativeSelect(JNIEnv* env, jobject thiz, jlong handle, jstring query) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        nrp::jni::JStringUTF q_guard(env, query);
        return doc->select(handle, q_guard.str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_nativeGetElementById(JNIEnv* env, jobject thiz, jlong handle, jstring id) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        nrp::jni::JStringUTF id_guard(env, id);
        return doc->getElementById(handle, id_guard.str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_nativeGetElementsByTag(JNIEnv* env, jobject thiz, jlong handle, jstring tag) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        nrp::jni::JStringUTF tag_guard(env, tag);
        return doc->getElementsByTag(handle, tag_guard.str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_nativeGetElementsByClass(JNIEnv* env, jobject thiz, jlong handle, jstring cls) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        nrp::jni::JStringUTF cls_guard(env, cls);
        return doc->getElementsByClass(handle, cls_guard.str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Document_nativeOuterHtml(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        return env->NewStringUTF(doc->outerHtml().c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Document_nativeText(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(handle);
        if (!doc) throw nrp::NrpException("Document is closed or invalid");
        return env->NewStringUTF(doc->text().c_str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Document_nativeClose(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        nrp::Runtime::get().objects().destroy(handle);
    });
}

// ==========================================
// Element JNI
// ==========================================

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Element_nativeTagName(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return env->NewStringUTF(el->tagName().c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Element_nativeAttrGet(JNIEnv* env, jobject thiz, jlong handle, jstring key) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF key_guard(env, key);
        return env->NewStringUTF(el->attr(key_guard.str()).c_str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeAttrSet(JNIEnv* env, jobject thiz, jlong handle, jstring key, jstring value) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF key_guard(env, key);
        nrp::jni::JStringUTF val_guard(env, value);
        el->attr(key_guard.str(), val_guard.str());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_lexsoup_Element_nativeHasAttr(JNIEnv* env, jobject thiz, jlong handle, jstring key) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF key_guard(env, key);
        return el->hasAttr(key_guard.str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeRemoveAttr(JNIEnv* env, jobject thiz, jlong handle, jstring key) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF key_guard(env, key);
        el->removeAttr(key_guard.str());
    });
}

JNIEXPORT jobjectArray JNICALL
Java_io_github_luandro_lexsoup_Element_nativeAttrKeys(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jobjectArray {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        auto attrs = el->attributes();
        std::vector<std::string> keys;
        for (const auto& pair : attrs) {
            keys.push_back(pair.first);
        }
        return nrp::TypeConverter::to_jobjectarray_string(env, keys);
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Element_nativeText(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return env->NewStringUTF(el->text().c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Element_nativeOwnText(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return env->NewStringUTF(el->ownText().c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Element_nativeHtml(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return env->NewStringUTF(el->html().c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Element_nativeOuterHtml(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return env->NewStringUTF(el->outerHtml().c_str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeParent(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->parent();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeChildren(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->children();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeChild(JNIEnv* env, jobject thiz, jlong handle, jint index) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->child(index);
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_lexsoup_Element_nativeChildrenSize(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jint {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->childrenSize();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeFirstElementChild(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->firstElementChild();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeLastElementChild(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->lastElementChild();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeNextElementSibling(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->nextElementSibling();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativePreviousElementSibling(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->previousElementSibling();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeSiblingElements(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->siblingElements();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeSelect(JNIEnv* env, jobject thiz, jlong handle, jstring query) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF q_guard(env, query);
        return el->select(q_guard.str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeSelectFirst(JNIEnv* env, jobject thiz, jlong handle, jstring query) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF q_guard(env, query);
        return el->selectFirst(q_guard.str());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_lexsoup_Element_nativeIs(JNIEnv* env, jobject thiz, jlong handle, jstring query) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF q_guard(env, query);
        return el->is(q_guard.str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeClosest(JNIEnv* env, jobject thiz, jlong handle, jstring query) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF q_guard(env, query);
        return el->closest(q_guard.str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeSetText(JNIEnv* env, jobject thiz, jlong handle, jstring value) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF val_guard(env, value);
        el->text(val_guard.str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeSetHtml(JNIEnv* env, jobject thiz, jlong handle, jstring value) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF val_guard(env, value);
        el->html(val_guard.str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeAppend(JNIEnv* env, jobject thiz, jlong handle, jstring html) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF h_guard(env, html);
        el->append(h_guard.str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativePrepend(JNIEnv* env, jobject thiz, jlong handle, jstring html) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF h_guard(env, html);
        el->prepend(h_guard.str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeAfter(JNIEnv* env, jobject thiz, jlong handle, jstring html) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF h_guard(env, html);
        el->after(h_guard.str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeBefore(JNIEnv* env, jobject thiz, jlong handle, jstring html) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF h_guard(env, html);
        el->before(h_guard.str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeRemove(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        el->remove();
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeWrap(JNIEnv* env, jobject thiz, jlong handle, jstring html) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        nrp::jni::JStringUTF h_guard(env, html);
        el->wrap(h_guard.str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Element_nativeUnwrap(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (!el) throw nrp::NrpException("Element is closed or invalid");
        return el->unwrap();
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Element_nativeClose(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* el = nrp::Runtime::get().objects().get<nrp::lexsoup::Element>(handle);
        if (el) {
            nrp::Handle parent = el->parent_doc();
            if (parent != nrp::kInvalidHandle) {
                auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(parent);
                if (doc) doc->untrack_child(handle);
            }
        }
        nrp::Runtime::get().objects().destroy(handle);
    });
}

// ==========================================
// Elements JNI
// ==========================================

JNIEXPORT jint JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeSize(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jint {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        return els->size();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeFirst(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        return els->first();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeLast(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        return els->last();
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeGet(JNIEnv* env, jobject thiz, jlong handle, jint index) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        return els->get(index);
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeSelect(JNIEnv* env, jobject thiz, jlong handle, jstring query) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        nrp::jni::JStringUTF q_guard(env, query);
        return els->select(q_guard.str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeAttrGet(JNIEnv* env, jobject thiz, jlong handle, jstring key) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        nrp::jni::JStringUTF key_guard(env, key);
        return env->NewStringUTF(els->attr(key_guard.str()).c_str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeAttrSet(JNIEnv* env, jobject thiz, jlong handle, jstring key, jstring value) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        nrp::jni::JStringUTF key_guard(env, key);
        nrp::jni::JStringUTF val_guard(env, value);
        els->attr(key_guard.str(), val_guard.str());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeHasAttr(JNIEnv* env, jobject thiz, jlong handle, jstring key) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        nrp::jni::JStringUTF key_guard(env, key);
        return els->hasAttr(key_guard.str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeText(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        return env->NewStringUTF(els->text().c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeOuterHtml(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (!els) throw nrp::NrpException("Elements collection is closed or invalid");
        return env->NewStringUTF(els->outerHtml().c_str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Elements_nativeClose(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* els = nrp::Runtime::get().objects().get<nrp::lexsoup::Elements>(handle);
        if (els) {
            nrp::Handle parent = els->parent_doc();
            if (parent != nrp::kInvalidHandle) {
                auto* doc = nrp::Runtime::get().objects().get<nrp::lexsoup::Document>(parent);
                if (doc) doc->untrack_child(handle);
            }
        }
        nrp::Runtime::get().objects().destroy(handle);
    });
}

}
