/* Copyright (c) 2015 Randy Westlund, All rights resrved.
 * This code is under the BSD-2-Clause license
 *
 * The testing suite for embedded-digimesh, using CUnit.
 */
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "digimesh.h"

/* Define some packets that are known to be valid. Taken from the digimesh
 * datasheet */
static uint8_t example_get_at[] = {0x7e, 0x00, 0x04, 0x08, 0x52, 0x4e, 0x48, 0x0f};
static uint8_t example_modem_status[] = {0x7e, 0x00, 0x02, 0x8A, 0x00, 0x75};

/********************************
 *    TEST HELPERS
 *******************************/

/* Test that adding bytes one at a time gives us the proper packet back Used as
 * a helper for verifying the double-buffering */
struct xbee_packet *
add_byte(const uint8_t *data, uint16_t len) {
    /* pass it bytes one at a time, expecting NULL back */
    for (int i = 0; i < len-1; i++) {
        CU_ASSERT_PTR_NULL(xbee_add_byte(data[i]));
    }
    /* passing the last byte should return a packet */
    struct xbee_packet *p = xbee_add_byte(data[len-1]);
    CU_ASSERT_PTR_NOT_NULL(p);
    /* make sure the value and length match */
    CU_ASSERT_FALSE(memcmp(p->buf, data, p->len));
    CU_ASSERT_EQUAL(p->len, len);
    return p;
}

/********************************
 *    TEST FUNCTIONS
 *******************************/

/* Make sure the checksum calculated matches what we expect */
void
test_calc_checksum(void) {
    struct xbee_packet p;
    /* fill packet with example */
    memcpy(p.buf, example_get_at, sizeof(example_get_at));
    p.len = sizeof(example_get_at);

    /* verify the checksum */
    uint8_t checksum =  xbee_calc_checksum(&p);
    CU_ASSERT_EQUAL(example_get_at[sizeof(example_get_at)-1], checksum);
}

/* Run it three times. The first two should get different pointers, but the
 * third should have recycled the first */
void
test_add_byte(void) {
    struct xbee_packet *p1 =
        add_byte(example_modem_status, sizeof(example_modem_status));
    struct xbee_packet *p2 =
        add_byte(example_modem_status, sizeof(example_modem_status));
    struct xbee_packet *p3 =
        add_byte(example_get_at, sizeof(example_get_at));
    CU_ASSERT_PTR_NOT_EQUAL(p1, p2);
    CU_ASSERT_PTR_EQUAL(p1, p3);
}


/* Test that building a command packet gives a parsable object */
void
test_build_command_packet(void) {
    struct xbee_packet p;
    xbee_build_command_packet(&p, example_get_at+5, sizeof(example_get_at)-6);
    /* test it be trying to feed it to the parser */
    add_byte(p.buf, p.len);
    /* make sure length is what we expect */
    CU_ASSERT_EQUAL(p.len, sizeof(example_get_at));
}

/* Test that sending a message to a destination gives a parsable object */
void
test_build_data_packet(void) {
    struct xbee_packet p;
    xbee_build_data_packet(&p, 0x00, (uint8_t *)"HELLO", sizeof("HELLO")-1);
    /* test it be trying to feed it to the parser */
    add_byte(p.buf, p.len);
    /* make sure length is what we expect */
    CU_ASSERT_EQUAL(p.len, 18 + sizeof("HELLO")-1);
}

/* Test whether frame_ids wrap properly */
void
test_get_frame_id(void) {
    struct xbee_packet p;
    /* save our starting frame_id */
    uint8_t first_id =
        xbee_build_data_packet(&p, 0x00, (uint8_t *)"HELLO", sizeof("HELLO")-1);

    /* iterate over the next 253, making sure they add up */
    uint8_t cur_id;
    for (int i = 1; i < 255; i++) {
        cur_id = xbee_build_data_packet(&p, 0x00, (uint8_t *)"HELLO", sizeof("HELLO")-1);

        /* if we've wrapped, we need to make first_id+i skip 0 */
        if (cur_id < first_id) { CU_ASSERT_EQUAL((uint8_t)(first_id + i + 1), cur_id); }
        else { CU_ASSERT_EQUAL(first_id + i, cur_id); }
    }
    /* make sure the 255th one is the same as the starting one */
    uint8_t wrap_id
        = xbee_build_data_packet(&p, 0x00, (uint8_t *)"HELLO", sizeof("HELLO")-1);
    CU_ASSERT_EQUAL(first_id, wrap_id);
}


/* Run the tests */
int
main (void) {
    CU_pSuite pSuite = NULL;
 
    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
       return CU_get_error();
 
    /* add a suite to the registry, other args are for setup and teardown */
    pSuite = CU_add_suite("Suite_1", NULL, NULL);
    if (NULL == pSuite) {
       CU_cleanup_registry();
       return CU_get_error();
    }
 
    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite,
            "test xbee_calc_checksum()", test_calc_checksum))
        || (NULL == CU_add_test(pSuite,
            "test xbee_add_byte()", test_add_byte))
        || (NULL == CU_add_test(pSuite,
            "test xbee_build_command_packet()", test_build_command_packet))
        || (NULL == CU_add_test(pSuite,
            "test xbee_build_data_packet()", test_build_data_packet))
        || (NULL == CU_add_test(pSuite,
            "test get_frame_id()", test_get_frame_id)))
    {
       CU_cleanup_registry();
       return CU_get_error();
    }
 
    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
