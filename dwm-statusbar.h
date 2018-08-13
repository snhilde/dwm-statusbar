#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>

// X11
#include <X11/Xlib.h>

// weather
#include <curl/curl.h>
#include <cJSON/cJSON.h>

// wifi
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>
#include <libnetlink.h>
#include <linux/if_arp.h>
#include <ctype.h>
#include <stdbool.h>

// disk usage
#include <sys/statvfs.h>

// memory
#include <proc/sysinfo.h>

// volume
#include <alsa/asoundlib.h>
#include <math.h>

#define STRING_LENGTH			128
#define BAR_LENGTH				512
#define TOTAL_LENGTH			1024
#define UTC_TO_CST_SECONDS		18000
#define HOUR					0
#define DAY						1

#define COLOR_NORMAL				color1
#define COLOR_ACTIVE				color2
#define COLOR1						color3
#define COLOR2						color4
#define COLOR_WARNING				color5
#define COLOR_ERROR					color6
#define GREEN_TEXT					color7
#define RED_TEXT					color8
#define COLOR_HEADING				COLOR_ACTIVE

#define WIFI_INTERFACE				"wlp4s0"	// TODO get from /proc/net/arp?
#define DISPLAY_KBD					true

#define TODO_FILE					"/home/user/.TODO"
#define STATUSBAR_LOG_FILE			"/home/user/.logs/dwm-statusbar.log"
#define DWM_LOG_FILE				"/home/user/.logs/dwm.log"
#define BACKUP_STATUS_FILE			"/home/user/.backup/.sb"
#define LOCATION					"0000000"
#define KEY							"00000000000000000000000000000000"
#define WEATHER_URL					"http://api.openweathermap.org/data/2.5/weather?id=" \
									LOCATION "&appid=" KEY "&units=imperial"
#define FORECAST_URL				"http://api.openweathermap.org/data/2.5/forecast?id=" \
									LOCATION "&appid=" KEY "&units=imperial"
#define RH_LOGIN					"username={username}&password={password}
#define DWM_CONFIG_FILE				"/home/user/.dwm/config.h"
#define NET_RX_FILE					"/sys/class/net/" WIFI_INTERFACE "/statistics/rx_bytes"
#define NET_TX_FILE					"/sys/class/net/" WIFI_INTERFACE "/statistics/tx_bytes"
#define CPU_USAGE_FILE				"/proc/stat"
#define CPU_TEMP_DIR				"/sys/class/hwmon/hwmon0/"
#define FAN_SPEED_DIR				"/sys/class/hwmon/hwmon2/device/"
#define SCREEN_BRIGHTNESS_FILE		"/sys/class/backlight/nvidia_backlight/brightness"
#define KBD_BRIGHTNESS_FILE			"/sys/class/leds/smc::kbd_backlight/brightness"
#define BATT_STATUS_FILE			"/sys/class/power_supply/BAT0/status"
#define BATT_CAPACITY_FILE			"/sys/class/power_supply/BAT0/capacity"

#define CONST_ERR(val) \
	{ fprintf(stderr, "%s\t%s\n", asctime(tm_struct), val); \
	perror("\tError"); }

#define INIT_ERR(val, ret) \
	{ fprintf(stderr, "%s\t%s\n", asctime(tm_struct), val); \
	perror("\tError"); \
	return ret; }

#define ERR(str, val) \
	{ snprintf(str, sizeof(str) - 1, "%c %s%c ", COLOR_ERROR, val, COLOR_NORMAL); \
	str[sizeof(str) - 1] = '\0'; \
	INIT_ERR(val, -1) }

struct json_struct {
	char *data;
	int size;
};

struct disk_usage_struct {
	struct statvfs fs_stat;
	float bytes_used;
	float bytes_total;
	char unit_used;
	char unit_total;
} root_fs;

struct file_link {
	char *filename;
	struct file_link *next;
} *therm_list = NULL, *fan_list = NULL;

const char color1 = '';
const char color2 = '';
const char color3 = '';
const char color4 = '';
const char color5 = '';
const char color6 = '';
const char color7 = '';
const char color8 = '';

// singletons
CURL *curl;
struct nl_sock *sb_socket;
int sb_id;
struct nl_msg *sb_msg;
struct nl_cb *sb_cb;
struct rtnl_handle sb_rth;
snd_mixer_elem_t *snd_elem;

long TODO_mtime = 0;
char weather_url[STRING_LENGTH];
char forecast_url[STRING_LENGTH];
int day_safe;				// due to cJSON's not being thread-safe
int temp_today;
bool need_to_get_weather = true;
long backup_mtime = 0;
bool equity_found = false;
bool portfolio_consts_found = false;
char portfolio_url[STRING_LENGTH];
char token_header[STRING_LENGTH];
char account_number[STRING_LENGTH];
bool need_equity_previous_close = true;
int day_equity_previous_close;
float equity_previous_close = 0.0;
struct curl_slist *headers = NULL;
bool wifi_connected = false;
struct tm *tm_struct = NULL;

int const_devidx;
int const_block_size;
int const_bar_max_len;
int const_cpu_ratio;
int const_temp_max;
int const_fan_min;
int const_fan_max;
int const_screen_brightness_max;
int const_kbd_brightness_max;
float const_vol_range;

char statusbar_string[TOTAL_LENGTH];
char top_bar[BAR_LENGTH];
char bottom_bar[BAR_LENGTH];

char TODO_string[STRING_LENGTH];
char log_string[STRING_LENGTH];
char weather_string[STRING_LENGTH];
char backup_string[STRING_LENGTH];
char portfolio_string[STRING_LENGTH];
char wifi_string[STRING_LENGTH];
char time_string[STRING_LENGTH];

char network_string[STRING_LENGTH];
char disk_string[STRING_LENGTH];
char RAM_string[STRING_LENGTH];
char load_string[STRING_LENGTH];
char CPU_usage_string[STRING_LENGTH];
char CPU_temp_string[STRING_LENGTH];
char fan_string[STRING_LENGTH];

char brightness_string[STRING_LENGTH];
char volume_string[STRING_LENGTH];
char battery_string[STRING_LENGTH];
