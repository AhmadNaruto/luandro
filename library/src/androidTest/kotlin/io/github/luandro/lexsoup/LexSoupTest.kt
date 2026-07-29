package io.github.luandro.lexsoup

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Phase 9: Integration — LexSoup Unit Tests
 *
 * Exercises the full LexSoup HTML-parser pipeline from Kotlin through JNI
 * to the Lexbor-backed native engine.
 */
@RunWith(AndroidJUnit4::class)
class LexSoupTest {

    // ----------------------------------------------------------------
    // Setup
    // ----------------------------------------------------------------

    private lateinit var doc: Document

    @Before
    fun setUp() {
        doc = LexSoup.parse(
            """<!DOCTYPE html>
            <html>
              <head><title>NRP Test</title></head>
              <body>
                <h1 id=\"main\">Hello World</h1>
                <p class=\"intro\">First paragraph.</p>
                <p class=\"intro\">Second paragraph.</p>
                <ul>
                  <li>Alpha</li>
                  <li>Beta</li>
                  <li>Gamma</li>
                </ul>
                <a href=\"https://example.com\">Link</a>
              </body>
            </html>"""
        )
    }

    @After
    fun tearDown() {
        doc.close()
    }

    // ----------------------------------------------------------------
    // Basic parsing
    // ----------------------------------------------------------------

    @Test
    fun title_returnsCorrectText() {
        assertEquals("NRP Test", doc.title())
    }

    @Test
    fun body_isNotNull() {
        assertNotNull(doc.body())
    }

    @Test
    fun head_isNotNull() {
        assertNotNull(doc.head())
    }

    // ----------------------------------------------------------------
    // CSS selector
    // ----------------------------------------------------------------

    @Test
    fun select_byTag_returnsCorrectCount() {
        doc.select("li").use { els ->
            assertEquals(3, els.size())
        }
    }

    @Test
    fun select_byClass_returnsMatchingElements() {
        doc.select(".intro").use { els ->
            assertEquals(2, els.size())
            assertEquals("First paragraph.", els.get(0).text())
            assertEquals("Second paragraph.", els.get(1).text())
        }
    }

    @Test
    fun select_byId_returnsSingleElement() {
        doc.select("#main").use { els ->
            assertEquals(1, els.size())
            assertEquals("Hello World", els.get(0).text())
        }
    }

    // ----------------------------------------------------------------
    // getElementById / getElementsByTag / getElementsByClass
    // ----------------------------------------------------------------

    @Test
    fun getElementById_findsElement() {
        val el = doc.getElementById("main")
        assertNotNull(el)
        assertEquals("Hello World", el!!.text())
    }

    @Test
    fun getElementById_returnsNullForMissing() {
        assertNull(doc.getElementById("nonexistent"))
    }

    @Test
    fun getElementsByTag_returnsAll() {
        doc.getElementsByTag("p").use { els ->
            assertEquals(2, els.size())
        }
    }

    @Test
    fun getElementsByClass_returnsAll() {
        doc.getElementsByClass("intro").use { els ->
            assertEquals(2, els.size())
        }
    }

    // ----------------------------------------------------------------
    // DOM traversal
    // ----------------------------------------------------------------

    @Test
    fun element_text_returnsVisibleText() {
        val el = doc.getElementById("main")
        assertNotNull(el)
        assertEquals("Hello World", el!!.text())
    }

    @Test
    fun element_tagName_isCorrect() {
        val el = doc.getElementById("main")
        assertNotNull(el)
        assertEquals("h1", el!!.tagName())
    }

    @Test
    fun element_attr_returnsAttributeValue() {
        doc.select("a").use { els ->
            assertEquals(1, els.size())
            assertEquals("https://example.com", els.get(0).attr("href"))
        }
    }

    // ----------------------------------------------------------------
    // Serialisation
    // ----------------------------------------------------------------

    @Test
    fun outerHtml_isNotEmpty() {
        val html = doc.outerHtml()
        assertTrue(html.contains("NRP Test"))
        assertTrue(html.contains("Hello World"))
    }

    @Test
    fun document_text_containsAllVisibleText() {
        val text = doc.text()
        assertTrue(text.contains("Hello World"))
        assertTrue(text.contains("First paragraph."))
        assertTrue(text.contains("Alpha"))
    }

    // ----------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------

    @Test
    fun closedDocument_throwsOnAccess() {
        val d = LexSoup.parse("<p>Test</p>")
        d.close()
        assertThrows(DocumentClosedException::class.java) { d.title() }
    }

    @Test
    fun use_block_closesDocument() {
        var handled = false
        LexSoup.parse("<p>ok</p>").use { d ->
            assertEquals("", d.title())
            handled = true
        }
        assertTrue(handled)
    }

    // ----------------------------------------------------------------
    // Empty / minimal HTML
    // ----------------------------------------------------------------

    @Test
    fun emptyHtml_parsesWithoutCrash() {
        LexSoup.parse("").use { d ->
            assertNotNull(d)
            assertEquals("", d.title())
        }
    }

    @Test
    fun fragmentHtml_parsesWithoutCrash() {
        LexSoup.parse("<p>Hello</p>").use { d ->
            d.select("p").use { els ->
                assertEquals(1, els.size())
                assertEquals("Hello", els.get(0).text())
            }
        }
    }
}
