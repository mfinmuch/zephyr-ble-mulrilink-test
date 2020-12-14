/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>
#include <host/conn_internal.h>

#define CHAR_SIZE_MAX 32
#define BT_EVAL_UUID_TEST BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef9))
#define BT_EVAL_UUID_TEST_chara BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1))

#define STACKSIZE 1024
int count;
static void start_scan(void);

static struct bt_conn *default_conn;
static struct bt_conn *test_conn[CONFIG_BT_MAX_CONN];

#define MIN_CONNECTION_INTERVAL         72     /**< Determines minimum connection interval in milliseconds, units * 1.25ms */
#define MAX_CONNECTION_INTERVAL         72    /**< Determines maximum connection interval in milliseconds, units * 1.25ms */
#define SLAVE_LATENCY                   0    /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT             50  /**< Determines supervisory timeout, units * 10ms */
static struct bt_le_conn_param *conn_params = BT_LE_CONN_PARAM(MIN_CONNECTION_INTERVAL, MAX_CONNECTION_INTERVAL, SLAVE_LATENCY, SUPERVISION_TIMEOUT);


static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params[CONFIG_BT_MAX_CONN];
static struct bt_gatt_subscribe_params subscribe_params[CONFIG_BT_MAX_CONN];

uint16_t service_handle1,service_handle2,c_flag1=0,i,c_flag2=0,service_handle;
uint8_t p1=0,p2=0;

static bool is_scanning = false;
static bool is_connecting = false;
static int connecting_index = -1;
int test_count=0;
int connect_count=0;

uint32_t start_time;
uint32_t stop_time;
uint32_t stop_time1;
uint32_t cycles_spent;
uint32_t nanoseconds_spent;
uint32_t ack_time;
uint32_t delay[5][100]={0};
uint32_t data1[10]={0};
uint32_t data2[10]={0};
uint32_t datacount[2]={0};
uint8_t array_count;

static int cmd_read(uint16_t handle,uint8_t conn_index);
static struct bt_gatt_read_params read_params;
uint8_t conn_num;
uint8_t service_flag[CONFIG_BT_MAX_CONN] ={0};
uint8_t read_end_flag=0;
uint8_t scan_end =0,found=0;
int full_flag;


/*WDT setting*/

#include <drivers/watchdog.h>
#include <stdbool.h>
#define WDT_FEED_TRIES 5
#if DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_watchdog)
#define WDT_NODE DT_INST(0, nordic_nrf_watchdog)
#endif

#ifdef WDT_NODE
#define WDT_DEV_NAME DT_LABEL(WDT_NODE)
#else
#define WDT_DEV_NAME ""
#error "Unsupported SoC and no watchdog0 alias in zephyr.dts"
#endif
int wdt_channel_id;
const struct device *wdt;
struct wdt_timeout_cfg wdt_config;

/*WDT setting end*/

static void wdt_callback(const struct device *wdt_dev, int channel_id)
{
	static bool handled_event;

	if (handled_event) {
		return;
	}

	wdt_feed(wdt_dev, channel_id);

	printk("Handled things..ready to reset\n");
	handled_event = true;
}

/*timer*/
void my_work_handler(struct k_work *work)
{
	int err;
	printk("test\n");
	err = wdt_setup(wdt, 0);
	if (err < 0) {
		printk("Watchdog setup error\n");
		return;
	}
}
K_WORK_DEFINE(my_work, my_work_handler);

void my_timer_handler(struct k_timer *timer)
{
	printk("int timer %d %u\n",count,timer->status);
	count++;
	if(count==5){
    		//k_work_submit(&my_work);
	}
}
void timer_stop(struct k_timer *timer){
	//printk("stop\n");
	count=0;

}
K_TIMER_DEFINE(my_timer, my_timer_handler, timer_stop);



static struct bt_gatt_write_params write_params[CONFIG_BT_MAX_CONN];
static uint8_t gatt_write_buf[CHAR_SIZE_MAX];

static void write_func(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_write_params *params)
{

	 //printk("Write complete\r\n");
	 //(void)memset(&write_params, 0, sizeof(write_params));

}


