#include "dwm-statusbar.h"

static void
center_bottom_bar(char *bottom_bar)
{
	if (strlen(bottom_bar) < bar_max_len) {
		int half = (bar_max_len - strlen(bottom_bar)) / 2;
		memmove(bottom_bar + half, bottom_bar, strlen(bottom_bar));
		for (int i = 0; i < half; i++)
			bottom_bar[i] = ' ';
	}
}

static int
format_string(Display *dpy, Window root)
{
	memset(statusbar_string, '\0', 1024);
	memset(top_bar, '\0', 512);
	memset(bottom_bar, '\0', 512);
			
	strcat(top_bar, TODO_string);
	strcat(top_bar, log_status_string);
	strcat(top_bar, weather_string);
	strcat(top_bar, backup_status_string);
	strcat(top_bar, portfolio_value_string);
	strcat(top_bar, wifi_status_string);
	strcat(top_bar, time_string);

	strcat(bottom_bar, network_usage_string);
	strcat(bottom_bar, disk_usage_string);
	strcat(bottom_bar, memory_string);
	strcat(bottom_bar, cpu_load_string);
	strcat(bottom_bar, cpu_usage_string);
	strcat(bottom_bar, cpu_temp_string);
	strcat(bottom_bar, fan_speed_string);
	
	strcat(bottom_bar, brightness_string);
	strcat(bottom_bar, volume_string);
	strcat(bottom_bar, battery_string);
	
	center_bottom_bar(bottom_bar);
	sprintf(statusbar_string, "%s;%s", top_bar, bottom_bar);
	
	if (!XStoreName(dpy, root, statusbar_string))
		return -1;
	if (!XFlush(dpy))
		return -1;
	
	return 0;
}

static int
get_TODO(void)
{
	// dumb function
	struct stat file_stat;
	if (stat(TODO_FILE, &file_stat) < 0)
		ERR(TODO_string, "Error Getting TODO File Stats")
	if (file_stat.st_mtime <= TODO_mtime)
		return 0;
	
	TODO_mtime = file_stat.st_mtime;
	
	FILE *fd;
	char line[128];
	
	if (!memset(TODO_string, '\0', 128))
		ERR(TODO_string, "Error resetting TODO_string")
	
	fd = fopen(TODO_FILE, "r");
	if (!fd)
		ERR(TODO_string, "Error Opening TODO File")
			
	// line 1
	if (fgets(line, 128, fd) == NULL)
		ERR(TODO_string, "All tasks completed!")
	line[strlen(line) - 1] = '\0'; // remove weird characters at end
	snprintf(TODO_string, 128, "%cTODO:%c%s",
			COLOR_HEADING, COLOR_NORMAL, line);
	
	// lines 2 and 3
	for (int i = 0; i < 2; i++) {
		memset(line, '\0', 128);
		if (fgets(line, 128, fd) == NULL) break;
		line[strlen(line) - 1] = '\0'; // remove weird characters at end
		switch (line[i]) {
			case '\0': break;
			case '\n': break;
			case '\t':
				memmove(line, line + i + 1, strlen(line));
				strncat(TODO_string, " -> ", sizeof TODO_string - strlen(TODO_string));
				strncat(TODO_string, line, sizeof TODO_string - strlen(TODO_string));
				break;
			default:
				if (i == 1) break;
				strncat(TODO_string, " | ", sizeof TODO_string - strlen(TODO_string));
				strncat(TODO_string, line, sizeof TODO_string - strlen(TODO_string));
				break;
		}
	}
	
	memset(TODO_string + TODO_MAX_LEN - 3, '.', 3);
	TODO_string[TODO_MAX_LEN] = '\0';
	
	if (fclose(fd))
		ERR(TODO_string, "Error Closing File")
			
	return 0;
}

static int
get_log_status(void)
{
	struct stat sb_stat;
	struct stat dwm_stat;

	if (stat(STATUSBAR_LOG_FILE, &sb_stat) < 0)
		ERR(log_status_string, "dwm-statusbar.log error")
	if (stat(DWM_LOG_FILE, &dwm_stat) < 0)
		ERR(log_status_string, "dwm.log error")
			
	if ((intmax_t)sb_stat.st_size > 1)
		sprintf(log_status_string, "%c Check SB Log%c ",
				COLOR_ERROR, COLOR_NORMAL);
	else if ((intmax_t)dwm_stat.st_size > 1)
		sprintf(log_status_string, "%c Check DWM Log%c ",
				COLOR_ERROR, COLOR_NORMAL);
	else
		if (!memset(log_status_string, '\0', 32))
			ERR(log_status_string, "error resetting log_status_string")
	
	return 0;
}

static int
get_index(cJSON *time_obj)
{
	const time_t time = time_obj->valueint;
	struct tm *ft_struct = localtime(&time);
	const int ft_day_of_week = ft_struct->tm_wday;
	int day = ft_day_of_week - day_safe;
	if (day < 0)
		day += 7;
	
	return day;
}

static int
parse_forecast_json(char *raw_json)
{
	// for 5-day forecast (sent as 3-hour intervals for 5 days)
	// only able to handle rain currently
	
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *list_array, *list_child;
	cJSON *main_dict, *temp_obj;
	cJSON *rain_dict, *rain_obj;
	int i;
	char forecast_string[32], tmp_str[16];
	if (!memset(forecast_string, '\0', 32))
		return -1;
	
	struct data {
		int high;
		int low;
		float precipitation;
	} data[5];
	
	const char days_of_week[10][4] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Mon", "Tue", "Wed" };
	
	for (i = 0; i < 5; i++) {
		if (!i) {
			data[i].high = temp_today;
			data[i].low = temp_today;
		} else {
			// god help us all if these values don't work
			data[i].high = -1000;
			data[i].low = 1000;
		}
			data[i].precipitation = 0.0;
	}
	
	list_array = cJSON_GetObjectItem(parsed_json, "list");
	if (!list_array)
		ERR(weather_string, "Error finding 'list' in forecast")
	cJSON_ArrayForEach(list_child, list_array) {
		int f_day = get_index(cJSON_GetObjectItem(list_child, "dt"));
		if (f_day > 3)
			break;
		
		main_dict = cJSON_GetObjectItem(list_child, "main");
		if (!main_dict)
			ERR(weather_string, "Error finding 'main_dict' in forecast")
		temp_obj = cJSON_GetObjectItem(main_dict, "temp");
		if (!temp_obj)
			ERR(weather_string, "Error finding 'temp_obj' in forecast")
		data[f_day].high = (int)fmax(data[f_day].high, temp_obj->valueint);
		data[f_day].low = (int)fmin(data[f_day].low, temp_obj->valueint);
		
		if (rain_dict = cJSON_GetObjectItem(list_child, "rain"))
			if (rain_obj = rain_dict->child)
				data[f_day].precipitation += rain_obj->valuedouble;
	}
	
	for (i = 0; i < 4; i++) {
		snprintf(tmp_str, 16, "%c %s(%2d/%2d)",
				data[i].precipitation >= 3 ? COLOR_WARNING : COLOR_NORMAL,
				i > 0 ? days_of_week[day_safe + i - 1] : "",
				data[i].high, data[i].low);
		strncat(weather_string, tmp_str, sizeof weather_string - strlen(weather_string) - 1);
	}
	
	cJSON_Delete(parsed_json);
	return 0;
}

