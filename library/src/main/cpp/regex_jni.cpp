// library/src/main/cpp/regex_jni.cpp
// Phase 6: Regex Engine JNI Bindings

#include <jni.h>
#include <regex/regex.h>
#include <regex/pattern.h>
#include <regex/matcher.h>
#include <regex/match_result.h>
#include <runtime.h>
#include <converter/type_converter.h>
#include <utilities/jni_utils.h>

extern "C" {

// ==========================================
// Regex (static utility) JNI
// ==========================================

JNIEXPORT jlong JNICALL
Java_io_github_luandro_regex_Regex_nativeCompile(JNIEnv* env, jclass clazz, jstring pattern, jstring flags) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        nrp::jni::JStringUTF pat_guard(env, pattern);
        nrp::jni::JStringUTF fl_guard(env, flags);
        nrp::Handle h = nrp::regex::Regex::compile(pat_guard.str(), fl_guard.str());
        return static_cast<jlong>(h);
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_regex_Regex_nativeMatches(JNIEnv* env, jclass clazz, jstring pattern, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        nrp::jni::JStringUTF pat_guard(env, pattern);
        nrp::jni::JStringUTF inp_guard(env, input);
        return static_cast<jboolean>(nrp::regex::Regex::matches(pat_guard.str(), inp_guard.str()));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_regex_Regex_nativeFind(JNIEnv* env, jobject thiz, jstring pattern, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        nrp::jni::JStringUTF pat_guard(env, pattern);
        nrp::jni::JStringUTF inp_guard(env, input);
        // Allocate a temporary pattern handle for the static find operation
        nrp::Handle ph = nrp::regex::Regex::compile(pat_guard.str(), "");
        nrp::Handle mrh = nrp::regex::Regex::find(ph, pat_guard.str(), inp_guard.str());
        // Destroy the temporary pattern handle
        nrp::Runtime::get().objects().destroy(ph);
        return static_cast<jlong>(mrh);
    });
}

JNIEXPORT jlongArray JNICALL
Java_io_github_luandro_regex_Regex_nativeFindAll(JNIEnv* env, jobject thiz, jstring pattern, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlongArray {
        nrp::jni::JStringUTF pat_guard(env, pattern);
        nrp::jni::JStringUTF inp_guard(env, input);
        nrp::Handle ph = nrp::regex::Regex::compile(pat_guard.str(), "");
        std::vector<nrp::Handle> handles = nrp::regex::Regex::findAll(ph, pat_guard.str(), inp_guard.str());
        nrp::Runtime::get().objects().destroy(ph);
        jlongArray arr = env->NewLongArray(static_cast<jsize>(handles.size()));
        if (arr) {
            std::vector<jlong> jhandles(handles.begin(), handles.end());
            env->SetLongArrayRegion(arr, 0, static_cast<jsize>(jhandles.size()), jhandles.data());
        }
        return arr;
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Regex_nativeReplace(JNIEnv* env, jobject thiz, jstring pattern, jstring input, jstring replacement) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        nrp::jni::JStringUTF pat_guard(env, pattern);
        nrp::jni::JStringUTF inp_guard(env, input);
        nrp::jni::JStringUTF rep_guard(env, replacement);
        std::string result = nrp::regex::Regex::replace(pat_guard.str(), inp_guard.str(), rep_guard.str());
        return env->NewStringUTF(result.c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Regex_nativeReplaceAll(JNIEnv* env, jobject thiz, jstring pattern, jstring input, jstring replacement) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        nrp::jni::JStringUTF pat_guard(env, pattern);
        nrp::jni::JStringUTF inp_guard(env, input);
        nrp::jni::JStringUTF rep_guard(env, replacement);
        std::string result = nrp::regex::Regex::replaceAll(pat_guard.str(), inp_guard.str(), rep_guard.str());
        return env->NewStringUTF(result.c_str());
    });
}

JNIEXPORT jobjectArray JNICALL
Java_io_github_luandro_regex_Regex_nativeSplit(JNIEnv* env, jobject thiz, jstring pattern, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jobjectArray {
        nrp::jni::JStringUTF pat_guard(env, pattern);
        nrp::jni::JStringUTF inp_guard(env, input);
        std::vector<std::string> parts = nrp::regex::Regex::split(pat_guard.str(), inp_guard.str());
        return nrp::TypeConverter::to_jobjectarray_string(env, parts);
    });
}

