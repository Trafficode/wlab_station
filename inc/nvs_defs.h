/* ---------------------------------------------------------------------------
 *  nvs
 * ---------------------------------------------------------------------------
 *  Name: nvs_defs.h
 * --------------------------------------------------------------------------*/
#ifndef NVS_DEFS_H_
#define NVS_DEFS_H_

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define NVS_PARTITION_SIZE   FIXED_PARTITION_SIZE(NVS_PARTITION)

#define NVS_ID_BOOT_COUNT (1)
#define NVS_ID_TIMESTAMP  (2)

#endif /* NVS_DEFS_H_ */
/* ---------------------------------------------------------------------------
 * end of file
 * --------------------------------------------------------------------------*/