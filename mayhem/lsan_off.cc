/*
 * mayhem/lsan_off.cc — build-time LeakSanitizer off-switch (fleet policy).
 *
 * -fsanitize=address always bundles LSan in; there is no flag to keep ASan
 * while dropping only leak detection. Defining this hook in a linked TU tells
 * the sanitizer runtime, at startup, that leak checking is off — ASan's
 * memory-corruption checks and UBSan stay on and halting. Leaks are not the
 * bug class this fleet fuzzes for.
 *
 * Compiled with $SANITIZER_FLAGS $DEBUG_FLAGS by mayhem/build.sh and linked
 * into EVERY ASan-built binary (fuzz_apdu and fuzz_apdu-standalone). This is
 * the only sanctioned mechanism: no runtime option overrides, no runtime
 * enable/disable calls, no Mayhemfile option lines.
 */
extern "C" int __lsan_is_turned_off(void)
{
    return 1;
}