static int
parse_weather_json(char *raw_json)
{
	// for current weather
	// only able to handle rain currently (winter is not coming)
	// if (id >= 200 && id < 300)
		// Stormy
	// else if (id >= 300 && id < 400)
		// Drizzly
	// else if (id >= 500 && id < 600)
		// Rainy
	// else if (id >= 700 && id < 800)
		// Low visibility
	// else if (id == 800)
		// Clear
	// else if (id > 800 && id < 900)
		// Cloudy
	
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *main_dict, *temp_obj;
	cJSON *weather_dict;
	int id;
	
	main_dict = cJSON_GetObjectItem(parsed_json, "main");
	if (!main_dict)
		ERR(weather_string, "Error finding 'main' in weather")
	temp_obj = cJSON_GetObjectItem(main_dict, "temp");
	if (!temp_obj)
		ERR(weather_string, "Error finding 'temp' in weather")
	temp_today = temp_obj->valueint;
	
	weather_dict = cJSON_GetObjectItem(parsed_json, "weather");
	if (!weather_dict)
		ERR(weather_string, "Error finding 'weather' in weather")
	weather_dict = weather_dict->child;
	if (!weather_dict)
		ERR(weather_string, "Error finding 'weather' in weather")
	id = cJSON_GetObjectItem(weather_dict, "id")->valueint;
	if (!id)
		ERR(weather_string, "Error getting id from weather")
	
	snprintf(weather_string, 96, " %c weather:%c%2d F",
			id < 800 ? COLOR_WARNING : COLOR_HEADING,
			id < 800 ? COLOR_WARNING : COLOR_NORMAL,
			temp_today);
	
	cJSON_Delete(parsed_json);
	return 0;
}
	
static size_t
curl_callback(char *weather_json, size_t size, size_t nmemb, void *userdata)
{
	const size_t received_size = size * nmemb;
	struct json_struct *tmp;
	
	tmp = (struct json_struct *)userdata;
	
	tmp->data = realloc(tmp->data, tmp->size + received_size + 1);
	if (!tmp->data)
		return -1;
	
	memcpy(&(tmp->data[tmp->size]), weather_json, received_size);
	tmp->size += received_size;
	tmp->data[tmp->size] = '\0';
	
	return received_size;
}

static int
get_weather(void)
{
	if (!memset(weather_string, '\0', 96))
		ERR(weather_string, "Error resetting weather_string")
			
	if (wifi_connected == false) {
		sprintf(weather_string, "%c weather:%cN/A ", COLOR_HEADING, COLOR_NORMAL);
		return -2;
	}
	
	CURL *curl;
	int i;
	struct json_struct json_structs[2];
	static const char *urls[2] = { weather_url, forecast_url };
	char cap[3];
	
	day_safe = tm_struct->tm_wday;
	
	if (curl_global_init(CURL_GLOBAL_ALL))
		ERR(weather_string, "Error curl_global_init(). Please fix issue and restart.")
	if (!(curl = curl_easy_init()))
		ERR(weather_string, "Error curl_easy_init(). Please fix issue and restart.")
			
	for (i = 0; i < 2; i++) {
		json_structs[i].data = (char *)malloc(1);
		if (json_structs[i].data == NULL)
			ERR(weather_string, "Out of memory");
		json_structs[i].size = 0;
		
		if (curl_easy_setopt(curl, CURLOPT_URL, urls[i]) != CURLE_OK ||
				curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_structs[i]) != CURLE_OK)
			ERR(weather_string, "Error curl_easy_setops() in get_weather(). Please fix issue and restart.")
		if (curl_easy_perform(curl) == CURLE_OK) {
			if (!i) {
				if (parse_weather_json(json_structs[i].data) < 0)
					ERR(weather_string, "Error parsing weather json. Please fix issue and restart.")
			} else {
				if (parse_forecast_json(json_structs[i].data) < 0)
					ERR(weather_string, "Error parsing forecast json. Please fix issue and restart.")
			}
			sprintf(cap, "%c ", COLOR_NORMAL);
			strcat(weather_string, cap);
			weather_update = false;
		} else
			sprintf(weather_string, "%c weather:%cN/A ",
					COLOR_HEADING, COLOR_NORMAL);
	}
	
	free(json_structs[0].data);
	free(json_structs[1].data);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}

static int
parse_error_code(int code, char *ret_str)
{
	switch (code) {
		case 20:
			strcpy(ret_str, "already done"); break;
		case 21:
			strcpy(ret_str, "tar error"); break;
		case 22:
			strcpy(ret_str, "gpg error"); break;
		case 23:
			strcpy(ret_str, "no acc token"); break;
		case 24:
			strcpy(ret_str, "error get url"); break;
		case 25:
			strcpy(ret_str, "token timeout"); break;
		case 26:
			strcpy(ret_str, "err verifying"); break;
		default:
			strcpy(ret_str, "err in backup");
	}
	
	return 0;
}

static int
get_backup_status(void)
{
	struct stat file_stat;
	if (stat(BACKUP_STATUS_FILE, &file_stat) < 0)
		ERR(backup_status_string, "Error Getting Backup File Stats")
	if (file_stat.st_mtime <= backup_mtime)
		return 0;
	
	backup_mtime = file_stat.st_mtime;
	
	FILE *fd;
	char line[32], print[16], color = COLOR_ERROR;
	int value;
	time_t curr_time;
	time_t t_diff;
	
	if (!memset(backup_status_string, '\0', 32))
		ERR(backup_status_string, "Error resetting backup_status_string")
	
	if (!(fd = fopen(BACKUP_STATUS_FILE, "r")))
		ERR(backup_status_string, "Error Opening Backup Status File")
			
	if (fgets(line, 32, fd) == NULL)
		ERR(backup_status_string, "No Backup History")
			
	if (fclose(fd))
		ERR(backup_status_string, "Error Closing Backup Status File")
			
	if (isdigit(line[0])) {
		sscanf(line, "%d", &value);
				
		if (value >= 20 && value <= 26)
			parse_error_code(value, print);
		else {
			time(&curr_time);
			t_diff = curr_time - value;
			if (t_diff > 86400)
				strcpy(print, "missed");
			else {
				strcpy(print, "done");
				color = COLOR1;
			}
		}
	} else {
		line[strlen(line) - 1] = '\0';
		strcpy(print, line);
		color = COLOR2;
	}
	
	snprintf(backup_status_string, 32, "%cbackup:%c %s%c ",
			COLOR_HEADING, color, print, COLOR_NORMAL);
		
	return 0;
}

static double
parse_portfolio_json(char *raw_json)
{
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *equity_obj, *extended_hours_equity_obj;
	cJSON *equity_previous_close_obj;
	double equity_f;
	
	equity_obj = cJSON_GetObjectItem(parsed_json, "equity");
	if (!equity_obj)
		return -1;
	
	extended_hours_equity_obj = cJSON_GetObjectItem(parsed_json, "extended_hours_equity");
	if (!extended_hours_equity_obj)
		return -1;
	
	if (extended_hours_equity_obj->valuestring == NULL)
		equity_f = atof(equity_obj->valuestring);
	else
		equity_f = atof(extended_hours_equity_obj->valuestring);
	
	if (!equity_previous_close) {
		equity_previous_close_obj = cJSON_GetObjectItem(parsed_json, "equity_previous_close");
		if (!equity_previous_close_obj)
			return -1;
		equity_previous_close = atof(equity_previous_close_obj->valuestring);
	}
	
	cJSON_Delete(parsed_json);
	return equity_f;
}
	