static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	uint32_t mantissa;
	int8_t exponent;
	int err;
	int a=0,b=0;
	int unsub_conn = 0;

	start_time = k_uptime_get_32();

	if (!data) {
		unsub_conn = bt_conn_index(conn);
		printk("[UNSUBSCRIBED] %d %d %d\n",unsub_conn,params->value_handle,subscribe_params[unsub_conn].value_handle);
		//subscribe_params[unsub_conn].value_handle=0U;
		//params->value_handle = 0U;
		 is_connecting = false;
		 //is_connecting = true;
	
		return BT_GATT_ITER_CONTINUE;
	}

	
	printk("index %d recv %u %u %u %u %u %u %u.\n", bt_conn_index(conn),((uint8_t *)data)[0],((uint8_t *)data)[1],((uint8_t *)data)[2],((uint8_t *)data)[3],((uint8_t *)data)[4],((uint8_t *)data)[5],((uint8_t *)data)[6]);
	//printk("is_scanning %d is_connecting %d connecting_index %d \n ",is_scanning,is_connecting,connecting_index);

	
	gatt_write_buf[0] = ((uint8_t *)data)[0];
	
	gatt_write_buf[1] = count;
	gatt_write_buf[2] =88;
	gatt_write_buf[3] =99;	
	gatt_write_buf[4] =45;
	gatt_write_buf[5] =99;
	gatt_write_buf[6] =22;
	gatt_write_buf[7] =11;
	
	count++;

	write_params[bt_conn_index(conn)].data = gatt_write_buf;
	write_params[bt_conn_index(conn)].length = 8;
	write_params[bt_conn_index(conn)].handle = service_handle;
	write_params[bt_conn_index(conn)].offset = 0;
	write_params[bt_conn_index(conn)].func = write_func;
	
	stop_time1 = k_uptime_get_32();	
	k_timer_start(&my_timer, K_SECONDS(1), K_SECONDS(1));
	err = bt_gatt_write(conn, &write_params[bt_conn_index(conn)]);
	
	if (err) {
		printk("Write failed (err %d)\r\n", err);
	} else {
		//printk("Write pending\r\n");
		stop_time = k_uptime_get_32();
		cycles_spent = stop_time - start_time;
		ack_time = (stop_time - stop_time1);
		k_timer_stop(&my_timer);
		//delay[((uint8_t *)data)[0]][array_count] = cycles_spent;
		//nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(cycles_spent);
		//printk("recv --> send %u ms ack %u\n",cycles_spent,ack_time);
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;
	char str[37];
	char dic_str[37];
	uint8_t conn_index;

	bt_uuid_to_str(attr->uuid, str, sizeof(str));
	//printk("UUID: %s  Handle: %u  , %u\r\n", str, attr->handle);

	bt_uuid_to_str(discover_params[conn_index].uuid,dic_str,sizeof(dic_str));
	//printk("uuid dic %s\n",dic_str);

	conn_index = bt_conn_index(conn);

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}


	if (!bt_uuid_cmp(discover_params[conn_index].uuid, BT_UUID_HRS)) {
		memcpy(&uuid, BT_UUID_HRS_MEASUREMENT, sizeof(uuid));
		discover_params[conn_index].uuid = &uuid.uuid;
		discover_params[conn_index].start_handle = attr->handle + 1;
		discover_params[conn_index].type = BT_GATT_DISCOVER_CHARACTERISTIC;
		err = bt_gatt_discover(conn, &discover_params[conn_index]);

		if (err) {
			printk("Discover failed  1 (err %d)\n", err);
			is_connecting = false;
			    connecting_index = -1;
			    start_scan();
			
		}
	} else if (!bt_uuid_cmp(discover_params[conn_index].uuid,BT_UUID_HRS_MEASUREMENT)) {
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params[conn_index].uuid = &uuid.uuid;
		discover_params[conn_index].start_handle = attr->handle + 2;
		discover_params[conn_index].type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params[conn_index].value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params[conn_index]);

		if (err) {
			printk("Discover failed 2 (err %d)\n", err);
			is_connecting = false;
			    connecting_index = -1;
			    start_scan();
			
		}

	}

	else{
		subscribe_params[conn_index].notify = notify_func;
		subscribe_params[conn_index].value = BT_GATT_CCC_NOTIFY;
		subscribe_params[conn_index].ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params[conn_index]);
		if (err && err != -EALREADY) {
		    printk("Subscribe failed (err %d)\n", err);
		} else {
		    //printk("[SUBSCRIBED] Index: %u\n",bt_conn_index(conn));
		    service_handle = 48;
		}

		is_connecting = false;
		connecting_index = -1;
		
		if(conn_index==(CONFIG_BT_MAX_CONN-1)){
			//start_scan();
			printk("full conn %u\n",conn_index);
			full_flag = 1;
		}else{
			//printk("not full conn start scan\n");
			start_scan();
		}

		//start_scan();
		return BT_GATT_ITER_STOP;
	} 
	

	return BT_GATT_ITER_STOP;

	//return BT_GATT_ITER_CONTINUE;

}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int i,a;
	struct bt_conn_info testinfo ;
	printk("[AD]: %u data_len %u\n", data->type, data->data_len);
	int err;


	err = bt_le_scan_stop();
	if (err) {
		printk("Stop LE scan failed (err %d)\n", err);           
                is_scanning = false;
                start_scan();
                return true;
	}else {
                printk("Stopped scanning!\n");
                is_connecting = true;
                is_scanning = false;
            }

	
	static struct bt_conn *tempconn;
	tempconn = bt_conn_create_le(addr,conn_params);

	uint8_t conn_index = bt_conn_index(tempconn);
	connecting_index = conn_index;
	if (conn_index < CONFIG_BT_MAX_CONN) {
		printk("Attempting to connect to, index %d\n", conn_index);
		test_conn[conn_index] = tempconn;
		
		tempconn = NULL;
		return false;
	} 

	else {
		printk("eir failed now %d max %d\n",conn_index,CONFIG_BT_MAX_CONN);
		is_connecting = false;
		connecting_index = -1;
		start_scan();
		return true;
	}

	

	//printk("eir test \r\n");
	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];
	
	bt_addr_le_to_str(addr, dev, sizeof(dev));
	//printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	//       dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
		



	if (((strcmp(dev, "ec:e4:cd:dc:1b:f0 (random)") == 0) || (strcmp(dev, "d8:68:7c:ea:a9:1a (random)") == 0) || (strcmp(dev, "c9:bc:14:2a:64:65 (random)") == 0)  || (strcmp(dev, "f4:5d:6d:31:ab:93 (random)") == 0) || (strcmp(dev, "dc:31:3f:c9:6d:f3 (random)") == 0)|| (strcmp(dev, "f0:84:71:86:e1:11 (random)") == 0)|| (strcmp(dev, "ce:6a:cf:8d:98:78 (random)") == 0)|| (strcmp(dev, "f2:73:92:38:73:33 (random)") == 0)|| (strcmp(dev, "c3:6b:5f:4b:a9:74 (random)") == 0) 
|| (strcmp(dev, "f0:84:71:86:e1:11 (random)") == 0)
|| (strcmp(dev, "d0:14:a9:a0:a5:d7 (random)") == 0)
|| (strcmp(dev, "f3:73:dc:0c:d9:a2 (random)") == 0)
|| (strcmp(dev, "d0:23:eb:8d:bd:4c (random)") == 0)
|| (strcmp(dev, "df:f9:d8:8f:b1:4a (random)") == 0)
|| (strcmp(dev, "df:12:f0:e2:ea:ac (random)") == 0)
|| (strcmp(dev, "c7:da:59:ec:7e:01 (random)") == 0)
|| (strcmp(dev, "fd:cf:d3:76:28:77 (random)") == 0)
|| (strcmp(dev, "ec:68:c2:c2:b5:56 (random)") == 0)
|| (strcmp(dev, "e7:18:e9:36:de:99 (random)") == 0)
|| (strcmp(dev, "ef:dc:05:bd:37:c2 (random)") == 0)
|| (strcmp(dev, "d7:17:d1:d5:71:f3 (random)") == 0)
|| (strcmp(dev, "da:5b:6b:ab:a5:86 (random)") == 0)
|| (strcmp(dev, "c5:ee:f7:a5:ea:b4 (random)") == 0)
|| (strcmp(dev, "c9:bc:14:2a:64:65 (random)") == 0)
) && (type == BT_GAP_ADV_TYPE_ADV_IND ||
	//if ((type == BT_GAP_ADV_TYPE_ADV_IND ||
	    type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND)) {
		printk("Starting data parse\n");
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static void start_scan(void)
{
	int err;
	 //if (!is_scanning && !is_connecting ) {
	 if (!is_scanning && !is_connecting ) {
		//printk("Starting to scan!\n");

		/* Use active scanning and disable duplicate filtering to handle any
		 * devices that might update their advertising data at runtime. */
		struct bt_le_scan_param scan_param = {
			.type       = BT_LE_SCAN_TYPE_ACTIVE,
			.options    = BT_LE_SCAN_OPT_NONE,
			.interval   = BT_GAP_SCAN_FAST_INTERVAL,
			.window     = BT_GAP_SCAN_FAST_WINDOW,
		};

		//err = bt_le_scan_start(&scan_param, device_found);
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return;
		}else {
		    is_scanning = true;
		    printk("Scanning successfully started\n");
		}
	} else if (is_scanning){
		printk("Already scanning!\n");
	} else {
		printk("Connecting, cannot scan now!\n");
	}


		//printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;
	
	struct bt_conn_info testinfo ;
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));


	if (!conn_err) {
		uint8_t conn_index = bt_conn_index(conn);

		if(conn_index<CONFIG_BT_MAX_CONN){
			if(conn==test_conn[conn_index]){
				printk("Connected: %s, index: %u, ref: %u\n", addr, conn_index, conn->ref);

				memcpy(&uuid, BT_UUID_HRS, sizeof(uuid));
				//memcpy(&uuid, BT_EVAL_UUID_TEST, sizeof(uuid));
				discover_params[conn_index].uuid = &uuid.uuid;
				discover_params[conn_index].func = discover_func;
				discover_params[conn_index].start_handle = 0x0001;
				//discover_params.start_handle = 0x002d;
				discover_params[conn_index].end_handle = 0xffff;
				discover_params[conn_index].type = BT_GATT_DISCOVER_PRIMARY;
				//discover_params.type = BT_GATT_DISCOVER_ATTRIBUTE;
				//discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

				err = bt_gatt_discover(test_conn[conn_index], &discover_params[conn_index]);
				if (err) {
				    is_connecting = false;
				    connecting_index = -1;
				    printk("Discover failed(err %d)\n", err);
				    start_scan();
				}
				
			}
			//printk("bt_gatt_discover end\n");
			//start_scan();
		}
	}else {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		is_connecting = false;
		connecting_index = -1;
		start_scan();
	}


}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	//printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	uint8_t conn_index = bt_conn_index(conn);
	printk("conn_index == connecting_index %d %d \n",conn_index ,connecting_index);
	if (conn_index == connecting_index) {
		connecting_index = -1;
		is_connecting = false;
	}
	printk("Disconnected: %s, index: %u, ref: %u, (reason 0x%02x)\n", addr, conn_index, conn->ref, reason);
	printk("is_scanning %d is_connecting %d connecting_index %d \n ",is_scanning,is_connecting,connecting_index);
	if (conn_index < CONFIG_BT_MAX_CONN) {
		if (test_conn[conn_index] == conn) {
		    bt_conn_unref(test_conn[conn_index]);
		    test_conn[conn_index] = NULL;

		}
	}	
	start_scan();

}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void main(void)
{
	int err,i=0;
	err = bt_enable(NULL);
	int all;
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	/*ec:e4:cd:dc:1b:f0*/
/*wdt*/

	printk("Watchdog sample application\n");
	wdt = device_get_binding(WDT_DEV_NAME);
	wdt_config.flags = WDT_FLAG_RESET_SOC;
	wdt_config.window.min = 0U;
	//wdt_config.window.max = 1000U;
	wdt_config.window.max = 0U;
	wdt_config.callback = wdt_callback;

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);

/*wdt end*/

	bt_conn_cb_register(&conn_callbacks);

	start_scan();
	
	
	while(true) {

		
	}
	printk("end\r\n");


}
