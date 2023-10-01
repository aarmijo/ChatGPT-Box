// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.2.3
// LVGL version: 8.3.4
// Project name: chat_gpt_new_gui

#include "app_sr.h"

#include "ui.h"
#include "app_ui_ctrl.h"
#include "OpenAI.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_log.h"
#include "hal/usb_phy_ll.h"
#include "settings.h"

static char *TAG = "ui-events";

void EventBtnSetupClick(lv_event_t *e)
{
    ui_sleep_show_animation();
}

void EventPanelSleepClickCb(lv_event_t *e)
{
    app_sr_start_once();
    ESP_LOGI(TAG, "sr start once");
}

void EventWifiResetConfirmClick(lv_event_t *e)
{
    static bool started = false;
    if (!started)
    {
        started = true;
        ESP_LOGI(TAG, "Start USB UF2 NVS");
        lv_label_set_text(ui_LabelSetupBtn1, "Reboot");
        uf2_nvs_storage_init(); // Configure USB NVS
    }
    else
    {
        ESP_LOGI(TAG, "Reboot from WIFI Page to Factory Partition");
        usb_phy_ll_int_jtag_enable(&USB_SERIAL_JTAG); // Configure USB PHY, Change back to USB-Serial-Jtag
        esp_restart();
    }
}

void EventSettingsRegionValueChange(lv_event_t *e)
{
    // TODO
}