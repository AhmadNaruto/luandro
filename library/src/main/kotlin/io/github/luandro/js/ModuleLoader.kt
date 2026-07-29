package io.github.luandro.js

/**
 * Phase 7: QuickJS Engine — Module loader callback.
 *
 * Called by the native engine when an ES `import` statement is resolved.
 * @param moduleName  The bare module specifier (e.g., "./util.js")
 * @param baseName    The filename of the importing module
 * @return            The source code of the module, or empty/null to signal not found
 */
fun interface ModuleLoader {
    fun load(moduleName: String, baseName: String): String?
}
