package io.github.luandro.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.github.luandro.js.JS
import io.github.luandro.lexsoup.LexSoup
import io.github.luandro.luau.Luau
import io.github.luandro.regex.Regex

class MainActivity : ComponentActivity() {
    companion object {
        init {
            System.loadLibrary("luandro_nrp")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme(
                colorScheme = darkColorScheme(
                    primary = Color(0xFF8AB4F8),
                    secondary = Color(0xFFC7D2FE),
                    surface = Color(0xFF1E1E2E),
                    background = Color(0xFF121218)
                )
            ) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    NrpDemoApp()
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun NrpDemoApp() {
    var selectedTab by remember { mutableIntStateOf(0) }
    val tabs = listOf("🥣 LexSoup", "🔤 Regex", "⚡ QuickJS", "🌙 Luau VM")

    Column(modifier = Modifier.fillMaxSize()) {
        TopAppBar(
            title = { Text("Luandro NRP — Multi-Engine Demo", fontSize = 18.sp) },
            colors = TopAppBarDefaults.topAppBarColors(
                containerColor = MaterialTheme.colorScheme.surface,
                titleContentColor = Color.White
            )
        )

        TabRow(selectedTabIndex = selectedTab) {
            tabs.forEachIndexed { index, title ->
                Tab(
                    selected = selectedTab == index,
                    onClick = { selectedTab = index },
                    text = { Text(title) }
                )
            }
        }

        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp)
        ) {
            when (selectedTab) {
                0 -> LexSoupTab()
                1 -> RegexTab()
                2 -> QuickJSTab()
                3 -> LuauTab()
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Tab 1: LexSoup (HTML5 Parser)
// -----------------------------------------------------------------------------
@Composable
fun LexSoupTab() {
    var htmlInput by remember {
        mutableStateOf(
            "<!DOCTYPE html>\n<html><body>\n  <h1 id='title'>Hello World</h1>\n  <p class='intro'>LexSoup DOM Parser in Action!</p>\n</body></html>"
        )
    }
    var resultText by remember { mutableStateOf("") }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("HTML Input", style = MaterialTheme.typography.titleMedium)
        OutlinedTextField(
            value = htmlInput,
            onValueChange = { htmlInput = it },
            modifier = Modifier.fillMaxWidth(),
            maxLines = 6
        )

        Button(
            onClick = {
                try {
                    LexSoup.parse(htmlInput).use { doc ->
                        val title = doc.title()
                        val h1Text = doc.getElementById("title")?.text() ?: "N/A"
                        val paragraphCount = doc.getElementsByClass("intro").use { it.size() }
                        resultText = "Document Title: '$title'\n" +
                                "#title element text: '$h1Text'\n" +
                                ".intro elements count: $paragraphCount\n" +
                                "Full document text: '${doc.text()}'"
                    }
                } catch (e: Exception) {
                    resultText = "Error: ${e.message}"
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Parse HTML DOM")
        }

        if (resultText.isNotEmpty()) {
            Text("Output:", style = MaterialTheme.typography.titleSmall)
            Text(
                text = resultText,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier
                    .fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surface)
                    .padding(12.dp)
            )
        }
    }
}

// -----------------------------------------------------------------------------
// Tab 2: Regex Engine
// -----------------------------------------------------------------------------
@Composable
fun RegexTab() {
    var patternInput by remember { mutableStateOf("\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Z|a-z]{2,}\\b") }
    var textInput by remember { mutableStateOf("Contact support@example.com or info@luandro.io for info.") }
    var resultText by remember { mutableStateOf("") }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("RegExp Pattern", style = MaterialTheme.typography.titleMedium)
        OutlinedTextField(
            value = patternInput,
            onValueChange = { patternInput = it },
            modifier = Modifier.fillMaxWidth()
        )

        Text("Text Input", style = MaterialTheme.typography.titleMedium)
        OutlinedTextField(
            value = textInput,
            onValueChange = { textInput = it },
            modifier = Modifier.fillMaxWidth()
        )

        Button(
            onClick = {
                try {
                    Regex.compile(patternInput).use { p ->
                        val matches = p.findAll(textInput)
                        val matchedValues = matches.map { m ->
                            val v = m.value
                            m.close()
                            v
                        }
                        resultText = "Matches found: ${matchedValues.size}\n" +
                                "Matches: $matchedValues\n" +
                                "Replace result: ${p.replaceAll(textInput, "[EMAIL]")}"
                    }
                } catch (e: Exception) {
                    resultText = "Error: ${e.message}"
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Run Regex Search & Replace")
        }

        if (resultText.isNotEmpty()) {
            Text("Output:", style = MaterialTheme.typography.titleSmall)
            Text(
                text = resultText,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier
                    .fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surface)
                    .padding(12.dp)
            )
        }
    }
}

// -----------------------------------------------------------------------------
// Tab 3: QuickJS-NG JavaScript Engine
// -----------------------------------------------------------------------------
@Composable
fun QuickJSTab() {
    var jsCode by remember {
        mutableStateOf(
            "const data = [10, 20, 30, 40];\nconst sum = data.reduce((a, b) => a + b, 0);\n'Sum is: ' + sum;"
        )
    }
    var resultText by remember { mutableStateOf("") }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("JavaScript Code", style = MaterialTheme.typography.titleMedium)
        OutlinedTextField(
            value = jsCode,
            onValueChange = { jsCode = it },
            modifier = Modifier.fillMaxWidth(),
            maxLines = 6
        )

        Button(
            onClick = {
                try {
                    JS.createRuntime().use { rt ->
                        rt.newContext().use { ctx ->
                            ctx.eval(jsCode).use { valResult ->
                                resultText = "Result Type: ${if (valResult.isString()) "String" else "Value"}\n" +
                                        "Evaluation Output: ${valResult}"
                            }
                        }
                    }
                } catch (e: Exception) {
                    resultText = "JS Error: ${e.message}"
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Execute JavaScript")
        }

        if (resultText.isNotEmpty()) {
            Text("Output:", style = MaterialTheme.typography.titleSmall)
            Text(
                text = resultText,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier
                    .fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surface)
                    .padding(12.dp)
            )
        }
    }
}

// -----------------------------------------------------------------------------
// Tab 4: Luau VM Engine
// -----------------------------------------------------------------------------
@Composable
fun LuauTab() {
    var luauCode by remember {
        mutableStateOf(
            "local function fib(n)\n  if n <= 1 then return n end\n  return fib(n-1) + fib(n-2)\nend\nreturn 'fib(10) = ' .. tostring(fib(10))"
        )
    }
    var resultText by remember { mutableStateOf("") }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("Luau Code", style = MaterialTheme.typography.titleMedium)
        OutlinedTextField(
            value = luauCode,
            onValueChange = { luauCode = it },
            modifier = Modifier.fillMaxWidth(),
            maxLines = 6
        )

        Button(
            onClick = {
                try {
                    Luau.createVM().use { vm ->
                        val result = vm.execute(luauCode)
                        resultText = "Evaluation Output: $result"
                    }
                } catch (e: Exception) {
                    resultText = "Luau Error: ${e.message}"
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Execute Luau Script")
        }

        if (resultText.isNotEmpty()) {
            Text("Output:", style = MaterialTheme.typography.titleSmall)
            Text(
                text = resultText,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier
                    .fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surface)
                    .padding(12.dp)
            )
        }
    }
}
