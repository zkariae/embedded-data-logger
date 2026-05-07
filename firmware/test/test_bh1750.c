/**
 * @file    test_bh1750.c
 * @brief   Tests unitaires pour le driver BH1750FVI
 */

#include "unity.h"
#include "bh1750.h"
#include "mock_hw.h"

/* Stubs pour les dépendances */
void delay_ms(uint32_t ms) { (void)ms; }
void uart_send_string(const char *str) { (void)str; }
void uart_send_int(int val) { (void)val; }
void systick_init(void) {}

void setUp(void)
{
    mock_i2c_reset();
    mock_i2c_simulate_ack();
}

void tearDown(void)
{
}

/* ====================================================================
   Tests pour bh1750_read_lux()
   ==================================================================== */

void test_bh1750_read_lux_should_return_correct_lux_value(void)
{
    /* Simuler la lecture de 1200 lux */
    /* raw = 1200 * 12 / 10 = 1440 = 0x05A0 */
    uint8_t msb = 0x05;
    uint8_t lsb = 0xA0;
    mock_i2c_set_lux_data(msb, lsb);
    
    uint16_t lux = bh1750_read_lux();
    
    /* lux = (0x05A0 * 10) / 12 = 14400 / 12 = 1200 */
    TEST_ASSERT_EQUAL_UINT16(1200, lux);
}

void test_bh1750_read_lux_minimum_value(void)
{
    /* Test valeur minimale : 0 lux */
    mock_i2c_set_lux_data(0x00, 0x00);
    
    uint16_t lux = bh1750_read_lux();
    TEST_ASSERT_EQUAL_UINT16(0, lux);
}

void test_bh1750_read_lux_maximum_value(void)
{
    /* Test valeur maximale : 65535 raw */
    mock_i2c_set_lux_data(0xFF, 0xFF);
    
    uint16_t lux = bh1750_read_lux();
    /* lux = (65535 * 10) / 12 = 54612 */
    TEST_ASSERT_EQUAL_UINT16(54612, lux);
}

void test_bh1750_read_lux_calculation_precision(void)
{
    /* Test de précision : raw = 1200 */
    mock_i2c_set_lux_data(0x04, 0xB0); /* 1200 en hex */
    
    uint16_t lux = bh1750_read_lux();
    /* lux = 1200 * 10 / 12 = 1000 */
    TEST_ASSERT_EQUAL_UINT16(1000, lux);
}

void test_bh1750_read_lux_returns_zero_on_i2c_error(void)
{
    /* Simuler une erreur I2C (NACK) */
    mock_i2c_simulate_nack();
    
    uint16_t lux = bh1750_read_lux();
    TEST_ASSERT_EQUAL_UINT16(0, lux);
}

void test_bh1750_read_lux_verifies_i2c_sequence(void)
{
    /* Vérifier que la séquence I2C est correcte */
    mock_i2c_set_lux_data(0x01, 0x00); /* 256 raw -> 213 lux */
    
    uint16_t lux = bh1750_read_lux();
    
    /* Vérifier que les registres I2C ont été manipulés */
    TEST_ASSERT_TRUE(mock_I2C1_CR1 & (1UL << 8));  /* START généré */
    TEST_ASSERT_TRUE(mock_I2C1_DR == BH1750_ONE_TIME_H_RES_MODE); /* Commande envoyée */
}

/* ====================================================================
   Tests pour bh1750_init()
   ==================================================================== */

void test_bh1750_init_should_enable_clocks(void)
{
    bh1750_init();
    
    /* Vérifier que les horloges GPIOB et I2C1 sont activées */
    TEST_ASSERT_TRUE(mock_RCC_AHB1ENR & (1UL << 1));  /* GPIOB */
    TEST_ASSERT_TRUE(mock_RCC_APB1ENR & (1UL << 21)); /* I2C1 */
}

void test_bh1750_init_should_configure_gpio_pins(void)
{
    bh1750_init();
    
    /* Vérifier PB6 et PB7 en AF4 (Alternate Function 4) */
    uint32_t moder = mock_GPIOB_MODER;
    TEST_ASSERT_EQUAL_UINT32(2UL << 12, moder & (3UL << 12)); /* PB6 = AF */
    TEST_ASSERT_EQUAL_UINT32(2UL << 14, moder & (3UL << 14)); /* PB7 = AF */
    
    /* Vérifier AFRL pour AF4 */
    uint32_t afrl = mock_GPIOB_AFRL;
    TEST_ASSERT_EQUAL_UINT32(4UL << 24, afrl & (0xFUL << 24)); /* PB6 = AF4 */
    TEST_ASSERT_EQUAL_UINT32(4UL << 28, afrl & (0xFUL << 28)); /* PB7 = AF4 */
}

void test_bh1750_init_should_enable_i2c_peripheral(void)
{
    bh1750_init();
    
    /* Vérifier que PE (Peripheral Enable) est activé */
    TEST_ASSERT_TRUE(mock_I2C1_CR1 & (1UL << 0));
}

void test_bh1750_init_should_send_power_on_command(void)
{
    bh1750_init();
    
    /* Vérifier que la commande POWER_ON a été envoyée */
    TEST_ASSERT_EQUAL_UINT32(BH1750_POWER_ON, mock_I2C1_DR);
}

/* ====================================================================
   Tests de non-régression - calculs de lux
   ==================================================================== */

void test_lux_calculation_formula(void)
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
        mock_i2c_set_lux_data(msb, lsb);
        
        uint16_t lux = bh1750_read_lux();
        TEST_ASSERT_EQUAL_UINT16(test_cases[i].expected_lux, lux);
    }
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_bh1750_read_lux_should_return_correct_lux_value);
    RUN_TEST(test_bh1750_read_lux_minimum_value);
    RUN_TEST(test_bh1750_read_lux_maximum_value);
    RUN_TEST(test_bh1750_read_lux_calculation_precision);
    RUN_TEST(test_bh1750_read_lux_returns_zero_on_i2c_error);
    RUN_TEST(test_bh1750_read_lux_verifies_i2c_sequence);
    RUN_TEST(test_bh1750_init_should_enable_clocks);
    RUN_TEST(test_bh1750_init_should_configure_gpio_pins);
    RUN_TEST(test_bh1750_init_should_enable_i2c_peripheral);
    RUN_TEST(test_bh1750_init_should_send_power_on_command);
    RUN_TEST(test_lux_calculation_formula);
    
    return UNITY_END();
}