static int
get_portfolio_value(void)
{
	// Robinhood starts trading at 9:00 am EST
	if (timezone / 3600 + tm_struct->tm_hour < 14 && equity_found == true)
		return 0;
	// Robinhood stops after-market trading at 6:00 pm EST
	if (timezone / 3600 + tm_struct->tm_hour > 23 && equity_found == true)
		return 0;
	
	if (wifi_connected == false) {
		sprintf(portfolio_value_string, "%crobinhood:%cN/A",
				COLOR_HEADING, COLOR_NORMAL);
		return -2;
	}
	
	if (portfolio_init == false)
		return -2;
	
	if (!memset(portfolio_value_string, '\0', 32))
		ERR(portfolio_value_string, "Error resetting portfolio_va...")
			
	int tz_gap;
	CURL *curl;
	struct json_struct portfolio_jstruct;
	static double equity;
	
	portfolio_jstruct.data = (char *)malloc(1);
	if (portfolio_jstruct.data == NULL)
		ERR(portfolio_value_string, "Out of memory");
	portfolio_jstruct.size = 0;
	
	if (curl_global_init(CURL_GLOBAL_ALL))
		ERR(portfolio_value_string, "Error curl_global_init()")
	if (!(curl = curl_easy_init()))
		ERR(portfolio_value_string, "Error curl_easy_init()")
			
	if (curl_easy_setopt(curl, CURLOPT_URL, portfolio_url) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &portfolio_jstruct) != CURLE_OK)
		ERR(portfolio_value_string, "Error curl_easy_setops()")
	if (curl_easy_perform(curl) == CURLE_OK) {
		if ((equity = parse_portfolio_json(portfolio_jstruct.data)) < 0)
			ERR(portfolio_value_string, "Error parsing portfolio json")
		
		sprintf(portfolio_value_string, "%crobinhood:%c%.2lf",
				COLOR_HEADING, equity >= equity_previous_close ? GREEN_TEXT : RED_TEXT, equity);
		equity_found = true;
	} else
		sprintf(portfolio_value_string, "%crobinhood:%cN/A",
				COLOR_HEADING, COLOR_NORMAL);
			
	free(portfolio_jstruct.data);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}

static int
free_wifi_list(struct nlmsg_list *list)
{
	struct nlmsg_list *next;
	
	while (list != NULL) {
		next = list->next;
		free(list);
		list = next;
	}
}

static int
format_wifi_status(char color)
{
	char tmp[32];
	
	if (strlen(wifi_status_string) > (sizeof wifi_status_string - 6))
		memset(wifi_status_string + strlen(wifi_status_string) - 3, '.', 3);
	
	sprintf(tmp, " %cwifi:%c %s %c ",
			COLOR_HEADING, color, wifi_status_string, COLOR_NORMAL);
	strcpy(wifi_status_string, tmp);
	
	return 0;
}

static int
print_ssid(uint8_t len, uint8_t *data)
{
	// stolen from iw
	int i;
	uint8_t tmp_str[2];

	memset(wifi_status_string, '\0', 32);
	for (i = 0; i < len && i < sizeof wifi_status_string; i++) {
		if (isprint(data[i]) && data[i] != ' ' && data[i] != '\\')
			sprintf(tmp_str, "%c", data[i]);
		else if (data[i] == ' ' && (i != 0 && i != len -1))
			sprintf(tmp_str, " ");
		else
			sprintf(tmp_str, "\\x%.2x", data[i]);
		strncat(wifi_status_string, tmp_str,
				sizeof wifi_status_string - strlen(wifi_status_string) - 1);
	}
	
	return 0;
}

static int
wifi_callback(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	uint8_t len;
	uint8_t *data;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (tb[NL80211_ATTR_SSID]) {
		len = nla_len(tb[NL80211_ATTR_SSID]);
		data = nla_data(tb[NL80211_ATTR_SSID]);
		print_ssid(len, data);
	}
	
	return NL_SKIP;
}

static int
store_nlmsg(const struct sockaddr_nl *who, struct nlmsghdr *hdr, void *arg)
{
	// stolen from iproute2
	struct nlmsg_list **linfo = (struct nlmsg_list**)arg;

	for (linfo; *linfo; linfo = &(*linfo)->next);
	*linfo = (struct nlmsg_list *)malloc(hdr->nlmsg_len + sizeof(void*));
	if (*linfo == NULL)
		return -1;

	memcpy(&(*linfo)->h, hdr, hdr->nlmsg_len);
	(*linfo)->next = NULL;

	return 0;
}

static int
ip_check(int flag)
{
	// stolen from iproute2
	struct rtnl_handle rth;
	struct nlmsg_list *linfo = NULL;
	struct nlmsg_list *head = NULL;
	
	struct ifinfomsg *ifi;
	struct rtattr *tb[IFLA_MAX+1];
	int len;
	int rv;
	
	if (rtnl_open(&rth, 0) < 0)
		ERR(wifi_status_string, "error: rtnl_open")
	if (rtnl_wilddump_request(&rth, AF_PACKET, RTM_GETLINK) < 0)
		ERR(wifi_status_string, "error: rtnl_wilddump_request")
	if (rtnl_dump_filter(&rth, store_nlmsg, &linfo) < 0)
		ERR(wifi_status_string, "error: rtnl_dump_filter")
	rtnl_close(&rth);
	
	head = linfo;
	for (int i = 1; i < devidx; i++, linfo = linfo->next);
	ifi = NLMSG_DATA(&linfo->h);
	if (!ifi)
		ERR(wifi_status_string, "error accessing ifi")
			
	len = linfo->h.nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));
	parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);
	
	if (flag)
		// 2 if down, 6 if up
		rv = *(__u8 *)RTA_DATA(tb[IFLA_OPERSTATE]);
	else
		// 0 if down, 1 if up
		rv = ifi->ifi_flags & IFF_UP;
	
	free_wifi_list(head);
	return rv;
}

static int
get_wifi_status(void)
{
	struct nl_sock *socket;
	int id;
	struct nl_msg *msg;
	struct nl_cb *cb;
	
	int ifi_flag;
	int op_state;
	char color = COLOR2;
	
	ifi_flag = ip_check(0);
	if (ifi_flag == -1) return -1;
	op_state = ip_check(1);
	if (op_state == -1) return -1;
	
	if (ifi_flag == 0 && op_state == 2) {
		strncpy(wifi_status_string, "Wireless Device Set Down", 31);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 0) {
		strncpy(wifi_status_string, "Wireless State Unknown", 31);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 2) {
		strncpy(wifi_status_string, "No Connection Initiated", 31);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 5) {
		strncpy(wifi_status_string, "No Carrier", 31);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 6) {
		if (wifi_connected == true) return 0;
		if (!memset(wifi_status_string, '\0', 32))
			ERR(wifi_status_string, "Error resetting wifi_status_string")
	
		socket = nl_socket_alloc();
		if (!socket)
			ERR(wifi_status_string, "err: nl_socket_alloc()")
		if (genl_connect(socket) < 0)
			ERR(wifi_status_string, "err: genl_connect()")
		id = genl_ctrl_resolve(socket, "nl80211");
		if (!id)
			ERR(wifi_status_string, "err: genl_ctrl_resolve()")
		
		msg = nlmsg_alloc();
		if (!msg)
			ERR(wifi_status_string, "err: nlmsg_alloc()")
		cb = nl_cb_alloc(NL_CB_DEFAULT);
		if (!cb)
			ERR(wifi_status_string, "err: nl_cb_alloc()")
		
		genlmsg_put(msg, 0, 0, id, 0, 0, NL80211_CMD_GET_INTERFACE, 0);
		if (nla_put(msg, NL80211_ATTR_IFINDEX, sizeof(uint32_t), &devidx) < 0)
			ERR(wifi_status_string, "err: nla_put()")
		nl_send_auto_complete(socket, msg);
		if (nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, wifi_callback, NULL) < 0)
			ERR(wifi_status_string, "err: nla_cb_set()")
		if (nl_recvmsgs(socket, cb) < 0)
			strncpy(wifi_status_string, "No Wireless Connection", 31);
		else
			color = COLOR1;

		wifi_connected = true;
		nlmsg_free(msg);
		nl_socket_free(socket);
		free(cb);
	} else
		ERR(wifi_status_string, "Error with WiFi Status")
	
	format_wifi_status(color);
	
	return 0;
}

