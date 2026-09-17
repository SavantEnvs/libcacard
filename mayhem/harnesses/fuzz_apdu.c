/*
 * libFuzzer harness for libcacard's 7816 APDU parser (src/card_7816.c).
 *
 * vcard_apdu_new() is the code that turns the raw bytes an untrusted
 * card-reader sends into a decoded VCardAPDU (class / channel / secure
 * messaging + the 7816-4 Lc/Le length decoding, incl. the extended-length
 * cases). It is the first thing every incoming command hits, so it is the
 * natural fuzz surface for hostile reader input.
 *
 * Byte-in only, no file I/O and no NSS/emulator init: the fuzzer bytes are
 * handed straight to vcard_apdu_new() as the raw APDU buffer, and the parsed
 * structure is freed with vcard_apdu_delete(). A copy is made so the parser
 * never sees a non-NUL-terminated / read-only buffer differently than in
 * production (it g_memdup2()s internally anyway).
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "vcard.h"
#include "card_7816.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > INT_MAX)
        return 0;

    /* Hand the fuzzer bytes to the parser as a mutable raw APDU buffer. */
    unsigned char *buf = (unsigned char *) malloc(size ? size : 1);
    if (!buf)
        return 0;
    if (size)
        memcpy(buf, data, size);

    vcard_7816_status_t status = 0;
    VCardAPDU *apdu = vcard_apdu_new(buf, (int) size, &status);
    if (apdu)
        vcard_apdu_delete(apdu);

    free(buf);
    return 0;
}
