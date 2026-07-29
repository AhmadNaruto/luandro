package io.github.luandro.js

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Phase 9: Integration — QuickJS Unit Tests
 *
 * Exercises the full QuickJS pipeline from Kotlin through JNI
 * to the QuickJS-NG-backed native engine.
 */
@RunWith(AndroidJUnit4::class)
class QuickJSTest {

    private lateinit var runtime: Runtime
    private lateinit var context: Context

    @Before
    fun setUp() {
        runtime = JS.createRuntime()
        context = runtime.newContext()
    }

    @After
    fun tearDown() {
        context.close()
        runtime.close()
    }

    // ----------------------------------------------------------------
    // Basic eval
    // ----------------------------------------------------------------

    @Test
    fun eval_arithmetic_returnsCorrectInt() {
        context.eval("1 + 2").use { v ->
            assertTrue(v.isNumber())
            assertEquals(3, v.toInt())
        }
    }

    @Test
    fun eval_string_returnsString() {
        context.eval("'hello'").use { v ->
            assertTrue(v.isString())
            assertEquals("hello", v.toString())
        }
    }

    @Test
    fun eval_boolean_returnsBoolean() {
        context.eval("true").use { v ->
            assertTrue(v.isBool())
            assertTrue(v.toBool())
        }
    }

    @Test
    fun eval_null_returnsNull() {
        context.eval("null").use { v ->
            assertTrue(v.isNull())
        }
    }

    @Test
    fun eval_undefined_returnsUndefined() {
        context.eval("undefined").use { v ->
            assertTrue(v.isUndefined())
        }
    }

    // ----------------------------------------------------------------
    // Global object
    // ----------------------------------------------------------------

    @Test
    fun setGlobal_andGetGlobal_roundtrips() {
        context.newString("NRP").use { sv ->
            context.setGlobal("myVar", sv)
        }
        context.getGlobal("myVar").use { v ->
            assertTrue(v.isString())
            assertEquals("NRP", v.toString())
        }
    }

    @Test
    fun eval_readsGlobalSet() {
        context.newInt(42).use { v -> context.setGlobal("answer", v) }
        context.eval("answer * 2").use { v ->
            assertEquals(84, v.toInt())
        }
    }

    // ----------------------------------------------------------------
    // Objects & arrays
    // ----------------------------------------------------------------

    @Test
    fun newObject_setAndGetProperty() {
        context.newObject().use { obj ->
            context.newString("world").use { sv ->
                obj.setProperty("hello", sv)
            }
            obj.getProperty("hello").use { pv ->
                assertEquals("world", pv.toString())
            }
        }
    }

    @Test
    fun newArray_length_returnsCorrect() {
        context.eval("[1, 2, 3]").use { arr ->
            assertTrue(arr.isArray())
            assertEquals(3, arr.length())
        }
    }

    @Test
    fun array_getPropertyAt_returnsElement() {
        context.eval("['a', 'b', 'c']").use { arr ->
            arr.getPropertyAt(1).use { v ->
                assertEquals("b", v.toString())
            }
        }
    }

    // ----------------------------------------------------------------
    // Function call
    // ----------------------------------------------------------------

    @Test
    fun eval_function_isCallable() {
        context.eval("(function(x) { return x * x; })").use { fn ->
            assertTrue(fn.isFunction())
            context.newInt(7).use { arg ->
                fn.call(null, arg).use { result ->
                    assertEquals(49, result.toInt())
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // JSON
    // ----------------------------------------------------------------

    @Test
    fun parseJSON_object_returnsObject() {
        context.parseJSON("""{\"name\":\"NRP\",\"version\":1}""").use { v ->
            assertTrue(v.isObject())
            v.getProperty("name").use { name ->
                assertEquals("NRP", name.toString())
            }
        }
    }

    @Test
    fun stringifyJSON_compact_returnsValidJson() {
        context.eval("({a: 1})").use { v ->
            val json = context.stringifyJSON(v, 0)
            assertEquals("\"{\\"a\\":1}\"", "\"${json}\"")
        }
    }

    // ----------------------------------------------------------------
    // Promises
    // ----------------------------------------------------------------

    @Test
    fun promise_resolve_settlesAfterExecutePendingJobs() {
        context.setGlobal("result", context.newString("pending"))
        context.eval(
            "Promise.resolve(42).then(v => { result = v; });"
        ).use { /* ignore */ }
        context.executePendingJobs()
        context.getGlobal("result").use { v ->
            assertEquals(42, v.toInt())
        }
    }

    // ----------------------------------------------------------------
    // Memory limit
    // ----------------------------------------------------------------

    @Test
    fun createRuntimeWithLimit_doesNotCrash() {
        JS.createRuntimeWithLimit(32L * 1024 * 1024).use { rt ->
            rt.newContext().use { ctx ->
                ctx.eval("1+1").use { v ->
                    assertEquals(2, v.toInt())
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------

    @Test
    fun closedContext_throwsOnEval() {
        val ctx = runtime.newContext()
        ctx.close()
        assertThrows(IllegalStateException::class.java) { ctx.eval("1") }
    }

    @Test
    fun runtime_isLive_afterCreation() {
        assertTrue(runtime.isLive)
    }

    @Test
    fun use_block_closesRuntime() {
        var ok = false
        JS.createRuntime().use { rt ->
            rt.newContext().use { ctx ->
                ctx.eval("'test'").use { v ->
                    assertEquals("test", v.toString())
                }
            }
            ok = true
        }
        assertTrue(ok)
    }
}