static int
get_time(void)
{
	if (!memset(time_string, '\0', 32))
		ERR(time_string, "Error resetting time_string")
	
	if (strftime(time_string, 32,  "%b %d - %I:%M", tm_struct) == 0)
		ERR(time_string, "Error with strftime()")
	if (tm_struct->tm_sec % 2)
		time_string[strlen(time_string) - 3] = ' ';
	
	return 0;
}

static char
get_unit(int unit)
{
	switch (unit) {
		case 1: return 'K';
		case 2: return 'M';
		case 3: return 'G';
		default: return 'B';
	}
}

static int
format_bytes(long *bytes, int *step)
{
	long bytes_n = *bytes;
	int step_n = 0;
	
	while (bytes_n >= 1 << 10) {
		bytes_n = bytes_n >> 10;
		step_n++;
	}
	
	*bytes = bytes_n;
	*step = step_n;
	
	return 0;
}

static int
get_network_usage(void)
{
	if (!memset(network_usage_string, '\0', 64))
		ERR(network_usage_string, "Error resetting network_usage")
	
	/* from top.c */
	const char* files[2] = { NET_RX_FILE, NET_TX_FILE };
	FILE *fd;
	char line[64];
	static long rx_old, tx_old;
	long rx_new, tx_new;
	long rx_bps, tx_bps;
	int step = 0;
	char rx_unit, tx_unit;
	
	for (int i = 0; i < 2; i++) {
		fd = fopen(files[i], "r");
		if (!fd)
			ERR(network_usage_string, "Read Error")
		fgets(line, 64, fd);
		if (fclose(fd))
			ERR(network_usage_string, "Close Error")
		
		if (i) {
			sscanf(line, "%d", &tx_new);
			tx_bps = tx_new - tx_old;
			format_bytes(&tx_bps, &step);
			tx_unit = get_unit(step);
		} else {
			sscanf(line, "%d", &rx_new);
			rx_bps = rx_new - rx_old;
			format_bytes(&rx_bps, &step);
			rx_unit = get_unit(step);
		}
	}
	
	if (rx_bps > 999) rx_bps = 999;
	if (tx_bps > 999) tx_bps = 999;
	
	snprintf(network_usage_string, 64, "%cnetwork:%c%3d %c/S down,%c%3d %c/S up%c ",
			COLOR_HEADING, rx_unit == 'M' ? COLOR_WARNING : COLOR_NORMAL, rx_bps, rx_unit,
			tx_unit == 'M' ? COLOR_WARNING : COLOR_NORMAL, tx_bps, tx_unit, COLOR_NORMAL);
	
	rx_old = rx_new;
	tx_old = tx_new;
	
	return 0;
}

static int
process_stat(struct disk_usage_struct *dus)
{
	int unit_int = 0;
	float bytes_used;
	float bytes_total;
	
	bytes_used = (float)(dus->fs_stat.f_blocks - dus->fs_stat.f_bfree) * block_size;
	bytes_total = (float)dus->fs_stat.f_blocks *  block_size;
	
	while (bytes_used > 1 << 10) {
		bytes_used /= 1024;
		unit_int++;
	}
	dus->bytes_used = bytes_used;
	dus->unit_used = get_unit(unit_int);
	
	unit_int = 0;
	while (bytes_total > 1 << 10) {
		bytes_total /= 1024;
		unit_int++;
	}
	dus->bytes_total = bytes_total;
	dus->unit_total = get_unit(unit_int);
	
	return 0;
}

static int
get_disk_usage(void)
{
	if (!memset(disk_usage_string, '\0', 64))
		ERR(disk_usage_string, "Error resetting disk_usage_string")
	
	int rootperc;

	if (statvfs("/", &root_fs.fs_stat) < 0)
		ERR(disk_usage_string, "Error getting filesystem stats")
	
	process_stat(&root_fs);
	rootperc = rint((double)root_fs.bytes_used / (double)root_fs.bytes_total * 100);
	
	snprintf(disk_usage_string, 64, " %c disk:%c%.1f%c/%.1f%c%c ", 
			rootperc >= 75? COLOR_WARNING : COLOR_HEADING,
			rootperc >= 75? COLOR_WARNING : COLOR_NORMAL,
			root_fs.bytes_used, root_fs.unit_used, root_fs.bytes_total, root_fs.unit_total,
			COLOR_NORMAL);

	return 0;
}

static int
get_memory(void)
{
	if (!memset(memory_string, '\0', 32))
		ERR(memory_string, "Error resetting memory_string")
	
	int memperc;

	meminfo();
	
	memperc = rint((double)kb_active / (double)kb_main_total * 100);
	if (memperc > 99)
		memperc = 99;
	
	snprintf(memory_string, 32, " %c RAM:%c%2d%% used%c ",
			memperc >= 75? COLOR_WARNING : COLOR_HEADING,
			memperc >= 75? COLOR_WARNING : COLOR_NORMAL,
			memperc, COLOR_NORMAL);
	
	return 0;
}

static int
get_cpu_load(void)
{
	if (!memset(cpu_load_string, '\0', 32))
		ERR(cpu_load_string, "Error resetting cpu_load_string")
	
	// why was this static?
	double av[3];
	
	loadavg(&av[0], &av[1], &av[2]);
	snprintf(cpu_load_string, 32, " %c load:%c%.2f %.2f %.2f%c ",
			av[0] > 1 ? COLOR_WARNING : COLOR_HEADING,
			av[0] > 1 ? COLOR_WARNING : COLOR_NORMAL,
			av[0], av[1], av[2], COLOR_NORMAL);
	
	return 0;
}

