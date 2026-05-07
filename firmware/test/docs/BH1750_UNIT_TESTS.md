# Tests unitaires du driver BH1750FVI

## Architecture des tests

```
firmware/test/
├── docs/
│   └── BH1750_UNIT_TESTS.md      ← Ce document
├── mock_hw.h                      ← Simulation des registres STM32
├── mock_hw.c                      ← Implémentation du mock matériel
├── test_bh1750.c                  ← Tests unitaires BH1750
├── test_stubs.c                   ← Stubs pour les dépendances (delay_ms, UART)
│
├── mock/                          ← (réservé) Mocks Ceedling pour futur déploiement
└── src/                           ← (réservé) Sources de test additionnels

firmware/
├── src/bh1750.c                   ← Driver modifié : ajout de bh1750_calculate_lux()
├── include/bh1750.h               ← Déclaration de bh1750_calculate_lux()
├── Makefile.test                  ← Build des tests (gcc hôte, pas cross-compilation)
├── project.yml                    ← Configuration Ceedling (futur)
├── Unity/                         ← Submodule : framework Unity (ThrowTheSwitch/Unity)
└── Makefile                       ← Build normal (ARM cross-compilation, inchangé)
```

---

## Principe général

Les tests sont exécutés **sur la machine hôte (PC)** et non sur le STM32. Pour cela :

1. Les registres matériels STM32 (`RCC_AHB1ENR`, `I2C1_CR1`, etc.) sont simulés par des **variables globales** dans `mock_hw.h` / `mock_hw.c`
2. Les fonctions dépendantes du matériel (`delay_ms()`, `uart_send_string()`) sont remplacées par des **stubs** vides dans `test_stubs.c`
3. Le mode `TEST_HOST` (passé avec `-DTEST_HOST`) active les mocks et désactive le code matériel réel via `#ifdef TEST_HOST`

---

## Ce qui est testé

### Fonction `bh1750_calculate_lux(msb, lsb)` — 6 cas de test

Cette fonction extraite de `bh1750_read_lux()` implémente le calcul :

```
lux = (raw * 10) / 12    où  raw = (msb << 8) | lsb
```

| Test | Entrée (msb, lsb) | raw | Résultat attendu | Vérifie |
|------|-------------------|-----|------------------|---------|
| `should_return_correct_value` | 0x05, 0xA0 | 1440 | **1200 lux** | Valeur nominale |
| `minimum_value` | 0x00, 0x00 | 0 | **0 lux** | Cas limite bas |
| `maximum_value` | 0xFF, 0xFF | 65535 | **54612 lux** | Cas limite haut |
| `calculation_precision` | 0x04, 0xB0 | 1200 | **1000 lux** | Précision du calcul |
| `zero_raw` | 0x00, 0x00 | 0 | **0 lux** | Redondant (minimum) |
| `formula` | [1200, 2400, 120, 12] | — | [1000, 2000, 100, 10] | Formule validée sur 4 valeurs |

---

## Ce qui reste à tester (mock I2C à finaliser)

### Fonction `bh1750_init()` — 4 cas pré-écrits

Les tests suivants sont déjà rédigés dans `test_bh1750.c` mais **désactivés** car le mock I2C ne simule pas encore le protocole suffisamment pour que le driver termine sa séquence :

| Test | Vérifie |
|------|---------|
| `init_should_enable_clocks` | `RCC_AHB1ENR` bit 1 (GPIOB) et `RCC_APB1ENR` bit 21 (I2C1) activés |
| `init_should_configure_gpio_pins` | PB6/PB7 en Alternate Function 4, open-drain, pull-up |
| `init_should_enable_i2c_peripheral` | `I2C1_CR1` bit 0 (PE) activé |
| `init_should_send_power_on_command` | Commande `BH1750_POWER_ON` (0x01) envoyée sur le bus I2C |

### Problème à résoudre

Le driver `bh1750_init()` exécute une séquence I2C complète :
```
i2c_start() → i2c_send_addr() → i2c_write_byte(POWER_ON) → i2c_stop()
```

