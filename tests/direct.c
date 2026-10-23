/*
 * Test resetting a card of type VCARD_DIRECT
 *
 * This code is licensed under the GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */
#include <glib.h>
#include "libcacard.h"

static const unsigned char test_aid[] = { 0xd2, 0x76, 0x00, 0x01, 0x24, 0x01 };

static VCardStatus
test_process_apdu(G_GNUC_UNUSED VCard *card, G_GNUC_UNUSED VCardAPDU *apdu,
                  VCardResponse **response)
{
    *response = vcard_make_response(VCARD7816_STATUS_SUCCESS);
    return VCARD_DONE;
}

/* The applet has no reset function, which vcard_select_applet() accepts */
static void test_reset_applet_without_reset_function(void)
{
    VCard *card;
    VReader *reader;
    VEvent *event;
    unsigned char atr[64];
    int atr_len = sizeof(atr);

    card = vcard_new(NULL, NULL);
    vcard_set_type(card, VCARD_DIRECT);
    vcard_add_applet(card, vcard_new_applet(test_process_apdu, NULL,
                                            test_aid, sizeof(test_aid)));

    vcard_reset(card, VCARD_POWER_ON);

    reader = vreader_new("direct", NULL, NULL);
    vreader_insert_card(reader, card);
    g_assert_cmpint(vreader_power_off(reader), ==, VREADER_OK);
    g_assert_cmpint(vreader_power_on(reader, atr, &atr_len), ==, VREADER_OK);
    vreader_insert_card(reader, NULL);

    vreader_free(reader);
    vcard_free(card);
    while ((event = vevent_get_next_vevent()) != NULL) {
        vevent_delete(event);
    }
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/direct/reset-applet-without-reset-function",
                    test_reset_applet_without_reset_function);

    return g_test_run();
}