static int
get_cpu_usage(void)
{
	if (!memset(cpu_usage_string, '\0', 32))
		ERR(cpu_usage_string, "Error resetting cpu_usage_string")
	
	/* from top.c */
	FILE *fd;
	char buf[64];
	int seca = 0, secb = 0, secc = 0, secd = 0, top, bottom, total = 0;
	int i;
	
	static struct {
		int oldval[7];
		int newval[7];
	} cpu;

	fd = fopen(CPU_USAGE_FILE, "r");
	if (!fd)
		ERR(cpu_usage_string, "Read Error")
	fgets(buf, 64, fd);
	if (fclose(fd))
		ERR(cpu_usage_string, "Close Error")
	
	sscanf(buf, "cpu %d %d %d %d", &cpu.newval[0], &cpu.newval[1], &cpu.newval[2], &cpu.newval[3],
			&cpu.newval[4], &cpu.newval[5], &cpu.newval[6]);

	// exclude first run
	if (cpu.oldval[0]) {
		for (i = 0; i < 7; i++) {
			secc += cpu.newval[i];
			secd += cpu.oldval[i];
			
			if (i == 3) continue;
			seca += cpu.newval[i];
			secb += cpu.oldval[i];
		}
		
		top = seca - secb + 1;
		bottom = secc - secd + 1;
		
		total = rint((double)top / (double)bottom * 100);
		total *= cpu_ratio;
	}
	
	for (i = 0; i < 7; i++)
		cpu.oldval[i] = cpu.newval[i];
	
	if (total >= 100) total = 99;
	snprintf(cpu_usage_string, 32, " %c CPU usage:%c%2d%%%c ",
			total >= 75 ? COLOR_WARNING : COLOR_HEADING,
			total >= 75 ? COLOR_WARNING : COLOR_NORMAL,
			total, COLOR_NORMAL);
	
	return 0;
}

static int
get_cpu_temp(void)
{
	if (!memset(cpu_temp_string, '\0', 32))
		ERR(cpu_temp_string, "Error resetting cpu_temp_string")
	
	struct cpu_temp_list *snake;
	int counter;
	int temp = 0;
	int tempperc;
	
	for (snake = temp_list, counter = 0; snake != NULL; snake = snake->next, counter++) {
		char path[128];
		FILE *fd;
		int tmp;
		
		strcpy(path, CPU_TEMP_DIR);
		strcat(path, snake->filename);
		
		fd = fopen(path, "r");
		if (!fd)
			ERR(cpu_temp_string, "Error Opening CPU File")
		if (!fscanf(fd, "%d", &tmp))
			ERR(cpu_temp_string, "Error Scanning CPU File")
		if (fclose(fd))
			ERR(cpu_temp_string, "Error Closing CPU File")
		temp += tmp;
	}
	
	temp /= counter;
	tempperc = rint((double)temp / (double)temp_max * 100);
	temp >>= 10;
	
	snprintf(cpu_temp_string, 32, " %c CPU temp:%c%2d degC%c ",
			tempperc >= 75? COLOR_WARNING : COLOR_HEADING,
			tempperc >= 75? COLOR_WARNING : COLOR_NORMAL,
			temp, COLOR_NORMAL);

	return 0;
}

static int
get_fan_speed(void)
{
	if (!memset(fan_speed_string, '\0', 32))
		ERR(fan_speed_string, "Error resetting fan_speed_string")
	
	FILE *fd;
	int rpm;
	int fanperc;
	
	fd = fopen(FAN_SPEED_FILE, "r");
	if (!fd)
		ERR(fan_speed_string, "Error Opening File")
	if (!fscanf(fd, "%d", &rpm))
		ERR(fan_speed_string, "Error Scanning File")
	if (fclose(fd))
		ERR(fan_speed_string, "Error Closing File")
	
	rpm -= fan_min;
	if (rpm <= 0)
		rpm = 0;
	
	fanperc = rint((double)rpm / (double)fan_max * 100);
	
	if (fanperc >= 100)
		snprintf(fan_speed_string, 32, " %c fan: MAX%c ", COLOR_WARNING, COLOR_NORMAL);
	else
		snprintf(fan_speed_string, 32, " %c fan:%c%2d%%%c ",
				fanperc >= 75? COLOR_WARNING : COLOR_HEADING,
				fanperc >= 75? COLOR_WARNING : COLOR_NORMAL,
				fanperc, COLOR_NORMAL);

	return 0;
}

static int
get_brightness(void)
{
	if (!memset(brightness_string, '\0', 32))
		ERR(brightness_string, "Error resetting brightness_string")
			
	const char* b_files[2] = { SCREEN_BRIGHTNESS_FILE, KBD_BRIGHTNESS_FILE };
	
	int scrn, kbd;
	int scrn_perc, kbd_perc;
	
	for (int i = 0; i < 2; i++) {
		if (i == 1 && DISPLAY_KBD == false) continue;
		FILE *fd;
		fd = fopen(b_files[i], "r");
		if (!fd)
			ERR(brightness_string, "Error w File Open")
		fscanf(fd, "%d", i == 0 ? &scrn : &kbd);
		if (fclose(fd))
			ERR(brightness_string, "Error File Close")
	}
	
	scrn_perc = rint((double)scrn / (double)screen_brightness_max * 100);
	if (DISPLAY_KBD == true)
		kbd_perc = rint((double)kbd / (double)kbd_brightness_max * 100);
	
	if (DISPLAY_KBD == true)
		snprintf(brightness_string, 32, " %c brightness:%c%3d%%, %3d%%%c ",
			COLOR_HEADING, COLOR_NORMAL, scrn_perc, kbd_perc, COLOR_NORMAL);
	else
		snprintf(brightness_string, 32, " %c brightness:%c%3d%%%c ",
			COLOR_HEADING, COLOR_NORMAL, scrn_perc, COLOR_NORMAL);
	
	return 0;
}

static int
get_volume(void)
{
	if (!memset(volume_string, '\0', 32))
		ERR(volume_string, "Error resetting volume_string")
	
	// stolen from amixer utility from alsa-utils
	long pvol;
	int swch, volperc;
	
	snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	
	if (snd_mixer_open(&handle, 0))
		SND_ERR("Error Open")
	if (snd_mixer_attach(handle, "default"))
		SND_ERR("Error Attch")
	if (snd_mixer_selem_register(handle, NULL, NULL))
		SND_ERR("Error Rgstr")
	if (snd_mixer_load(handle))
		SND_ERR("Error Load")
	
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_name(sid, "Master");
	
	if (!(elem = snd_mixer_find_selem(handle, sid)))
		SND_ERR("Error Elem")
	
	if (snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &swch))
		SND_ERR("Error Get S")
	if (!swch) {
		snprintf(volume_string, 32, " %c volume:%cmute%c ",
				COLOR_HEADING, COLOR_NORMAL, COLOR_NORMAL);
	} else {
		if (snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &pvol))
			SND_ERR("Error Get V")
		// round to the nearest ten
		volperc = (double)pvol / vol_range * 100;
		volperc = rint((float)volperc / 10) * 10;
		
		snprintf(volume_string, 32, " %c volume:%c%3d%%%c ",
				COLOR_HEADING, COLOR_NORMAL, volperc, COLOR_NORMAL);
	}
	
	if (snd_mixer_close(handle))
		SND_ERR("Error Close")
	handle = NULL;
	snd_config_update_free_global();
	
	return 0;
}

static int
get_battery(void)
{
	if (!memset(battery_string, '\0', 32))
		ERR(battery_string, "Error resetting battery_string")
	/* from acpi.c and other acpi source files */
	FILE *fd;
	char status_string[20];
	int status; // -1 = discharging, 0 = full, 1 = charging
	int capacity;
	
	const char *filepaths[2] = { BATT_STATUS_FILE, BATT_CAPACITY_FILE };
	
	fd = fopen(BATT_STATUS_FILE, "r");
	if (!fd)
		ERR(battery_string, "Err Open Bat Fil")
	fscanf(fd, "%s", &status_string);
	if (fclose(fd))
		ERR(battery_string, "Err Close Bat fd")

	if (!strcmp(status_string, "Full") || !strcmp(status_string, "Unknown")) {
		status = 0;
		snprintf(battery_string, 32, " %c battery:%c full %c",
				COLOR_HEADING, COLOR1, COLOR_NORMAL);
		return 0;
	}
	
	if (!strcmp(status_string, "Discharging"))
		status = -1;
	else if (!strcmp(status_string, "Charging"))
		status = 1;
	else
		ERR(battery_string, "Err Read Bat Sts")
		
	fd = fopen(BATT_CAPACITY_FILE, "r");
	if (!fd)
		ERR(battery_string, "Err Open Bat Fil")
	fscanf(fd, "%d", &capacity);
	if (fclose(fd))
		ERR(battery_string, "Err Close Bat fd")
	
	if (capacity > 99)
		capacity = 99;
		
	snprintf(battery_string, 32, " %c battery: %c%2d%% %c",
			capacity < 20 ? COLOR_ERROR : status > 0 ? COLOR2 : COLOR_WARNING,
			status > 0 ? '+' : '-', capacity, COLOR_NORMAL);
	
	return 0;
}

