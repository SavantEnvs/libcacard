/*
 * apdu_probe — a tiny dynamically-linked known-answer driver for libcacard's
 * 7816 APDU parser (src/card_7816.c, vcard_apdu_new). Used ONLY by
 * mayhem/test.sh as the behavioral oracle: it links the REAL project code
 * (NORMAL flags, no sanitizer), decodes a fixed APDU passed as hex on the
 * command line, and prints the requested decoded field. test.sh asserts the
 * EXACT decoded value, so neutering the parser (or the verify-repo sabotage
 * shim that _exit(0)s the probe) prints nothing and every assertion FAILS.
 *
 *   usage: apdu_probe <field> <hex-apdu>
 *   field: status|cla|ins|p1|p2|lc|le|type|gentype|channel
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vcard.h"
#include "card_7816.h"

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <field> <hex-apdu>\n", argv[0]);
        return 2;
    }

    const char *field = argv[1];
    const char *hs = argv[2];
    int n = (int) (strlen(hs) / 2);
    unsigned char *buf = malloc(n ? n : 1);
    if (!buf) return 3;
    for (int i = 0; i < n; i++)
        buf[i] = (unsigned char) ((hexval(hs[2 * i]) << 4) | hexval(hs[2 * i + 1]));

    vcard_7816_status_t status = 0;
    VCardAPDU *apdu = vcard_apdu_new(buf, n, &status);

    if (!apdu) {
        /* Parser rejected the input; only the status field is meaningful. */
        if (!strcmp(field, "status")) printf("0x%04x\n", (unsigned) status);
        else printf("(rejected)\n");
        free(buf);
        return 0;
    }

    if      (!strcmp(field, "status"))  printf("0x%04x\n", (unsigned) status);
    else if (!strcmp(field, "cla"))     printf("0x%02x\n", apdu->a_cla);
    else if (!strcmp(field, "ins"))     printf("0x%02x\n", apdu->a_ins);
    else if (!strcmp(field, "p1"))      printf("0x%02x\n", apdu->a_p1);
    else if (!strcmp(field, "p2"))      printf("0x%02x\n", apdu->a_p2);
    else if (!strcmp(field, "lc"))      printf("%d\n", apdu->a_Lc);
    else if (!strcmp(field, "le"))      printf("%d\n", apdu->a_Le);
    else if (!strcmp(field, "type"))    printf("0x%02x\n", apdu->a_type);
    else if (!strcmp(field, "gentype")) printf("%d\n", (int) apdu->a_gen_type);
    else if (!strcmp(field, "channel")) printf("%d\n", apdu->a_channel);
    else                                printf("(unknown-field)\n");

    vcard_apdu_delete(apdu);
    free(buf);
    return 0;
}
