#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "sntp.h"
#include "simple_SNTP.h"
#include <time.h>

void sntp_service_init(ntp_time* ntp_time){
    static char* SNTP = "SNTP_INIT";
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
    esp_netif_sntp_start();

    setenv("TZ", "UTC-7", 1);
    tzset();

    ntp_time->synced = false;
    while(ntp_time->now < 1744273248)
    {
        time(&ntp_time->now);
        ESP_LOGW(SNTP, "Waiting for system time to be set...");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    char strftime_buf[64];
    time(&ntp_time->now);
    localtime_r(&ntp_time->now, &ntp_time->timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &ntp_time->timeinfo);

    ESP_LOGI(SNTP, "The current date/time in UTC+7 is: %s", strftime_buf);
    ntp_time->synced = true;
}