static int
parse_account_number_json(char *raw_json)
{
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *results, *account, *account_num;
	cJSON *weather_dict;
	int id;
	
	results = cJSON_GetObjectItem(parsed_json, "results");
	if (!results)
		return -1;
	account = results->child;
	if (!account)
		return -1;
	account_num = cJSON_GetObjectItem(account, "account_number");
	if (!account_num)
		return -1;
	
	strncpy(account_number, account_num->valuestring, 31);
	
	cJSON_Delete(parsed_json);
	return 0;
}
	
static int
get_account_number(void)
{
	CURL *curl;
	struct json_struct account_number_struct;
	
	account_number_struct.data = (char *)malloc(1);
	if (account_number_struct.data == NULL)
		INIT_ERR("error allocating account_number_struct.data")
	account_number_struct.size = 0;
	
	if (curl_global_init(CURL_GLOBAL_ALL))
		INIT_ERR("error curl_global_init() in get_account_number()")
	if (!(curl = curl_easy_init()))
		INIT_ERR("error curl_easy_init() in get_account_number()")
	
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, token_header);
	if (headers == NULL)
		INIT_ERR("error curl_slist_append() in get_account_number()")
			
	if (curl_easy_setopt(curl, CURLOPT_URL, "https://api.robinhood.com/accounts/") != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &account_number_struct) != CURLE_OK)
		INIT_ERR("error curl_easy_setopt() in get_account_number()")
	if (curl_easy_perform(curl) != CURLE_OK)
		INIT_ERR("error curl_easy_perform() in get_account_number()")
		
	if (parse_account_number_json(account_number_struct.data) < 0)
		INIT_ERR("error parse_account_number_json() in get_account_number()")
	
	free(account_number_struct.data);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}

static int
parse_token_json(char *raw_json)
{
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *token = cJSON_GetObjectItem(parsed_json, "token");
	if (!token)
		INIT_ERR("error finding \"token\" in token json")
	
	snprintf(token_header, 64, "Authorization: Token %s", token->valuestring);
	
	cJSON_Delete(parsed_json);
	return 0;
}
	
static int
get_token(void)
{
	CURL *curl;
	struct json_struct token_struct;
	struct curl_slist *header = NULL;
	
	token_struct.data = (char *)malloc(1);
	if (token_struct.data == NULL)
		INIT_ERR("error allocating token_struct.data")
	token_struct.size = 0;
	
	if (curl_global_init(CURL_GLOBAL_ALL))
		INIT_ERR("error curl_global_init() in get_token()")
	if (!(curl = curl_easy_init()))
		INIT_ERR("error curl_easy_init() in get_token()")
	
	header = curl_slist_append(header, "Accept: application/json");
	if (header == NULL)
		INIT_ERR("error curl_slist_append() in get_token()")
			
	if (curl_easy_setopt(curl, CURLOPT_URL, "https://api.robinhood.com/api-token-auth/") != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, RH_LOGIN) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &token_struct) != CURLE_OK)
		INIT_ERR("error curl_easy_setopt() in get_token()")
	if (curl_easy_perform(curl) != CURLE_OK)
		INIT_ERR("error curl_easy_perform() in get_token()")
		
	if (parse_token_json(token_struct.data) < 0)
		INIT_ERR("error parse_token_json() in get_token()")
	
	free(token_struct.data);
	curl_slist_free_all(header);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}

static int
init_portfolio()
{
	if (get_token() < 0)
		return -1;
	if (get_account_number() < 0)
		return -1;
	snprintf(portfolio_url, 128, "https://api.robinhood.com/accounts/%s/portfolio/", account_number);
	portfolio_init = true;
	
	return 0;
}

static int
populate_tm_struct(void)
{
	time_t tval;
	time(&tval);
	tm_struct = localtime(&tval);
	
	return 0;
}

static int
loop (Display *dpy, Window root)
{
	int weather_return = 0;
	
	while (1) {
		// get times
		populate_tm_struct();
		
		// // run every second
		get_time();
		get_network_usage();
		get_cpu_usage();
		if (weather_update == true && wifi_connected == true)
			if ((weather_return = get_weather()) < 0)
				if (weather_return != -2)
					break;
		if (portfolio_init == false && wifi_connected == true)
			init_portfolio();
		
		// run every five seconds
		if (tm_struct->tm_sec % 5 == 0) {
			get_TODO();
			get_backup_status();
			if (get_portfolio_value() == -1)
				break;
			if (get_wifi_status() < 0)
				break;
			get_memory();
			get_cpu_load();
			get_cpu_temp();
			get_fan_speed();
			get_brightness();
			get_volume();
			get_battery();
		}
		
		// run every minute
		if (tm_struct->tm_sec == 0) {
			get_log_status();
			get_disk_usage();
		}
		
		// run every 3 hours
		if ((tm_struct->tm_hour + 1) % 3 == 0 && tm_struct->tm_min == 0 && tm_struct->tm_sec == 0)
			if (wifi_connected == false)
				weather_update = true;
			else
				if ((weather_return = get_weather()) < 0)
					if (weather_return != -2)
						break;
		
		format_string(dpy, root);
		sleep(1);
	}
	
	return -1;
}

static int
make_urls(void)
{
	snprintf(weather_url, 128, "http://api.openweathermap.org/data/2.5/weather?id=%s&appid=%s&units=imperial", LOCATION, KEY);
	snprintf(forecast_url, 128, "http://api.openweathermap.org/data/2.5/forecast?id=%s&appid=%s&units=imperial", LOCATION, KEY);
	
	return 0;
}

static int
get_vol_range(void)
{
	long min, max;
	
	snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	
	// stolen from amixer utility from alsa-utils
    if ((snd_mixer_open(&handle, 0)) < 0)
		SND_INIT_ERR("error opening volume handle")
    if ((snd_mixer_attach(handle, "default")) < 0)
		SND_INIT_ERR("error attaching volume handle")
    if ((snd_mixer_selem_register(handle, NULL, NULL)) < 0)
		SND_INIT_ERR("error registering volume handle")
    if ((snd_mixer_load(handle)) < 0)
		SND_INIT_ERR("error loading volume handle")
		
	snd_mixer_selem_id_alloca(&sid);
    if (sid == NULL)
		SND_INIT_ERR("error allocating memory for volume id")

	snd_mixer_selem_id_set_name(sid, "Master");
    if (!(elem = snd_mixer_find_selem(handle, sid)))
		SND_INIT_ERR("error finding volume property")
	
	if (snd_mixer_selem_get_playback_volume_range(elem, &min, &max))
		SND_INIT_ERR("error getting volume range")
		
	if (snd_mixer_close(handle))
		SND_INIT_ERR("error closing volume handle")
	handle = NULL;
	snd_config_update_free_global();
	
	return max - min;
}

