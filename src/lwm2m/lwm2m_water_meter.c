/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include <math.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include "ucifi_water_meter.h"
#include "lwm2m_app_utils.h"
#include "meter_sensor.h"
#include "water_meter_event.h"

LOG_MODULE_REGISTER(lwm2m_water_meter,CONFIG_APP_LOG_LEVEL);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
#define METER_TYPE "water meter"
#else
#define METER_TYPE "pulse type"
#endif

#define SENSOR_UNIT_NAME "m/s^3"


#define MIN_RANGE_VALUE     0.1
#define MAX_RANGE_VALUE     30.0


/* Allocate data cache storage */
static struct lwm2m_time_series_elem meter_volume_cache[10];

int lwm2m_init_water_meter(void)
{
	double min_range_val = MIN_RANGE_VALUE;
	double max_range_val = MAX_RANGE_VALUE;

	water_meter_init();

	lwm2m_create_object_inst(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0));
	lwm2m_set_f64(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0, WATER_METER_MINIMUM_FLOW_RATE_RID),
		        min_range_val);
	lwm2m_set_f64(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0, WATER_METER_MAXIMUM_FLOW_RATE_RID),
		        max_range_val);
    lwm2m_set_res_buf(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0,
                WATER_METER_TYPE_RID),
                METER_TYPE, sizeof(METER_TYPE),
                sizeof(METER_TYPE), LWM2M_RES_DATA_FLAG_RW);

	regitster_clean_meter_data_cb(clean_meter_data);

	/* Enable data cache */
	// lwm2m_engine_enable_cache(LWM2M_PATH(UCIFI_OBJECT_WATER_METER_ID, 0, WATER_METER_CUMULATED_WATER_VOLUME_RID),
    //     meter_volume_cache, ARRAY_SIZE(meter_volume_cache));

	lwm2m_enable_cache(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0, WATER_METER_CUMULATED_WATER_VOLUME_RID),
        meter_volume_cache, ARRAY_SIZE(meter_volume_cache));
	
	return 0;
}



void update_water_meter_value(struct meter_data val)
{
	lwm2m_set_f64(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0, WATER_METER_CUMULATED_WATER_VOLUME_RID), val.volume);
	lwm2m_set_u32(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0, WATER_METER_CUMULATED_PULSE_VALUE_RID), val.pulse_value);
	LOG_DBG("Update meter volume:%lf\t total_pulse=%d\n", val.volume, val.pulse_value);
}





extern void send_leak_detection_alert(void);
static bool water_meter_event_handler(const struct app_event_header *aeh)
{
	if (is_water_meter_event(aeh)) 
	{
		struct water_meter_event *event = cast_water_meter_event(aeh);
		if (event->type == LEAK_DETECTION_ALARM) 
		{
			lwm2m_set_bool(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0, WATER_METER_LEAK_DETECTE_RID), true);
		}
		else
		{
			lwm2m_set_bool(&LWM2M_OBJ(UCIFI_OBJECT_WATER_METER_ID, 0, WATER_METER_LEAK_DETECTE_RID), false);
		}

		send_leak_detection_alert();

		return true;
	}
	else
	{
		return false;
	}
}



APP_EVENT_LISTENER(water_meter_alarm, water_meter_event_handler);
APP_EVENT_SUBSCRIBE(water_meter_alarm, water_meter_event);
