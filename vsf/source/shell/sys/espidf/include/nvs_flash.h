/*============================ INCLUDES ======================================*/

#ifndef __VSF_ESPIDF_NVS_FLASH_H__
#define __VSF_ESPIDF_NVS_FLASH_H__

#include "nvs.h"
#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

// ---------------------------------------------------------------------------
//  Initialize
// ---------------------------------------------------------------------------

/**
 * @brief Initialize the default NVS partition (label "nvs").
 */
esp_err_t nvs_flash_init(void);

/**
 * @brief Initialize NVS storage for a named partition.
 *
 * @param partition_label  Label of the partition (max 16 chars).
 */
esp_err_t nvs_flash_init_partition(const char *partition_label);

/**
 * @brief Initialize NVS storage from an esp_partition_t pointer.
 *
 * @param partition  Partition descriptor obtained from esp_partition API.
 */
esp_err_t nvs_flash_init_partition_ptr(const esp_partition_t *partition);

// ---------------------------------------------------------------------------
//  Deinitialize
// ---------------------------------------------------------------------------

/**
 * @brief Deinitialize the default NVS partition.
 */
esp_err_t nvs_flash_deinit(void);

/**
 * @brief Deinitialize NVS storage for a named partition.
 */
esp_err_t nvs_flash_deinit_partition(const char *partition_label);

// ---------------------------------------------------------------------------
//  Erase
// ---------------------------------------------------------------------------

/**
 * @brief Erase the default NVS partition (label "nvs").
 *
 * If the partition is initialised, it is first deinitialised.
 */
esp_err_t nvs_flash_erase(void);

/**
 * @brief Erase a named NVS partition.
 *
 * If the partition is initialised, it is first deinitialised.
 */
esp_err_t nvs_flash_erase_partition(const char *part_name);

/**
 * @brief Erase a partition identified by pointer.
 *
 * If the partition is initialised, it is first deinitialised.
 */
esp_err_t nvs_flash_erase_partition_ptr(const esp_partition_t *partition);

#ifdef __cplusplus
}
#endif

#endif      /* __VSF_ESPIDF_NVS_FLASH_H__ */