static int
get_kbd_brightness_max(void)
{
	char file_str[128];
	char dir_str[128];
	DIR *dir;
	struct dirent *file;
	int count = 0;
	FILE *fd;
	int max;
	
	strcpy(file_str, KBD_BRIGHTNESS_FILE);
	strcpy(dir_str, dirname(file_str));
	
	if ((dir = opendir(dir_str)) == NULL)
		INIT_ERR("error opening keyboard brightness directory")
			
	while ((file = readdir(dir)) != NULL)
		if (!strcmp(file->d_name, "max_brightness")) {
			if (strlen(dir_str) + strlen(file->d_name) + 2 > sizeof dir_str)
				break;
			strcat(dir_str, "/");
			strcat(dir_str, file->d_name);
			count++;
			break;
		}
	if (!count)
		INIT_ERR("error finding keyboard brightness directory")
	
	fd = fopen(dir_str, "r");
	if (!fd)
		INIT_ERR("error opening keyboard brightness file")
	fscanf(fd, "%d", &max);
	
	if (fclose(fd) < 0)
		INIT_ERR("error closing keyboard brightness file")
	if (closedir(dir) < 0)
		INIT_ERR("error closing keyboard brightness directory")
	return max;
}

static int
get_screen_brightness_max(void)
{
	char file_str[128];
	char dir_str[128];
	DIR *dir;
	struct dirent *file;
	int count = 0;
	FILE *fd;
	int max;
	
	strcpy(file_str, SCREEN_BRIGHTNESS_FILE);
	strcpy(dir_str, dirname(file_str));
	
	if ((dir = opendir(dir_str)) == NULL)
		INIT_ERR("error opening screen brightness directory")
			
	while ((file = readdir(dir)) != NULL)
		if (!strcmp(file->d_name, "max_brightness")) {
			if (strlen(dir_str) + strlen(file->d_name) + 2 > sizeof dir_str)
				break;
			strcat(dir_str, "/");
			strcat(dir_str, file->d_name);
			count++;
			break;
		}
	if (!count)
		INIT_ERR("error finding screen brightness directory")
	
	fd = fopen(dir_str, "r");
	if (!fd)
		INIT_ERR("error opening screen brightness file")
	fscanf(fd, "%d", &max);
	
	if (fclose(fd) < 0)
		INIT_ERR("error closing screen brightness file")
	if (closedir(dir) < 0)
		INIT_ERR("error closing screen brightness directory")
	return max;
}

static int
get_fan_max(void)
{
	char file_str[128];
	char dir_str[128];
	DIR *dir;
	struct dirent *file;
	char tmp[64];
	char *token;
	FILE *fd;
	int count = 0;
	int max;
	
	strcpy(file_str, FAN_SPEED_FILE);
	strcpy(dir_str, dirname(file_str));
	
	if ((dir = opendir(dir_str)) == NULL)
		INIT_ERR("error opening fan directory")
			
	while ((file = readdir(dir)) != NULL) {
		strcpy(tmp, file->d_name);
		if ((token = strtok(tmp, "_")))
			if ((token = strtok(NULL, "_")))
				if (!strcmp(token, "max")) {
					if (strlen(dir_str) + strlen(file->d_name) + 2 > sizeof dir_str)
						break;
					strcat(dir_str, "/");
					strcat(dir_str, file->d_name);
					count++;
					break;
				}
	}
	if (!count)
		INIT_ERR("error finding fan max file")
	
	fd = fopen(dir_str, "r");
	if (!fd)
		INIT_ERR("error opening file in get_fan_max")
	fscanf(fd, "%d", &max);
	max -= fan_min;
	
	if (fclose(fd) < 0)
		INIT_ERR("error closing file in get_fan_max")
	if (closedir(dir) < 0)
		INIT_ERR("error closing directory in get_fan_max")
	return max;
}

static int
get_fan_min(void)
{
	char file_str[128];
	char dir_str[128];
	DIR *dir;
	struct dirent *file;
	char tmp[64];
	char *token;
	FILE *fd;
	int count = 0;
	int min;
	
	strcpy(file_str, FAN_SPEED_FILE);
	strcpy(dir_str, dirname(file_str));
	
	if ((dir = opendir(dir_str)) == NULL)
		INIT_ERR("error opening fan directory")
			
	while ((file = readdir(dir)) != NULL) {
		strcpy(tmp, file->d_name);
		if ((token = strtok(tmp, "_")))
			if ((token = strtok(NULL, "_")))
				if (!strcmp(token, "min")) {
					if (strlen(dir_str) + strlen(file->d_name) + 2 > sizeof(dir_str))
						break;
					strcat(dir_str, "/");
					strcat(dir_str, file->d_name);
					count++;
					break;
				}
	}
	if (!count)
		INIT_ERR("error finding fan min file")
	
	fd = fopen(dir_str, "r");
	if (!fd)
		INIT_ERR("error opening file in get_fan_min")
	fscanf(fd, "%d", &min);
	
	if (fclose(fd) < 0)
		INIT_ERR("error closing file in get_fan_min")
	if (closedir(dir) < 0)
		INIT_ERR("error closing directory in get_fan_min")
	return min;
}

static int
free_list(struct cpu_temp_list *list)
{
	struct cpu_temp_list *next;
	
	while (list != NULL) {
		next = list->next;
		free(list->filename);
		free(list);
		list = next;
	}
}

static struct cpu_temp_list *
add_link(struct cpu_temp_list *list, char *filename)
{
	struct cpu_temp_list *new;
	struct cpu_temp_list *worm;
	
	new = (struct cpu_temp_list *)malloc(sizeof(struct cpu_temp_list));
	if (new == NULL) {
		fprintf(stderr, "%serror allocating memory for cpu temperature file list\n",
				asctime(tm_struct));
		perror("Error");
		printf("\n");
		return NULL;
	}
	new->filename = (char *)malloc(strlen(filename) + 1);
	if (new->filename == NULL) {
		fprintf(stderr, "%serror allocating memory for cpu temperature file list name\n",
				asctime(tm_struct));
		perror("Error");
		printf("\n");
		return NULL;
	}
	
	strcpy(new->filename, filename);
	new->next = NULL;
	
	if (list == NULL)
		return new;
	else {
		for (worm = list; worm->next != NULL; worm = worm->next);
		worm->next = new;
	}
	
	return list;
}

static struct cpu_temp_list *
populate_temp_list(struct cpu_temp_list *list, char *match)
{
	DIR *dir;
	struct dirent *file;
	char tmp[64];
	char *token;
	int count = 0;
	
	if ((dir = opendir(CPU_TEMP_DIR)) == NULL)
		return NULL;
		
	while ((file = readdir(dir)) != NULL) {
		strcpy(tmp, file->d_name);
		if ((token = strtok(tmp, "_")))
			if ((token = strtok(NULL, "_")))
				if (!strcmp(token, match)) {
					list = add_link(list, file->d_name);
					if (list == NULL) {
						fprintf(stderr, "%serror adding link to cpu temperature file list\n",
								asctime(tm_struct));
						perror("Error");
						printf("\n");
						return NULL;
					}
					count++;
				}
	}
	if (!count) {
		fprintf(stderr, "%serror finding files for cpu temp list\n",
				asctime(tm_struct));
		perror("Error");
		printf("\n");
		return NULL;
	}
	
	if (closedir(dir) < 0) {
		fprintf(stderr, "%serror closing directory in populate_temp_list\n",
				asctime(tm_struct));
		perror("Error");
		printf("\n");
		return NULL;
	}
	return list;
}

