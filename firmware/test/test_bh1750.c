/**
 * @file    test_bh1750.c
 * @brief   Tests unitaires pour le driver BH1750FVI
 */

#include "unity.h"
#include "bh1750.h"

/* Déclarations des stubs nécessaires */
void delay_ms(uint32_t ms);
void uart_send_string(const char *str);
void uart_send_int(int val);
void systick_init(void);

/* Prototypes des fonctions de test */
void test_bh1750_calculate_lux_should_return_correct_value(void);
void test_bh1750_calculate_lux_minimum_value(void);
void test_bh1750_calculate_lux_maximum_value(void);
void test_bh1750_calculate_lux_calculation_precision(void);
void test_bh1750_calculate_lux_zero_raw(void);
void test_bh1750_calculate_lux_formula(void);

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_bh1750_calculate_lux_should_return_correct_value);
    RUN_TEST(test_bh1750_calculate_lux_minimum_value);
    RUN_TEST(test_bh1750_calculate_lux_maximum_value);
    RUN_TEST(test_bh1750_calculate_lux_calculation_precision);
    RUN_TEST(test_bh1750_calculate_lux_zero_raw);
    RUN_TEST(test_bh1750_calculate_lux_formula);
    
    return UNITY_END();
}

/* ====================================================================
   Tests pour bh1750_calculate_lux()
   ==================================================================== */

void test_bh1750_calculate_lux_should_return_correct_value(void)
{
    /* raw = 1200, lux = 1200 * 10 / 12 = 1000 */
    uint16_t lux = bh1750_calculate_lux(0x04, 0xB0); /* 1200 = 0x04B0 */
    TEST_ASSERT_EQUAL_UINT16(1000, lux);
}

void test_bh1750_calculate_lux_minimum_value(void)
{
    /* raw = 0, lux = 0 */
    uint16_t lux = bh1750_calculate_lux(0x00, 0x00);
    TEST_ASSERT_EQUAL_UINT16(0, lux);
}

void test_bh1750_calculate_lux_maximum_value(void)
{
    /* raw = 65535, lux = 65535 * 10 / 12 = 54612 */
    uint16_t lux = bh1750_calculate_lux(0xFF, 0xFF);
    TEST_ASSERT_EQUAL_UINT16(54612, lux);
}

void test_bh1750_calculate_lux_calculation_precision(void)
{
    /* Test de précision : raw = 1200 */
    uint16_t lux = bh1750_calculate_lux(0x04, 0xB0); /* 1200 = 0x04B0 */
    TEST_ASSERT_EQUAL_UINT16(1000, lux);
}

void test_bh1750_calculate_lux_zero_raw(void)
{
    /* raw = 0 */
    uint16_t lux = bh1750_calculate_lux(0x00, 0x00);
    TEST_ASSERT_EQUAL_UINT16(0, lux);
}

void test_bh1750_calculate_lux_formula(void)
{
    /* Vérifier la formule : lux = raw * 10 / 12 */
    struct {
        uint16_t raw;
        uint16_t expected_lux;
    } test_cases[] = {
        {1200, 1000},   /* 1200 * 10 / 12 = 1000 */
        {2400, 2000},   /* 2400 * 10 / 12 = 2000 */
        {120, 100},     /* 120 * 10 / 12 = 100 */
        {12, 10},       /* 12 * 10 / 12 = 10 */
    };
    
    for (int i = 0; i < 4; i++) {
        uint8_t msb = (test_cases[i].raw >> 8) & 0xFF;
        uint8_t lsb = test_cases[i].raw & 0xFF;
        
        uint16_t lux = bh1750_calculate_lux(msb, lsb);
        TEST_ASSERT_EQUAL_UINT16(test_cases[i].expected_lux, lux);
    }
}