// ==========================================
// Pattern JNI
// ==========================================

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Pattern_nativePattern(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        return env->NewStringUTF(pat->pattern().c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Pattern_nativeFlags(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        return env->NewStringUTF(pat->flags().c_str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_regex_Pattern_nativeMatcher(JNIEnv* env, jobject thiz, jlong handle, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        nrp::jni::JStringUTF inp_guard(env, input);
        nrp::Handle mh = pat->matcher(handle, inp_guard.str());
        return static_cast<jlong>(mh);
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_regex_Pattern_nativeMatches(JNIEnv* env, jobject thiz, jlong handle, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        nrp::jni::JStringUTF inp_guard(env, input);
        return static_cast<jboolean>(pat->matches(inp_guard.str()));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_regex_Pattern_nativeFind(JNIEnv* env, jobject thiz, jlong handle, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        nrp::jni::JStringUTF inp_guard(env, input);
        nrp::Handle mrh = pat->find(handle, inp_guard.str());
        return static_cast<jlong>(mrh);
    });
}

JNIEXPORT jlongArray JNICALL
Java_io_github_luandro_regex_Pattern_nativeFindAll(JNIEnv* env, jobject thiz, jlong handle, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlongArray {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        nrp::jni::JStringUTF inp_guard(env, input);
        std::vector<nrp::Handle> handles = pat->findAll(handle, inp_guard.str());
        jlongArray arr = env->NewLongArray(static_cast<jsize>(handles.size()));
        if (arr) {
            std::vector<jlong> jhandles(handles.begin(), handles.end());
            env->SetLongArrayRegion(arr, 0, static_cast<jsize>(jhandles.size()), jhandles.data());
        }
        return arr;
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Pattern_nativeReplace(JNIEnv* env, jobject thiz, jlong handle, jstring input, jstring replacement) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        nrp::jni::JStringUTF inp_guard(env, input);
        nrp::jni::JStringUTF rep_guard(env, replacement);
        std::string result = pat->replace(inp_guard.str(), rep_guard.str());
        return env->NewStringUTF(result.c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Pattern_nativeReplaceAll(JNIEnv* env, jobject thiz, jlong handle, jstring input, jstring replacement) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        nrp::jni::JStringUTF inp_guard(env, input);
        nrp::jni::JStringUTF rep_guard(env, replacement);
        std::string result = pat->replaceAll(inp_guard.str(), rep_guard.str());
        return env->NewStringUTF(result.c_str());
    });
}

JNIEXPORT jobjectArray JNICALL
Java_io_github_luandro_regex_Pattern_nativeSplit(JNIEnv* env, jobject thiz, jlong handle, jstring input) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jobjectArray {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (!pat) throw nrp::NrpException("Pattern is closed or invalid");
        nrp::jni::JStringUTF inp_guard(env, input);
        std::vector<std::string> parts = pat->split(inp_guard.str());
        return nrp::TypeConverter::to_jobjectarray_string(env, parts);
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_regex_Pattern_nativeClose(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(handle);
        if (pat) pat->close();
        nrp::Runtime::get().objects().destroy(handle);
    });
}

// ==========================================
// Matcher JNI
// ==========================================

JNIEXPORT jlong JNICALL
Java_io_github_luandro_regex_Matcher_nativeGetPattern(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jlong>(m->pattern());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Matcher_nativeGetInput(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return env->NewStringUTF(m->input().c_str());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_regex_Matcher_nativeHasMatch(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jboolean>(m->hasMatch());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_regex_Matcher_nativeMatches(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jboolean>(m->matches());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_regex_Matcher_nativeFind(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jboolean>(m->find());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_regex_Matcher_nativeFindFrom(JNIEnv* env, jobject thiz, jlong handle, jint startIndex) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jboolean>(m->findFrom(static_cast<int>(startIndex)));
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_regex_Matcher_nativeLookingAt(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jboolean>(m->lookingAt());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Matcher_nativeGroup(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        std::string g = m->group();
        return env->NewStringUTF(g.c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Matcher_nativeGroupByIndex(JNIEnv* env, jobject thiz, jlong handle, jint groupIndex) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        std::string g = m->groupByIndex(static_cast<int>(groupIndex));
        return env->NewStringUTF(g.c_str());
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_regex_Matcher_nativeGroupCount(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jint {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jint>(m->groupCount());
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_regex_Matcher_nativeStart(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jint {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jint>(m->start());
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_regex_Matcher_nativeEnd(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jint {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        return static_cast<jint>(m->end());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_regex_Matcher_nativeReset(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        m->reset();
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_regex_Matcher_nativeResetWithInput(JNIEnv* env, jobject thiz, jlong handle, jstring input) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        nrp::jni::JStringUTF inp_guard(env, input);
        m->resetWithInput(inp_guard.str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Matcher_nativeReplaceAll(JNIEnv* env, jobject thiz, jlong handle, jstring replacement) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        nrp::jni::JStringUTF rep_guard(env, replacement);
        std::string result = m->replaceAll(rep_guard.str());
        return env->NewStringUTF(result.c_str());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_Matcher_nativeReplaceFirst(JNIEnv* env, jobject thiz, jlong handle, jstring replacement) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        nrp::jni::JStringUTF rep_guard(env, replacement);
        std::string result = m->replaceFirst(rep_guard.str());
        return env->NewStringUTF(result.c_str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_regex_Matcher_nativeToMatchResult(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (!m) throw nrp::NrpException("Matcher is closed or invalid");
        nrp::Handle mrh = m->toMatchResult(handle);
        return static_cast<jlong>(mrh);
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_regex_Matcher_nativeClose(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* m = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(handle);
        if (m) {
            nrp::Handle parent = m->pattern();
            if (parent != nrp::kInvalidHandle) {
                auto* pat = nrp::Runtime::get().objects().get<nrp::regex::Pattern>(parent);
                if (pat) pat->untrack_child(handle);
            }
            m->close();
        }
        nrp::Runtime::get().objects().destroy(handle);
    });
}

// ==========================================
// MatchResult JNI
// ==========================================

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_MatchResult_nativeValue(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* mr = nrp::Runtime::get().objects().get<nrp::regex::MatchResult>(handle);
        if (!mr) throw nrp::NrpException("MatchResult is closed or invalid");
        return env->NewStringUTF(mr->value().c_str());
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_regex_MatchResult_nativeStart(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jint {
        auto* mr = nrp::Runtime::get().objects().get<nrp::regex::MatchResult>(handle);
        if (!mr) throw nrp::NrpException("MatchResult is closed or invalid");
        return static_cast<jint>(mr->start());
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_regex_MatchResult_nativeEnd(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jint {
        auto* mr = nrp::Runtime::get().objects().get<nrp::regex::MatchResult>(handle);
        if (!mr) throw nrp::NrpException("MatchResult is closed or invalid");
        return static_cast<jint>(mr->end());
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_regex_MatchResult_nativeGroupCount(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jint {
        auto* mr = nrp::Runtime::get().objects().get<nrp::regex::MatchResult>(handle);
        if (!mr) throw nrp::NrpException("MatchResult is closed or invalid");
        return static_cast<jint>(mr->groupCount());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_regex_MatchResult_nativeGroupValue(JNIEnv* env, jobject thiz, jlong handle, jint index) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jstring {
        auto* mr = nrp::Runtime::get().objects().get<nrp::regex::MatchResult>(handle);
        if (!mr) throw nrp::NrpException("MatchResult is closed or invalid");
        std::string val = mr->groupValue(static_cast<int>(index));
        return env->NewStringUTF(val.c_str());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_regex_MatchResult_nativeClose(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* mr = nrp::Runtime::get().objects().get<nrp::regex::MatchResult>(handle);
        if (mr) {
            nrp::Handle parent = mr->parent();
            if (parent != nrp::kInvalidHandle) {
                auto* matcher = nrp::Runtime::get().objects().get<nrp::regex::Matcher>(parent);
                if (matcher) matcher->untrack_child(handle);
            }
            mr->close();
        }
        nrp::Runtime::get().objects().destroy(handle);
    });
}

} // extern "C"
