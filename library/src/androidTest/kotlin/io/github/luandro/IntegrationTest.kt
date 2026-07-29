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
 * Phase 9: Integration Tests
 *
 * Verifies that ALL engines share the same native Runtime and interoperate
 * correctly within a single process. These tests exercise cross-engine
 * scenarios and confirm the "single source of truth" design principle.
 */
@RunWith(AndroidJUnit4::class)
class IntegrationTest {

    // ----------------------------------------------------------------
    // Cross-engine: LexSoup + Regex
    // ----------------------------------------------------------------

    @Test
    fun lexsoup_extractLinks_filteredByRegex() {
        val html = """
            <html><body>
              <a href="https://example.com">Example</a>
              <a href="https://nrp.github.io">NRP</a>
              <a href="http://insecure.io">Insecure</a>
            </body></html>
        """
        val httpsLinks = mutableListOf<String>()

        LexSoup.parse(html).use { doc ->
            Regex.compile("^https://").use { pattern ->
                doc.select("a").use { els ->
                    for (i in 0 until els.size()) {
                        val href = els.get(i).attr("href")
                        if (pattern.matches(href)) httpsLinks += href
                    }
                }
            }
        }

        assertEquals(2, httpsLinks.size)
        assertTrue(httpsLinks.all { it.startsWith("https://") })
    }

    @Test
    fun lexsoup_extractText_processedByRegex() {
        LexSoup.parse("<p>Price: ${'$'}42.99 USD</p>").use { doc ->
            val text = doc.select("p").use { els -> els.get(0).text() }
            Regex.compile("\\d+\\.\\d+").use { p ->
                val mr = p.find(text)
                assertNotNull(mr)
                assertEquals("42.99", mr!!.value)
                mr.close()
            }
        }
    }

    // ----------------------------------------------------------------
    // Cross-engine: LexSoup inside Luau
    // ----------------------------------------------------------------

    @Test
    fun luau_usesLexsoupGlobal_parsesHtml() {
        Luau.createVM().use { vm ->
            // lexsoup is auto-registered in every VM
            val result = vm.execute("""
                local doc = lexsoup.parse('<h1>Hello from Luau</h1>')
                return doc:title()
            """)
            // title() from a fragment without <title> tag returns ""
            assertTrue(result.isString())
        }
    }

    @Test
    fun luau_usesRegexGlobal_matchesString() {
        Luau.createVM().use { vm ->
            val result = vm.execute("""
                local pattern = regex.compile('\\\\d+')
                return pattern:matches('12345')
            """)
            assertTrue(result.isBool())
            assertTrue(result.toBoolean())
        }
    }

    @Test
    fun luau_usesJsGlobal_runsJavaScript() {
        Luau.createVM().use { vm ->
            val result = vm.execute("""
                local runtime = js.createRuntime()
                local ctx = runtime:newContext()
                local val = ctx:eval('6 * 7')
                local n = val:toNumber()
                return n
            """)
            assertTrue(result.isNumber())
            assertEquals(42, result.toInt())
        }
    }

    // ----------------------------------------------------------------
    // Cross-engine: QuickJS + Regex
    // ----------------------------------------------------------------

    @Test
    fun quickjs_evalResult_processedByRegex() {
        JS.createRuntime().use { rt ->
            rt.newContext().use { ctx ->
                ctx.eval("'user@example.com'").use { v ->
                    val email = v.toString()
                    Regex.compile("^[\\w.]+@[\\w.]+$").use { p ->
                        assertTrue(p.matches(email))
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Cross-engine: QuickJS + LexSoup
    // ----------------------------------------------------------------

    @Test
    fun quickjs_generatesHtml_parsedByLexSoup() {
        JS.createRuntime().use { rt ->
            rt.newContext().use { ctx ->
                ctx.eval(
                    "'<h1>' + 'Generated' + '</h1>'"
                ).use { v ->
                    val html = v.toString()
                    LexSoup.parse(html).use { doc ->
                        doc.select("h1").use { els ->
                            assertEquals(1, els.size())
                            assertEquals("Generated", els.get(0).text())
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Shared runtime — all engines coexist in same process
    // ----------------------------------------------------------------

    @Test
    fun allEngines_runConcurrently_noInterference() {
        val html = "<p id='x'>Hello</p>"
        val script = "return 2 ^ 10"
        val jsCode = "[1,2,3].reduce((a,b) => a+b, 0)"
        val regexPat = "Hello"

        LexSoup.parse(html).use { doc ->
            Luau.createVM().use { vm ->
                JS.createRuntime().use { rt ->
                    rt.newContext().use { ctx ->
                        Regex.compile(regexPat).use { pat ->

                            // LexSoup
                            val text = doc.getElementById("x")!!.text()
                            assertEquals("Hello", text)

                            // Regex
                            assertTrue(pat.matches(text))

                            // Luau
                            val luauResult = vm.execute(script)
                            assertEquals(1024, luauResult.toInt())

                            // QuickJS
                            ctx.eval(jsCode).use { v ->
                                assertEquals(6, v.toInt())
                            }
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Handle safety — objects from different engines don't cross
    // ----------------------------------------------------------------

    @Test
    fun multipleVMs_areIndependent() {
        Luau.createVM().use { vm1 ->
            Luau.createVM().use { vm2 ->
                vm1.setGlobal("x", LuauValue.of(1))
                vm2.setGlobal("x", LuauValue.of(2))
                assertEquals(1, vm1.execute("return x").toInt())
                assertEquals(2, vm2.execute("return x").toInt())
            }
        }
    }

    @Test
    fun multipleJSRuntimes_areIndependent() {
        JS.createRuntime().use { rt1 ->
            JS.createRuntime().use { rt2 ->
                rt1.newContext().use { ctx1 ->
                    rt2.newContext().use { ctx2 ->
                        ctx1.newInt(10).use { v -> ctx1.setGlobal("g", v) }
                        ctx2.newInt(20).use { v -> ctx2.setGlobal("g", v) }
                        assertEquals(10, ctx1.getGlobal("g").toInt())
                        assertEquals(20, ctx2.getGlobal("g").toInt())
                    }
                }
            }
        }
    }
}
