package io.github.luandro.luau

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Phase 9: Integration — LuauVM Unit Tests
 *
 * Exercises the full Luau pipeline from Kotlin through JNI
 * to the Luau VM native engine.
 */
@RunWith(AndroidJUnit4::class)
class LuauTest {

    private lateinit var vm: LuauVM

    @Before
    fun setUp() {
        vm = Luau.createVM()
    }

    @After
    fun tearDown() {
        vm.close()
    }

    // ----------------------------------------------------------------
    // Basic execution
    // ----------------------------------------------------------------

    @Test
    fun execute_arithmeticReturn_returnsNumber() {
        val v = vm.execute("return 1 + 2")
        assertTrue(v.isNumber())
        assertEquals(3, v.toInt())
    }

    @Test
    fun execute_stringReturn_returnsString() {
        val v = vm.execute("return 'hello NRP'")
        assertTrue(v.isString())
        assertEquals("hello NRP", v.toString())
    }

    @Test
    fun execute_boolReturn_returnsBoolean() {
        val v = vm.execute("return true")
        assertTrue(v.isBool())
        assertTrue(v.toBoolean())
    }

    @Test
    fun execute_noReturn_returnsNil() {
        val v = vm.execute("local x = 1")
        assertTrue(v.isNil())
    }

    // ----------------------------------------------------------------
    // Compile + executeCompiled
    // ----------------------------------------------------------------

    @Test
    fun compile_producesNonEmptyBytecode() {
        val bc = vm.compile("return 42")
        assertTrue(bc.isNotEmpty())
    }

    @Test
    fun executeCompiled_returnsCorrectResult() {
        val bc = vm.compile("return 7 * 6")
        val v = vm.executeCompiled(bc)
        assertEquals(42, v.toInt())
    }

    // ----------------------------------------------------------------
    // Global state
    // ----------------------------------------------------------------

    @Test
    fun setGlobal_string_isReadableFromScript() {
        vm.setGlobal("greeting", LuauValue.of("NRP"))
        val v = vm.execute("return greeting")
        assertEquals("NRP", v.toString())
    }

    @Test
    fun setGlobal_number_isReadableFromScript() {
        vm.setGlobal("count", LuauValue.of(99))
        val v = vm.execute("return count + 1")
        assertEquals(100, v.toInt())
    }

    @Test
    fun setGlobal_bool_isReadableFromScript() {
        vm.setGlobal("flag", LuauValue.of(true))
        val v = vm.execute("return flag")
        assertTrue(v.toBoolean())
    }

    @Test
    fun getGlobal_afterSet_roundtrips() {
        vm.setGlobal("x", LuauValue.of(3.14))
        val v = vm.getGlobal("x")
        assertTrue(v.isNumber())
        assertEquals(3.14, v.toDouble(), 0.0001)
    }

    @Test
    fun getGlobal_missingName_returnsNil() {
        val v = vm.getGlobal("__nonexistent_12345")
        assertTrue(v.isNil())
    }

    @Test
    fun removeGlobal_makesItNilInScript() {
        vm.setGlobal("temp", LuauValue.of("delete me"))
        vm.removeGlobal("temp")
        val v = vm.execute("return temp")
        assertTrue(v.isNil())
    }

    // ----------------------------------------------------------------
    // Error handling
    // ----------------------------------------------------------------

    @Test
    fun execute_syntaxError_throwsLuauCompileException() {
        assertThrows(LuauCompileException::class.java) {
            vm.execute("this is not valid luau @@@@")
        }
    }

    @Test
    fun execute_runtimeError_throwsLuauRuntimeException() {
        assertThrows(LuauRuntimeException::class.java) {
            vm.execute("error('boom')")
        }
    }

    // ----------------------------------------------------------------
    // Memory-limited VM
    // ----------------------------------------------------------------

    @Test
    fun createVM_withMemoryLimit_executesSimpleScript() {
        Luau.createVM(8L * 1024 * 1024).use { vm ->
            val v = vm.execute("return 'ok'")
            assertEquals("ok", v.toString())
        }
    }

    // ----------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------

    @Test
    fun closedVM_throwsOnExecute() {
        val v = Luau.createVM()
        v.close()
        assertThrows(IllegalStateException::class.java) { v.execute("return 1") }
    }

    @Test
    fun isLive_beforeClose_returnsTrue() {
        assertTrue(vm.isLive)
    }

    @Test
    fun isLive_afterClose_returnsFalse() {
        val v = Luau.createVM()
        assertTrue(v.isLive)
        v.close()
        assertFalse(v.isLive)
    }

    @Test
    fun use_block_closesVM() {
        var ok = false
        Luau.createVM().use { v ->
            assertEquals(10, v.execute("return 10").toInt())
            ok = true
        }
        assertTrue(ok)
    }

    // ----------------------------------------------------------------
    // Standard library availability
    // ----------------------------------------------------------------

    @Test
    fun standardLib_math_isAvailable() {
        val v = vm.execute("return math.sqrt(16)")
        assertEquals(4.0, v.toDouble(), 0.001)
    }

    @Test
    fun standardLib_string_isAvailable() {
        val v = vm.execute("return string.upper('nrp')")
        assertEquals("NRP", v.toString())
    }

    @Test
    fun standardLib_table_isAvailable() {
        val v = vm.execute("""
            local t = {3, 1, 2}
            table.sort(t)
            return t[1]
        """)
        assertEquals(1, v.toInt())
    }
}
