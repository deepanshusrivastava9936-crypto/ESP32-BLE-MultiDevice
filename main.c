#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"

#define MAX_DEVICES 5

char *TAG = "BLE-Server";
uint8_t ble_addr_type;
static uint16_t device_map[MAX_DEVICES] = {0}; // conn_handle for each device
void ble_app_advertise(void);

// Write callback
static int device_write(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char temp_buf[ctxt->om->om_len + 1];
    memcpy(temp_buf, ctxt->om->om_data, ctxt->om->om_len);
    temp_buf[ctxt->om->om_len] = '\0';

    // Find device number
    int dev_num = 0;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_map[i] == conn_handle) {
            dev_num = i + 1;
            break;
        }
    }

    printf("Device %d wrote: %s\n", dev_num, temp_buf);

    // Handle commands
    if (strcmp(temp_buf, "LIGHT ON") == 0) { printf("LIGHT ON\n"); }
    else if (strcmp(temp_buf, "LIGHT OFF") == 0) { printf("LIGHT OFF\n"); }
    else if (strcmp(temp_buf, "FAN ON") == 0) { printf("FAN ON\n"); }
    else if (strcmp(temp_buf, "FAN OFF") == 0) { printf("FAN OFF\n"); }

    return 0;
}

// Read callback
static int device_read(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    os_mbuf_append(ctxt->om, "Data from the server", strlen("Data from the server"));
    return 0;
}

// GATT services
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x180A),
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0xFEF4),
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0xDEAD),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {0}}},
    {0}};

// GAP event handler
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            uint16_t conn_handle = event->connect.conn_handle;
            int dev_num = 0;

            // Check if handle already exists
            int found = 0;
            for (int i = 0; i < MAX_DEVICES; i++) {
                if (device_map[i] == conn_handle) {
                    dev_num = i + 1;
                    found = 1;
                    break;
                }
            }

            // Assign first empty slot if new
            if (!found) {
                for (int i = 0; i < MAX_DEVICES; i++) {
                    if (device_map[i] == 0) {
                        device_map[i] = conn_handle;
                        dev_num = i + 1;
                        break;
                    }
                }
            }

            ESP_LOGI(TAG, "Device %d connected, conn_handle=%d", dev_num, conn_handle);
            ble_app_advertise();
        } else {
            ESP_LOGI(TAG, "BLE GAP EVENT CONNECT FAILED");
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
    {
        uint16_t conn_handle = event->disconnect.conn.conn_handle;
        int dev_num = 0;

        // Clear device map
        for (int i = 0; i < MAX_DEVICES; i++) {
            if (device_map[i] == conn_handle) {
                device_map[i] = 0;
                dev_num = i + 1;
                break;
            }
        }

        ESP_LOGI(TAG, "Device %d disconnected, conn_handle=%d", dev_num, conn_handle);
        ble_app_advertise();
    }
    break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "BLE GAP EVENT ADV_COMPLETE");
        ble_app_advertise();
        break;

    default:
        break;
    }
    return 0;
}

// Advertise
void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

// On sync
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
}

// Host task
void host_task(void *param)
{
    nimble_port_run();
}

// Main
void app_main()
{
    nvs_flash_init();
    nimble_port_init();

    ble_svc_gap_device_name_set("BLE-Server");
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    nimble_port_freertos_init(host_task);
}