Chaque fonction attend des flags précis dans `I2C1_SR1` (`I2C_SR1_SB`, `I2C_SR1_ADDR`, etc.). Pour que le test passe, le mock I2C doit :
1. Mettre le flag `SB` après `I2C_CR1_START`
2. Mettre le flag `ADDR` après écriture de l'adresse dans `I2C1_DR`
3. Mettre les flags `TXE` + `BTF` après écriture de la commande dans `I2C1_DR`

---

## Build et exécution

```bash
# Depuis firmware/
make -f Makefile.test test

# Nettoyage
make -f Makefile.test clean

# Compilation seule (sans exécution)
make -f Makefile.test build/test/bh1750_tests
```

### Exemple de sortie

```
test_bh1750_calculate_lux_should_return_correct_value:PASS
test_bh1750_calculate_lux_minimum_value:PASS
...
-----------------------
6 Tests 0 Failures 0 Ignored
OK
```

---

## Dépendances

| Dépendance | Rôle | Installation |
|------------|------|--------------|
| **Unity** | Framework de tests unitaires | Submodule git (`firmware/Unity/`) |
| **gcc** | Compilateur hôte | `apt install gcc` |
| **make** | Build system | `apt install make` |

---

## Ajout de nouveaux tests

1. Créer une fonction de test dans `test_bh1750.c` avec le prototype `void test_ma_fonction(void)`
2. Ajouter `RUN_TEST(test_ma_fonction)` dans le `main()`
3. Si besoin de mocker un nouveau registre, l'ajouter dans `mock_hw.h` et `mock_hw.c`
4. Exécuter : `make -f Makefile.test test`

---

## Annexe : Registres simulés

| Macro (code) | Variable mock | Registre STM32 simulé |
|-------------|---------------|----------------------|
| `RCC_AHB1ENR` | `mock_RCC_AHB1ENR` | RCC AHB1 Enable |
| `RCC_APB1ENR` | `mock_RCC_APB1ENR` | RCC APB1 Enable |
| `GPIOB_MODER` | `mock_GPIOB_MODER` | GPIOB Mode |
| `GPIOB_OTYPER` | `mock_GPIOB_OTYPER` | GPIOB Output Type |
| `GPIOB_OSPEEDR` | `mock_GPIOB_OSPEEDR` | GPIOB Speed |
| `GPIOB_PUPDR` | `mock_GPIOB_PUPDR` | GPIOB Pull-up/down |
| `GPIOB_AFRL` | `mock_GPIOB_AFRL` | GPIOB Alternate Function Low |
| `GPIOB_ODR` | `mock_GPIOB_ODR` | GPIOB Output Data |
| `I2C1_CR1` | `mock_I2C1_CR1` | I2C1 Control 1 |
| `I2C1_CR2` | `mock_I2C1_CR2` | I2C1 Control 2 |
| `I2C1_DR` | `mock_I2C1_DR` | I2C1 Data |
| `I2C1_SR1` | `mock_I2C1_SR1` | I2C1 Status 1 |
| `I2C1_SR2` | `mock_I2C1_SR2` | I2C1 Status 2 |
| `I2C1_CCR` | `mock_I2C1_CCR` | I2C1 Clock Control |
| `I2C1_TRISE` | `mock_I2C1_TRISE` | I2C1 TRISE |

---

## Annexe : Fichiers modifiés par rapport à l'original

| Fichier | Modification |
|---------|-------------|
| `src/bh1750.c` | Ajout de `bh1750_calculate_lux()` + guard `#ifdef TEST_HOST` |
| `include/bh1750.h` | Déclaration de `bh1750_calculate_lux()` |
| `include/uart.h` | Guard `#ifdef TEST_HOST` pour éviter conflit registres |
| (créé) `test/` | Dossier complet : tests, mocks, stubs |
| (créé) `Makefile.test` | Build et exécution des tests sur hôte |
| (créé) `project.yml` | Configuration Ceedling |
| (créé) `Unity/` | Submodule Unity |
| (créé) `.gitmodules` | Déclaration du submodule |
