package io.github.luandro

import androidx.test.ext.junit.runners.AndroidJUnit4
import io.github.luandro.js.JS
import io.github.luandro.lexsoup.LexSoup
import io.github.luandro.luau.Luau
import io.github.luandro.regex.Regex
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import kotlin.system.measureTimeMillis

/**
 * Phase 9: Integration — Performance Tests
 *
 * These tests measure throughput and latency of each engine.
 * They are not pass/fail benchmarks — they emit timing information via
 * logcat and assert only that operations complete within generous upper
 * bounds to detect catastrophic regressions.
 *
 * Run with: adb shell am instrument -w -r -e class io.github.luandro.PerformanceTest
 */
@RunWith(AndroidJUnit4::class)
class PerformanceTest {

    companion object {
        /** Generous upper bound (ms) for each perf assertion. */
        private const val MAX_LEXSOUP_PARSE_MS   = 500L
        private const val MAX_REGEX_OPS_MS       = 500L
        private const val MAX_LUAU_EXEC_MS       = 1000L
        private const val MAX_QUICKJS_EXEC_MS    = 1000L
    }

    // ----------------------------------------------------------------
    // LexSoup throughput
    // ----------------------------------------------------------------

    @Test
    fun lexsoup_parse1000SmallDocuments_withinTimeLimit() {
        val html = "<html><body><h1>Hello</h1><p>World</p></body></html>"
        val elapsed = measureTimeMillis {
            repeat(1000) {
                LexSoup.parse(html).use { doc ->
                    doc.select("h1").use { els ->
                        assertEquals("Hello", els.get(0).text())
                    }
                }
            }
        }
        println("[PERF] LexSoup 1000x parse: ${elapsed}ms")
        assertTrue(
            "LexSoup 1000x parse took ${elapsed}ms (limit ${MAX_LEXSOUP_PARSE_MS}ms)",
            elapsed < MAX_LEXSOUP_PARSE_MS
        )
    }

    @Test
    fun lexsoup_parseLargeDocument_withinTimeLimit() {
        // Build a ~100 KB HTML document
        val sb = StringBuilder()
        sb.append("<html><body>")
        repeat(2000) { i ->
            sb.append("<p class='item' id='item-$i'>Item $i content with <b>bold</b> and <em>emphasis</em>.</p>")
        }
        sb.append("</body></html>")
        val html = sb.toString()

        val elapsed = measureTimeMillis {
            LexSoup.parse(html).use { doc ->
                doc.select(".item").use { els ->
                    assertEquals(2000, els.size())
                }
            }
        }
        println("[PERF] LexSoup large doc parse+select 2000 elements: ${elapsed}ms")
        assertTrue(
            "LexSoup large doc took ${elapsed}ms (limit ${MAX_LEXSOUP_PARSE_MS}ms)",
            elapsed < MAX_LEXSOUP_PARSE_MS
        )
    }

    // ----------------------------------------------------------------
    // Regex throughput
    // ----------------------------------------------------------------

    @Test
    fun regex_10000Matches_withinTimeLimit() {
        Regex.compile("\\b\\w{5}\\b").use { pat ->
            val input = "hello world every single piece of text here today"
            val elapsed = measureTimeMillis {
                repeat(10_000) {
                    pat.find(input)?.close()
                }
            }
            println("[PERF] Regex 10000x find: ${elapsed}ms")
            assertTrue(
                "Regex 10000x find took ${elapsed}ms (limit ${MAX_REGEX_OPS_MS}ms)",
                elapsed < MAX_REGEX_OPS_MS
            )
        }
    }

    @Test
    fun regex_replaceAll_10000times_withinTimeLimit() {
        Regex.compile("\\d+").use { pat ->
            val input = "Order 123 processed at 456 timestamp 789"
            val elapsed = measureTimeMillis {
                repeat(10_000) {
                    pat.replaceAll(input, "#")
                }
            }
            println("[PERF] Regex 10000x replaceAll: ${elapsed}ms")
            assertTrue(
                "Regex 10000x replaceAll took ${elapsed}ms (limit ${MAX_REGEX_OPS_MS}ms)",
                elapsed < MAX_REGEX_OPS_MS
            )
        }
    }

    // ----------------------------------------------------------------
    // Luau throughput
    // ----------------------------------------------------------------

    @Test
    fun luau_execute1000Scripts_withinTimeLimit() {
        Luau.createVM().use { vm ->
            val elapsed = measureTimeMillis {
                repeat(1000) {
                    val v = vm.execute("return math.sqrt($it)")
                    assertTrue(v.isNumber())
                }
            }
            println("[PERF] Luau 1000x execute: ${elapsed}ms")
            assertTrue(
                "Luau 1000x execute took ${elapsed}ms (limit ${MAX_LUAU_EXEC_MS}ms)",
                elapsed < MAX_LUAU_EXEC_MS
            )
        }
    }

    @Test
    fun luau_compileThenExecute100Times_withinTimeLimit() {
        Luau.createVM().use { vm ->
            val source = """
                local sum = 0
                for i = 1, 100 do sum = sum + i end
                return sum
            """
            val bytecode = vm.compile(source)
            val elapsed = measureTimeMillis {
                repeat(100) {
                    val v = vm.executeCompiled(bytecode)
                    assertEquals(5050, v.toInt())
                }
            }
            println("[PERF] Luau 100x executeCompiled: ${elapsed}ms")
            assertTrue(
                "Luau 100x executeCompiled took ${elapsed}ms (limit ${MAX_LUAU_EXEC_MS}ms)",
                elapsed < MAX_LUAU_EXEC_MS
            )
        }
    }

    // ----------------------------------------------------------------
    // QuickJS throughput
    // ----------------------------------------------------------------

    @Test
    fun quickjs_eval1000Scripts_withinTimeLimit() {
        JS.createRuntime().use { rt ->
            rt.newContext().use { ctx ->
                val elapsed = measureTimeMillis {
                    repeat(1000) {
                        ctx.eval("Math.sqrt($it)").use { v ->
                            assertTrue(v.isNumber())
                        }
                    }
                }
                println("[PERF] QuickJS 1000x eval: ${elapsed}ms")
                assertTrue(
                    "QuickJS 1000x eval took ${elapsed}ms (limit ${MAX_QUICKJS_EXEC_MS}ms)",
                    elapsed < MAX_QUICKJS_EXEC_MS
                )
            }
        }
    }
}
