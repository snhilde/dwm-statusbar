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

#define COLOR_NORMAL				color1
#define COLOR_ACTIVE				color2
#define COLOR1						color3
#define COLOR2						color4
#define COLOR_WARNING				color5
#define COLOR_ERROR					color6
#define GREEN_TEXT					color7
#define RED_TEXT					color8
#define COLOR_HEADING				COLOR_ACTIVE

#define TODO_MAX_LEN				100
#define WIFI_INTERFACE				"wlp4s0"
#define DISPLAY_KBD					true

#define TODO_FILE					"/home/user/.TODO"
#define STATUSBAR_LOG_FILE			"/home/user/.logs/dwm-statusbar.log"
#define DWM_LOG_FILE				"/home/user/.logs/dwm.log"
#define BACKUP_STATUS_FILE			"/home/user/.backup/.sb"
#define LOCATION					"0000000"
#define KEY							"00000000000000000000000000000000"
#define RH_LOGIN					"username={username}&password={password}

#define DWM_CONFIG_FILE				"/home/user/.dwm/config.h"
#define NET_RX_FILE					NET_CAT(WIFI_INTERFACE, rx)
#define NET_TX_FILE					NET_CAT(WIFI_INTERFACE, tx)
#define CPU_USAGE_FILE				"/proc/stat"
#define CPU_TEMP_DIR				"/sys/class/hwmon/hwmon0/"
#define FAN_SPEED_FILE				"/sys/class/hwmon/hwmon2/device/fan1_input"
#define SCREEN_BRIGHTNESS_FILE		"/sys/class/backlight/nvidia_backlight/brightness"
#define KBD_BRIGHTNESS_FILE			"/sys/class/leds/smc::kbd_backlight/brightness"
#define BATT_STATUS_FILE			"/sys/class/power_supply/BAT0/status"
#define BATT_CAPACITY_FILE			"/sys/class/power_supply/BAT0/capacity"

#define ERR(str, val) \
	{ snprintf(str, sizeof(str) - 1, "%c %s%c ", COLOR_ERROR, val, COLOR_NORMAL); \
	str[sizeof(str) - 1] = '\0'; \
	fprintf(stderr, "%s\t%s\n\n", asctime(tm_struct), val); \
	return -1; }

#define INIT_ERR(val) \
	{ fprintf(stderr, "%s\t%s\n", asctime(tm_struct), val); \
	perror("\tError"); \
	printf("\n"); \
	return -1; }

#define SND_ERR(val) \
	{ snd_mixer_close(handle); \
	handle = NULL; \
	snd_config_update_free_global(); \
	snprintf(volume_string, sizeof(volume_string) - 1, "%c %s%c ", COLOR_ERROR, val, COLOR_NORMAL); \
	volume_string[sizeof(volume_string) - 1] = '\0'; \
	fprintf(stderr, "%s%s\n\n", asctime(tm_struct), val); \
	return -1; }

#define SND_INIT_ERR(val) \
	{ snd_mixer_close(handle); \
	handle = NULL; \
	snd_config_update_free_global(); \
	fprintf(stderr, "%s%s\n", asctime(tm_struct), val); \
	perror("Error"); \
	printf("\n"); \
	return -1; }

#define CAT_5(A, B, C, D, E) #A B #C #D #E
#define NET_CAT(X, Y) CAT_5(/sys/class/net/, X, /statistics/, Y, _bytes)

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


struct cpu_temp_list {
	char *filename;
	struct cpu_temp_list *next;
} *temp_list = NULL;

const char color1 = '';
const char color2 = '';
const char color3 = '';
const char color4 = '';
const char color5 = '';
const char color6 = '';
const char color7 = '';
const char color8 = '';

long TODO_mtime = 0;
char weather_url[128];
char forecast_url[128];
int day_safe;				// due to cJSON's not being thread-safe
int temp_today;
bool weather_update = true;
long backup_mtime = 0;
bool equity_found = false;
bool portfolio_init = false;
char portfolio_url[128];
char token_header[64];
char account_number[32];
float equity_previous_close = 0.0;
struct curl_slist *headers = NULL;
bool wifi_connected = false;
int devidx;
struct tm *tm_struct = NULL;
int block_size;
int cpu_ratio;
int temp_max;
int fan_min;
int fan_max;
int screen_brightness_max;
int kbd_brightness_max;
float vol_range;

char statusbar_string[1024];
char top_bar[512];
char bottom_bar[512];
int bar_max_len;

char TODO_string[128];
char log_status_string[32];
char weather_string[96];
char backup_status_string[32];
char portfolio_value_string[32];
char wifi_status_string[32];
char time_string[32];

char network_usage_string[64];
char disk_usage_string[64];
char memory_string[32];
char cpu_load_string[32];
char cpu_usage_string[32];
char cpu_temp_string[32];
char fan_speed_string[32];

char brightness_string[32];
char volume_string[32];
char battery_string[32];
