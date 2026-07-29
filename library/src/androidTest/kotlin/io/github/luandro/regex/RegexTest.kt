package io.github.luandro.regex

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Phase 9: Integration — Regex Unit Tests
 *
 * Exercises the full Regex pipeline from Kotlin through JNI
 * to the jsregexp-backed native engine.
 */
@RunWith(AndroidJUnit4::class)
class RegexTest {

    // ----------------------------------------------------------------
    // Pattern compilation
    // ----------------------------------------------------------------

    @Test
    fun compile_simplePattern_doesNotThrow() {
        Regex.compile("hello").close()
    }

    @Test
    fun compile_withFlags_doesNotThrow() {
        Regex.compile("hello", "i").close()
    }

    @Test
    fun pattern_property_returnsOriginalPattern() {
        Regex.compile("\\d+").use { p ->
            assertEquals("\\d+", p.pattern)
        }
    }

    // ----------------------------------------------------------------
    // matches()
    // ----------------------------------------------------------------

    @Test
    fun matches_exactString_returnsTrue() {
        Regex.compile("^hello$").use { p ->
            assertTrue(p.matches("hello"))
        }
    }

    @Test
    fun matches_nonMatchingString_returnsFalse() {
        Regex.compile("^hello$").use { p ->
            assertFalse(p.matches("world"))
        }
    }

    @Test
    fun matches_caseInsensitiveFlag_returnsTrue() {
        Regex.compile("^HELLO$", "i").use { p ->
            assertTrue(p.matches("hello"))
        }
    }

    // ----------------------------------------------------------------
    // find()
    // ----------------------------------------------------------------

    @Test
    fun find_matchExists_returnsMatchResult() {
        Regex.compile("\\d+").use { p ->
            val mr = p.find("abc 123 def")
            assertNotNull(mr)
            mr!!.use { assertEquals("123", it.value) }
        }
    }

    @Test
    fun find_noMatch_returnsNull() {
        Regex.compile("\\d+").use { p ->
            assertNull(p.find("no digits here"))
        }
    }

    // ----------------------------------------------------------------
    // findAll()
    // ----------------------------------------------------------------

    @Test
    fun findAll_multipleMatches_returnsAll() {
        Regex.compile("\\d+").use { p ->
            val results = p.findAll("1 2 3 abc 45")
            assertEquals(4, results.size)
            val values = results.map { it.value }
            assertEquals(listOf("1", "2", "3", "45"), values)
            results.forEach { it.close() }
        }
    }

    @Test
    fun findAll_noMatch_returnsEmpty() {
        Regex.compile("\\d+").use { p ->
            val results = p.findAll("no digits")
            assertTrue(results.isEmpty())
        }
    }

    // ----------------------------------------------------------------
    // replace() / replaceAll()
    // ----------------------------------------------------------------

    @Test
    fun replace_firstOccurrence_replacesOnce() {
        Regex.compile("a").use { p ->
            val result = p.replace("banana", "X")
            assertEquals("bXnana", result)
        }
    }

    @Test
    fun replaceAll_allOccurrences() {
        Regex.compile("a").use { p ->
            val result = p.replaceAll("banana", "X")
            assertEquals("bXnXnX", result)
        }
    }

    // ----------------------------------------------------------------
    // split()
    // ----------------------------------------------------------------

    @Test
    fun split_byDelimiter_returnsTokens() {
        Regex.compile(",").use { p ->
            val parts = p.split("a,b,c")
            assertEquals(listOf("a", "b", "c"), parts)
        }
    }

    @Test
    fun split_noDelimiter_returnsSingleToken() {
        Regex.compile(",").use { p ->
            val parts = p.split("abc")
            assertEquals(listOf("abc"), parts)
        }
    }

    // ----------------------------------------------------------------
    // Matcher
    // ----------------------------------------------------------------

    @Test
    fun matcher_find_iteratesAllMatches() {
        Regex.compile("\\w+").use { p ->
            p.matcher("foo bar baz").use { m ->
                val words = mutableListOf<String>()
                while (m.find()) {
                    words += m.group() ?: ""
                }
                assertEquals(listOf("foo", "bar", "baz"), words)
            }
        }
    }

    @Test
    fun matcher_matches_fullStringMatch() {
        Regex.compile("\\d{3}-\\d{4}").use { p ->
            p.matcher("555-1234").use { m ->
                assertTrue(m.matches())
            }
        }
    }

    @Test
    fun matcher_groupCount_captureGroups() {
        Regex.compile("(\\w+)@(\\w+)").use { p ->
            p.matcher("user@host").use { m ->
                assertTrue(m.find())
                assertTrue(m.groupCount() >= 2)
                assertEquals("user", m.groupByIndex(1))
                assertEquals("host", m.groupByIndex(2))
            }
        }
    }

    @Test
    fun matcher_replaceAll_replacesAll() {
        Regex.compile("\\d").use { p ->
            p.matcher("a1b2c3").use { m ->
                assertEquals("aXbXcX", m.replaceAll("X"))
            }
        }
    }

    // ----------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------

    @Test
    fun closedPattern_throwsOnAccess() {
        val p = Regex.compile("test")
        p.close()
        assertThrows(IllegalStateException::class.java) { p.matches("test") }
    }

    @Test
    fun use_block_closesPattern() {
        var ok = false
        Regex.compile("ok").use { p ->
            assertTrue(p.matches("ok"))
            ok = true
        }
        assertTrue(ok)
    }
}