static int
get_temp_max(void)
{
	struct cpu_temp_list *max_list = NULL, *snake;
	FILE *fd;
	int max;
	int total = 0;
	int counter;
	
	max_list = populate_temp_list(max_list, "max");
	if (max_list == NULL)
			INIT_ERR("error populating temperature file list in get_temp_max")
	
	for (snake = max_list, counter = 0; snake != NULL; snake = snake->next, counter++) {
		char path[128];
		strcpy(path, CPU_TEMP_DIR);
		strcat(path, snake->filename);
		
		if (!(fd = fopen(path, "r")))
			INIT_ERR("error opening file in get_temp_max")
		if (!fscanf(fd, "%d", &max))
			INIT_ERR("error reading value in get_temp_max")
		if (fclose(fd) < 0)
			INIT_ERR("error closing file in get_temp_max")
		total += max;
	}
	
	free_list(max_list);
	return total / counter;
}

static int
get_cpu_ratio(void)
{
	FILE *fd;
	char line[256];
	char *token;
	int cores;
	int threads;
	
	fd = fopen("/proc/cpuinfo", "r");
	if (!fd)
		INIT_ERR("error opening file in get_cpu_ratio")
	while (fgets(line, 256, fd) != NULL && strncmp(line, "cpu cores", 9));
	if (fclose(fd) < 0)
		INIT_ERR("error closing file in get_cpu_ratio")
			
	token = strtok(line, ":");
	if (token == NULL)
		INIT_ERR("error parsing /proc/cpuinfo to get cpu ratio")
	token = strtok(NULL, ":");
	if (token == NULL)
		INIT_ERR("error parsing /proc/cpuinfo to get cpu ratio")
	
	cores = atoi(token);
	if ((threads = sysconf(_SC_NPROCESSORS_ONLN)) < 0)
		INIT_ERR("error getting threads in get_cpu_ratio")
	
	return threads / cores;
}

static int
get_font(char *font)
{
	FILE *fd;
	char line[256];
	char *token;
	
	if (!(fd = fopen(DWM_CONFIG_FILE, "r")))
		INIT_ERR("error opening file in get_font")
	while (fgets(line, 128, fd) != NULL && strncmp(line, "static const char font[]", 24));
	if (line == NULL)
		INIT_ERR("no font found in config file")
	if (fclose(fd) < 0)
		INIT_ERR("error closing file in get_font")
			
	token = strtok(line, "=");
	if (token == NULL)
		INIT_ERR("error parsing dwm config to get font")
	token = strtok(NULL, "=");
	if (token == NULL)
		INIT_ERR("error parsing dwm config to get font")
			
	while (*token != '"') token++;
	token++;
	token = strtok(token, "\"");
	if (token == NULL)
		INIT_ERR("error parsing dwm config to get font")
			
	memcpy(font, token, strlen(token));
	
	return 0;
}

static int
get_bar_max_len(Display *dpy)
{
	int width_p, width_c;
	char *fontname;
	char **miss_list, *def;
	int count;
	XFontSet fontset;
	XFontStruct *xfont;
	XRectangle rect;
	
	width_p = DisplayWidth(dpy, DefaultScreen(dpy));
	fontname = (char *)malloc(256);
	if (fontname == NULL)
		INIT_ERR("error allocating memory for fontname")
	if (get_font(fontname) < 0)
		return -1;
	fontset = XCreateFontSet(dpy, fontname, &miss_list, &count, &def);
	if (fontset) {
		width_c = XmbTextExtents(fontset, "0", 1, NULL, &rect);
		XFreeFontSet(dpy, fontset);
	} else {
		if (!(xfont = XLoadQueryFont(dpy, fontname))
				&& !(xfont = XLoadQueryFont(dpy, "fixed")))
			INIT_ERR("error loading font for bar\n")
		width_c = XTextWidth(xfont, "0", 1);
		XFreeFont(dpy, xfont);
	}
	
	free(fontname);
	return (width_p / width_c) - 2;
}

static int
get_block_size(void)
{
	if (statvfs("/", &root_fs.fs_stat) < 0)
		INIT_ERR("error getting root file stats")
			
	return root_fs.fs_stat.f_bsize;
}

static int
get_dev_id(void)
{
	int index = if_nametoindex(WIFI_INTERFACE);
	if (!index)
		INIT_ERR("error finding index value for wireless interface")
	return index;
}

static int
get_consts(Display *dpy)
{
	if ((devidx = get_dev_id()) == 0 )
		INIT_ERR("error getting device id")
	if ((block_size = get_block_size()) < 0 )
		INIT_ERR("error getting block size")
	if ((bar_max_len = get_bar_max_len(dpy)) < 0 )
		INIT_ERR("error calculating max bar length")
	if ((cpu_ratio = get_cpu_ratio()) < 0 )
		INIT_ERR("error calculating cpu ratio")
	if ((temp_max = get_temp_max()) < 0 )
		INIT_ERR("error getting max temp")
	if ((fan_min = get_fan_min()) < 0 )
		INIT_ERR("error getting min fan speed")
	if ((fan_max = get_fan_max()) < 0 )
		INIT_ERR("error getting max fan speed")
	if ((screen_brightness_max = get_screen_brightness_max()) < 0 )
		INIT_ERR("error getting max screen brightness")
	if ((kbd_brightness_max = get_kbd_brightness_max()) < 0 )
		INIT_ERR("error getting max keyboard brightness")
	if ((vol_range = get_vol_range()) < 0 )
		INIT_ERR("error getting volume range")
			
	return 0;
}

static int
init(Display *dpy, Window root)
{
	time_t curr_time;
	
	if ((temp_list = populate_temp_list(temp_list, "input")) == NULL)
		INIT_ERR("error opening temperature directory")
	populate_tm_struct();
	if (get_consts(dpy) < 0)
		INIT_ERR("error intializing constants")
	make_urls();
	
	get_TODO();
	get_log_status();
	get_weather();
	get_backup_status();
	get_portfolio_value();
	get_wifi_status();
	get_time();
	
	get_network_usage();
	get_disk_usage();
	get_memory();
	get_cpu_load();
	get_cpu_usage();
	get_cpu_temp();
	get_fan_speed();
	
	get_brightness();
	get_volume();
	get_battery();
	
	time(&curr_time);
	if (format_string(dpy, root) < 0)
		INIT_ERR("error format_string() in init()")
	
	return 0;
}

int
main(void)
{
	static Display *dpy;
	static int screen;
	static Window root;

	dpy = XOpenDisplay(NULL);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	
	if (init(dpy, root) < 0)
		strcpy(statusbar_string, "Initialization failed. Check log for details.");
	else {
		switch (loop(dpy, root)) {
			case 1:
				strcpy(statusbar_string, "Error getting weather. Loop broken. Check log for details.");
				break;
			case 2:
				strcpy(statusbar_string, "Error getting WiFi info. Loop broken. Check log for details.");
				break;
			default:
				strcpy(statusbar_string, "Loop broken. Check log for details.");
				break;
		}
	}
	
	XStoreName(dpy, root, statusbar_string);
	XFlush(dpy);
	
	return -1;
}
