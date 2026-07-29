package io.github.luandro

import androidx.test.ext.junit.runners.AndroidJUnit4
import io.github.luandro.js.JS
import io.github.luandro.lexsoup.LexSoup
import io.github.luandro.luau.Luau
import io.github.luandro.luau.LuauValue
import io.github.luandro.regex.Regex
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Phase 9: Integration — Stress & Memory-Leak Tests
 *
 * These tests exercise heavy repeated allocation / deallocation cycles
 * to surface use-after-free, double-free, handle leaks, and
 * native memory leaks. They are intentionally repetitive.
 */
@RunWith(AndroidJUnit4::class)
class StressTest {

    // ----------------------------------------------------------------
    // LexSoup — repeated create/destroy
    // ----------------------------------------------------------------

    @Test
    fun lexsoup_createDestroy5000_noLeak() {
        repeat(5000) { i ->
            LexSoup.parse("<p>Item $i</p>").use { doc ->
                assertNotNull(doc)
            }
        }
    }

    @Test
    fun lexsoup_selectAndClose5000_noLeak() {
        val html = "<ul>" + (1..20).joinToString("") { "<li>Item $it</li>" } + "</ul>"
        repeat(5000) {
            LexSoup.parse(html).use { doc ->
                doc.select("li").use { els ->
                    assertEquals(20, els.size())
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Regex — repeated compile/match/destroy
    // ----------------------------------------------------------------

    @Test
    fun regex_compileAndDestroy5000_noLeak() {
        repeat(5000) { i ->
            Regex.compile("\\d+").use { pat ->
                val mr = pat.find("test $i end")
                assertNotNull(mr)
                mr!!.close()
            }
        }
    }

    @Test
    fun regex_matcher_findAll_5000_noLeak() {
        Regex.compile("[aeiou]").use { pat ->
            repeat(5000) {
                pat.matcher("hello world").use { m ->
                    var count = 0
                    while (m.find()) count++
                    assertEquals(3, count) // e, o, o
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Luau — repeated VM create/execute/destroy
    // ----------------------------------------------------------------

    @Test
    fun luau_createVMAndExecute1000_noLeak() {
        repeat(1000) { i ->
            Luau.createVM().use { vm ->
                val v = vm.execute("return $i * 2")
                assertEquals(i * 2, v.toInt())
            }
        }
    }

    @Test
    fun luau_singleVM_execute5000Scripts_noLeak() {
        Luau.createVM().use { vm ->
            repeat(5000) { i ->
                val v = vm.execute("return $i + 1")
                assertEquals(i + 1, v.toInt())
            }
        }
    }

    @Test
    fun luau_setAndRemoveGlobals_1000_noLeak() {
        Luau.createVM().use { vm ->
            repeat(1000) { i ->
                val name = "var_$i"
                vm.setGlobal(name, LuauValue.of(i))
                assertEquals(i, vm.getGlobal(name).toInt())
                vm.removeGlobal(name)
                assertTrue(vm.getGlobal(name).isNil())
            }
        }
    }

    // ----------------------------------------------------------------
    // QuickJS — repeated context create/eval/destroy
    // ----------------------------------------------------------------

    @Test
    fun quickjs_createContextAndEval1000_noLeak() {
        JS.createRuntime().use { rt ->
            repeat(1000) { i ->
                rt.newContext().use { ctx ->
                    ctx.eval("$i * $i").use { v ->
                        assertEquals(i * i, v.toInt())
                    }
                }
            }
        }
    }

    @Test
    fun quickjs_heavyObjectCreation_noLeak() {
        JS.createRuntime().use { rt ->
            rt.newContext().use { ctx ->
                repeat(2000) {
                    ctx.newObject().use { obj ->
                        ctx.newString("value").use { sv ->
                            obj.setProperty("key", sv)
                        }
                        obj.getProperty("key").use { pv ->
                            assertEquals("value", pv.toString())
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Cross-engine stress
    // ----------------------------------------------------------------

    @Test
    fun allEngines_interleavedOperations_500_noLeak() {
        Luau.createVM().use { vm ->
            JS.createRuntime().use { rt ->
                rt.newContext().use { ctx ->
                    Regex.compile("\\d+").use { pat ->
                        repeat(500) { i ->
                            // Luau
                            val luauVal = vm.execute("return $i")
                            assertEquals(i, luauVal.toInt())

                            // QuickJS
                            ctx.eval("$i + 1").use { v ->
                                assertEquals(i + 1, v.toInt())
                            }

                            // Regex
                            val mr = pat.find("item $i end")
                            assertNotNull(mr)
                            mr!!.close()

                            // LexSoup
                            LexSoup.parse("<p>$i</p>").use { doc ->
                                doc.select("p").use { els ->
                                    assertEquals(i.toString(), els.get(0).text())
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Double-close safety (idempotent close)
    // ----------------------------------------------------------------

    @Test
    fun luau_doubleClose_noNativeCrash() {
        val vm = Luau.createVM()
        vm.close()
        vm.close() // must not crash
    }

    @Test
    fun quickjs_doubleClose_noNativeCrash() {
        val rt = JS.createRuntime()
        val ctx = rt.newContext()
        ctx.close()
        ctx.close() // must not crash
        rt.close()
        rt.close()  // must not crash
    }

    @Test
    fun regex_doubleClose_noNativeCrash() {
        val pat = Regex.compile("test")
        pat.close()
        pat.close() // must not crash
    }

    @Test
    fun lexsoup_doubleClose_noNativeCrash() {
        val doc = LexSoup.parse("<p>test</p>")
        doc.close()
        doc.close() // must not crash
    }
}
