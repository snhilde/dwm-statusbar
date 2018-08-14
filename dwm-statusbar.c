#include "dwm-statusbar.h"

static int
center_bottom_bar(char *bottom_bar)
{
	if (err_flags & 1 << BOTTOMBAR)
		return -1;
	
	int half;
	
	if ( const_bar_max_len < 0 )
		ERR(STRING(BOTTOMBAR), "Cannot find bar_max_len. Please fix issue and restart program,")
	
	if (strlen(STRING(BOTTOMBAR)) < const_bar_max_len) {
		half = (const_bar_max_len - strlen(STRING(BOTTOMBAR))) / 2;
		memmove(STRING(BOTTOMBAR) + half, STRING(BOTTOMBAR), strlen(STRING(BOTTOMBAR)));
		memset(STRING(BOTTOMBAR), ' ', half - 1);
	} else
		memset(STRING(BOTTOMBAR) + strlen(STRING(BOTTOMBAR)) - 3, '.', 3);
	
	return 0;
}

static int
trunc_TODO_string(void)
{
	if (err_flags & 1 << BOTTOMBAR)
		return -1;
	
	int len_avail, i;
	const char *top_strings[5] = {STRING(WEATHER), STRING(BACKUP), STRING(PORTFOLIO),
		STRING(WIFI), STRING(TIME)};
	
	if (const_bar_max_len < 0)
		return -1;
	
	len_avail = const_bar_max_len - 32; // 32 for tags on left side
	for (i = 0; i < 5; i++)
		len_avail -= strlen(top_strings[i]);
	
	if (strlen(STRING(TODO)) > len_avail) {
		memset(STRING(TODO) + len_avail - 4, '.', 3);
		STRING(TODO)[len_avail - 1] = '\0';
	}
	
	return 0;
}

static int
handle_flags(void)
{
	// for (int i = 0; i < NUM_FLAGS; i++) {
		// if (err_flags & 1 << i)
			// snprintf(
	// }
}

static int
format_string(Display *dpy, Window root)
{
	memset(STRING(STATUSBAR), '\0', TOTAL_LENGTH);
	memset(STRING(TOPBAR), '\0', BAR_LENGTH);
	memset(STRING(BOTTOMBAR), '\0', BAR_LENGTH);
	
	handle_flags();
	
	trunc_TODO_string();
			
	strncat(STRING(TOPBAR), STRING(LOG), BAR_LENGTH - strlen(STRING(TOPBAR)));
	strncat(STRING(TOPBAR), STRING(TODO), BAR_LENGTH - strlen(STRING(TOPBAR)));
	strncat(STRING(TOPBAR), STRING(WEATHER), BAR_LENGTH - strlen(STRING(TOPBAR)));
	strncat(STRING(TOPBAR), STRING(BACKUP), BAR_LENGTH - strlen(STRING(TOPBAR)));
	strncat(STRING(TOPBAR), STRING(PORTFOLIO), BAR_LENGTH - strlen(STRING(TOPBAR)));
	strncat(STRING(TOPBAR), STRING(WIFI), BAR_LENGTH - strlen(STRING(TOPBAR)));
	strncat(STRING(TOPBAR), STRING(TIME), BAR_LENGTH - strlen(STRING(TOPBAR)));

	strncat(STRING(BOTTOMBAR), STRING(NETWORK), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	strncat(STRING(BOTTOMBAR), STRING(DISK), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	strncat(STRING(BOTTOMBAR), STRING(RAM), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	strncat(STRING(BOTTOMBAR), STRING(LOAD), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	strncat(STRING(BOTTOMBAR), STRING(CPU_USAGE), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	strncat(STRING(BOTTOMBAR), STRING(CPU_TEMP), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	strncat(STRING(BOTTOMBAR), STRING(FAN), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	
	strncat(STRING(BOTTOMBAR), STRING(BRIGHTNESS), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	strncat(STRING(BOTTOMBAR), STRING(VOLUME), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	strncat(STRING(BOTTOMBAR), STRING(BATTERY), BAR_LENGTH - strlen(STRING(BOTTOMBAR)));
	
	center_bottom_bar(STRING(BOTTOMBAR));
	snprintf(STRING(STATUSBAR), TOTAL_LENGTH, "%s;%s", STRING(TOPBAR), STRING(BOTTOMBAR));
	
	if (!XStoreName(dpy, root, STRING(STATUSBAR)))
		INIT_ERR("error with XStoreName() in format_string()", -1)
	if (!XFlush(dpy))
		INIT_ERR("error with XFlush() in format_string()", -1)
	
	return 0;
}

static int
get_log(void)
{
	if (err_flags & 1 << LOG)
		return -1;
	
	struct stat sb_stat;
	struct stat dwm_stat;

	if (stat(DWM_LOG_FILE, &dwm_stat) < 0)
		ERR(STRING(LOG), "dwm.log error")
	if (stat(STATUSBAR_LOG_FILE, &sb_stat) < 0)
		ERR(STRING(LOG), "dwm-statusbar.log error")
			
	if ((intmax_t)dwm_stat.st_size > 1)
		sprintf(STRING(LOG), "%clog: %c Check DWM Log%c ",
				COLOR_HEADING, COLOR_ERROR, COLOR_NORMAL);
	else if ((intmax_t)sb_stat.st_size > 1)
		sprintf(STRING(LOG), "%clog: %c Check SB Log%c ",
				COLOR_HEADING, COLOR_ERROR, COLOR_NORMAL);
	else
		if (!memset(STRING(LOG), '\0', STRING_LENGTH))
			ERR(STRING(LOG), "error resetting log string")
	
	return 0;
}

static int
get_TODO(void)
{
	if (err_flags & 1 << TODO)
		return -1;
	
	// dumb function
	struct stat file_stat;
	FILE *fd;
	char line[STRING_LENGTH];
	
	if (stat(TODO_FILE, &file_stat) < 0)
		ERR(STRING(TODO), "Error Getting TODO File Stats")
	if (file_stat.st_mtime <= TODO_mtime)
		return 0;
	else
		TODO_mtime = file_stat.st_mtime;
	
	if (!memset(STRING(TODO), '\0', STRING_LENGTH))
		ERR(STRING(TODO), "Error resetting TODO_string")
	
	fd = fopen(TODO_FILE, "r");
	if (!fd)
		ERR(STRING(TODO), "Error Opening TODO File")
			
	// line 1
	if (fgets(line, STRING_LENGTH, fd) == NULL) {
		strncpy(STRING(TODO), "All tasks completed!", STRING_LENGTH - 1);
		return 0;
	}
	line[strlen(line) - 1] = '\0'; // remove weird characters at end
	snprintf(STRING(TODO), STRING_LENGTH, "%cTODO:%c%s",
			COLOR_HEADING, COLOR_NORMAL, line);
	
	// lines 2 and 3
	for (int i = 0; i < 2; i++) {
		memset(line, '\0', STRING_LENGTH);
		if (fgets(line, STRING_LENGTH, fd) == NULL) break;
		line[strlen(line) - 1] = '\0'; // remove weird characters at end
		switch (line[i]) {
			case '\0': break;
			case '\n': break;
			case '\t':
				memmove(line, line + i + 1, strlen(line));
				strncat(STRING(TODO), " -> ", sizeof STRING(TODO) - strlen(STRING(TODO)));
				strncat(STRING(TODO), line, sizeof STRING(TODO) - strlen(STRING(TODO)));
				break;
			default:
				if (i == 1) break;
				strncat(STRING(TODO), " | ", sizeof STRING(TODO) - strlen(STRING(TODO)));
				strncat(STRING(TODO), line, sizeof STRING(TODO) - strlen(STRING(TODO)));
				break;
		}
	}
	
	if (fclose(fd))
		ERR(STRING(TODO), "Error Closing File")
			
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
	char  tmp_str[16];
	
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
		ERR(STRING(WEATHER), "Error finding 'list' in forecast")
	cJSON_ArrayForEach(list_child, list_array) {
		int f_day = get_index(cJSON_GetObjectItem(list_child, "dt"));
		if (f_day > 3)
			break;
		
		main_dict = cJSON_GetObjectItem(list_child, "main");
		if (!main_dict)
			ERR(STRING(WEATHER), "Error finding 'main_dict' in forecast")
		temp_obj = cJSON_GetObjectItem(main_dict, "temp");
		if (!temp_obj)
			ERR(STRING(WEATHER), "Error finding 'temp_obj' in forecast")
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
		strncat(STRING(WEATHER), tmp_str, STRING_LENGTH - strlen(STRING(WEATHER)));
	}
	
	cJSON_Delete(parsed_json);
	return 0;
}

static int
parse_weather_json(char *raw_json)
{
	// for current weather
	// only able to handle rain currently (winter is not coming)
	/*  id  | condition
	   ----------------
	   200s | stormy
	   300s | drizzly
	   500s | rainy
	   700s | low vis
	   800  | clear
	   800s | cloudy */
	
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *main_dict, *temp_obj;
	cJSON *weather_dict;
	int id;
	
	main_dict = cJSON_GetObjectItem(parsed_json, "main");
	if (!main_dict)
		ERR(STRING(WEATHER), "Error finding 'main' in weather")
	temp_obj = cJSON_GetObjectItem(main_dict, "temp");
	if (!temp_obj)
		ERR(STRING(WEATHER), "Error finding 'temp' in weather")
	temp_today = temp_obj->valueint;
	
	weather_dict = cJSON_GetObjectItem(parsed_json, "weather");
	if (!weather_dict)
		ERR(STRING(WEATHER), "Error finding 'weather' in weather")
	weather_dict = weather_dict->child;
	if (!weather_dict)
		ERR(STRING(WEATHER), "Error finding 'weather' in weather")
	id = cJSON_GetObjectItem(weather_dict, "id")->valueint;
	if (!id)
		ERR(STRING(WEATHER), "Error getting id from weather")
	
	snprintf(STRING(WEATHER), STRING_LENGTH, "%c weather:%c%2d F",
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
	if (err_flags & 1 << WEATHER)
		return -1;
	
	if (!memset(STRING(WEATHER), '\0', STRING_LENGTH))
		ERR(STRING(WEATHER), "Error resetting weather_string")
			
	sprintf(STRING(WEATHER), "%c weather:%cN/A ", COLOR_HEADING, COLOR_NORMAL);
	if (wifi_connected == false)
		return -2;
	
	int i;
	struct json_struct json_structs[2];
	static const char *urls[2] = { WEATHER_URL, FORECAST_URL };
	
	curl_easy_reset(sb_curl);
	day_safe = tm_struct->tm_wday;
	
	for (i = 0; i < 2; i++) {
		json_structs[i].data = malloc(1);
		if (json_structs[i].data == NULL)
			ERR(STRING(WEATHER), "Out of memory");
		json_structs[i].size = 0;
		
		if (curl_easy_setopt(sb_curl, CURLOPT_URL, urls[i]) != CURLE_OK ||
				curl_easy_setopt(sb_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
				curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
				curl_easy_setopt(sb_curl, CURLOPT_WRITEDATA, &json_structs[i]) != CURLE_OK)
			ERR(STRING(WEATHER), "Error curl_easy_setops() in get_weather(). Please fix issue and restart.")
		if (curl_easy_perform(sb_curl) == CURLE_OK) {
			if (!i) {
				if (parse_weather_json(json_structs[i].data) < 0)
					ERR(STRING(WEATHER), "Error parsing weather json. Please fix issue and restart.")
			} else {
				if (parse_forecast_json(json_structs[i].data) < 0)
					ERR(STRING(WEATHER), "Error parsing forecast json. Please fix issue and restart.")
			}
			need_to_get_weather = false;
		} else
			break;
		
		free(json_structs[i].data);
	}
	
	return 0;
}

static int
parse_error_code(int code, char *output, int len)
{
	switch (code) {
		case 20:
			strncpy(output, "already done", len - 1); break;
		case 21:
			strncpy(output, "tar error", len - 1); break;
		case 22:
			strncpy(output, "gpg error", len - 1); break;
		case 23:
			strncpy(output, "no acc token", len - 1); break;
		case 24:
			strncpy(output, "error get url", len - 1); break;
		case 25:
			strncpy(output, "token timeout", len - 1); break;
		case 26:
			strncpy(output, "err verifying", len - 1); break;
		default:
			strncpy(output, "err in backup", len - 1);
	}
	
	return 0;
}

static int
get_backup(void)
{
	if (err_flags & 1 << BACKUP)
		return -1;
	
	struct stat file_stat;
	if (stat(BACKUP_STATUS_FILE, &file_stat) < 0)
		ERR(STRING(BACKUP), "Error Getting Backup File Stats")
	if (file_stat.st_mtime <= backup_mtime)
		return 0;
	
	backup_mtime = file_stat.st_mtime;
	
	FILE *fd;
	char line[32], status[16], color = COLOR_ERROR;
	int value, len;
	time_t curr_time;
	time_t t_diff;
	
	len = sizeof status;
	
	if (!memset(STRING(BACKUP), '\0', STRING_LENGTH))
		ERR(STRING(BACKUP), "Error resetting backup string")
	
	if (!(fd = fopen(BACKUP_STATUS_FILE, "r")))
		ERR(STRING(BACKUP), "Error Opening Backup Status File")
			
	if (fgets(line, 32, fd) == NULL)
		ERR(STRING(BACKUP), "No Backup History")
			
	if (fclose(fd))
		ERR(STRING(BACKUP), "Error Closing Backup Status File")
			
	if (isdigit(line[0])) {
		sscanf(line, "%d", &value);
				
		if (value >= 20 && value <= 26)
			parse_error_code(value, status, len);
		else {
			time(&curr_time);
			t_diff = curr_time - value;
			if (t_diff > 86400)
				strncpy(status, "missed", len - 1);
			else {
				strncpy(status, "done", len - 1);
				color = COLOR1;
			}
		}
	} else {
		line[strlen(line) - 1] = '\0';
		strncpy(status, line, len - 1);
		color = COLOR2;
	}
	
	snprintf(STRING(BACKUP), STRING_LENGTH, "%cbackup:%c %s%c ",
			COLOR_HEADING, color, status, COLOR_NORMAL);
		
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
	
	if (need_equity_previous_close) {
		equity_previous_close_obj = cJSON_GetObjectItem(parsed_json, "equity_previous_close");
		if (!equity_previous_close_obj)
			return -1;
		equity_previous_close = atof(equity_previous_close_obj->valuestring);
		need_equity_previous_close = false;
	}
	
	cJSON_Delete(parsed_json);
	return equity_f;
}

static int
get_cst_time(int flag)
{
	time_t seconds;
	int hour;
	struct tm *cst_tm_struct;
	
	time(&seconds);
	seconds += timezone - UTC_TO_CST_SECONDS;	// calculates seconds since epoch for CST
	
	cst_tm_struct = localtime(&seconds);
	
	if (flag == HOUR)
		return cst_tm_struct->tm_hour;
	else if (flag == DAY)
		return cst_tm_struct->tm_wday;
	else
		return -1;
}

static int
run_or_skip(void)
{
	int cst_hour;
	int cst_day;
	
	cst_hour = get_cst_time(HOUR);
	cst_day = get_cst_time(DAY);
	
	if (wifi_connected == false)
		return 2;
	
	if (portfolio_consts_found == false)
		return 2;
	
	if (equity_found == true) {
		// Markets are closed on Saturday and Sunday CST
		if (cst_day == 0 || cst_day == 6)
			return 1;
		// Robinhood starts trading at 9:00 am EST
		else if (cst_hour < 9)
			return 1;
		// Robinhood stops after-market trading at 6:00 pm EST
		else if (cst_hour > 18)
			return 1;
		else
			if (day_equity_previous_close != cst_day) {
				need_equity_previous_close = true;
				day_equity_previous_close = cst_day;
			}
	} else
		day_equity_previous_close = cst_day;
	
	return 0;
}
	
static int
get_portfolio(void)
{
	if (err_flags & 1 << PORTFOLIO)
		return -1;
	
	switch (run_or_skip()) {
		case 0: break;
		case 1: return 0;
		case 2: return -2;
	}
	
	if (!memset(STRING(PORTFOLIO), '\0', STRING_LENGTH))
		ERR(STRING(PORTFOLIO), "Error resetting portfolio_va...")
			
	sprintf(STRING(PORTFOLIO), "%cportfolio:%cN/A",
			COLOR_HEADING, COLOR_NORMAL);
			
	struct json_struct portfolio_jstruct;
	static double equity;
	char equity_string[16];
	
	curl_easy_reset(sb_curl);
	
	portfolio_jstruct.data = malloc(1);
	if (portfolio_jstruct.data == NULL)
		ERR(STRING(PORTFOLIO), "Out of memory");
	portfolio_jstruct.size = 0;
	
	if (curl_easy_setopt(sb_curl, CURLOPT_URL, portfolio_url) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEDATA, &portfolio_jstruct) != CURLE_OK)
		ERR(STRING(PORTFOLIO), "Error curl_easy_setops()")
	if (curl_easy_perform(sb_curl) == CURLE_OK) {
		if ((equity = parse_portfolio_json(portfolio_jstruct.data)) < 0)
			ERR(STRING(PORTFOLIO), "Error parsing portfolio json")
		snprintf(equity_string, sizeof equity_string, "%.2lf", equity);
		
		sprintf(STRING(PORTFOLIO), "%cportfolio:%c%.2lf",
				COLOR_HEADING, equity >= equity_previous_close ? GREEN_TEXT : RED_TEXT, equity);
		equity_found = true;
	}
			
	free(portfolio_jstruct.data);
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
	
	return 0;
}

static int
format_wifi_status(char color, char *ssid_string)
{
	if (strlen(ssid_string) > STRING_LENGTH - 12)
		memset(ssid_string + STRING_LENGTH - 15, '.', 3);
	
	snprintf(STRING(WIFI), STRING_LENGTH, "%c wifi:%c %s%c",
			COLOR_HEADING, color, ssid_string, COLOR_NORMAL);
	
	return 0;
}

static int
print_ssid(uint8_t len, uint8_t *data, char *ssid_string)
{
	// stolen from iw
	int i;
	uint8_t tmp_str[2];

	for (i = 0; i < len && i < STRING_LENGTH; i++) {
		if (isprint(data[i]) && data[i] != ' ' && data[i] != '\\')
			sprintf(tmp_str, "%c", data[i]);
		else if (data[i] == ' ' && (i != 0 && i != len - 1))
			sprintf(tmp_str, " ");
		else
			sprintf(tmp_str, "\\x%.2x", data[i]);
		strncat(ssid_string, tmp_str, STRING_LENGTH - strlen(ssid_string));
	}
	ssid_string[strlen(ssid_string)] = ' ';
	
	return 0;
}

static int
wifi_callback(struct nl_msg *msg, void *arg)
{
	char *ssid_string;
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	uint8_t len;
	uint8_t *data;

	ssid_string = (char *)arg;
	
	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (tb[NL80211_ATTR_SSID]) {
		len = nla_len(tb[NL80211_ATTR_SSID]);
		data = nla_data(tb[NL80211_ATTR_SSID]);
		print_ssid(len, data, ssid_string);
	}
	
	return NL_SKIP;
}

static int
store_nlmsg(const struct sockaddr_nl *who, struct nlmsghdr *hdr, void *arg)
{
	// stolen from iproute2
	struct nlmsg_list **linfo = (struct nlmsg_list**)arg;

	for (linfo; *linfo; linfo = &(*linfo)->next);
	*linfo = malloc(hdr->nlmsg_len + sizeof(void*));
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
	struct nlmsg_list *linfo = NULL;
	struct nlmsg_list *head = NULL;
	
	struct ifinfomsg *ifi;
	struct rtattr *tb[IFLA_MAX+1];
	int len;
	int rv;
	
	if (rtnl_wilddump_request(&sb_rth, AF_PACKET, RTM_GETLINK) < 0)
		ERR(STRING(WIFI), "error: rtnl_wilddump_request")
	if (rtnl_dump_filter(&sb_rth, store_nlmsg, &linfo) < 0)
		ERR(STRING(WIFI), "error: rtnl_dump_filter")
	
	head = linfo;
	for (int i = 1; i < const_devidx; i++, linfo = linfo->next);
	ifi = NLMSG_DATA(&linfo->h);
	if (!ifi)
		ERR(STRING(WIFI), "error accessing ifi")
			
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
get_wifi(void)
{
	if (err_flags & 1 << WIFI)
		return -1;
	
	int ifi_flag;
	int op_state;
	char color = COLOR2;
	char *ssid_string;
	
	if (const_devidx < 0)
		ERR(STRING(WIFI), "Error finding device ID")
	
	ifi_flag = ip_check(0);
	if (ifi_flag == -1) return -1;
	op_state = ip_check(1);
	if (op_state == -1) return -1;
	
	if (ifi_flag == 0 && op_state == 2) {
		strncpy(ssid_string, "Wireless Device Set Down", STRING_LENGTH - 1);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 0) {
		strncpy(ssid_string, "Wireless State Unknown", STRING_LENGTH - 1);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 2) {
		strncpy(ssid_string, "No Connection Initiated", STRING_LENGTH - 1);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 5) {
		strncpy(ssid_string, "No Carrier", STRING_LENGTH - 1);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 6) {
		if (wifi_connected == true)
			return 0;
		if (!memset(STRING(WIFI), '\0', STRING_LENGTH))
			ERR(STRING(WIFI), "Error resetting wifi_string")
		ssid_string = malloc(STRING_LENGTH);
		if (ssid_string == NULL)
			ERR(STRING(WIFI), "err: no memory")
		if (!memset(ssid_string, '\0', STRING_LENGTH))
			ERR(STRING(WIFI), "Error resetting ssid_string")
	
		genlmsg_put(sb_msg, 0, 0, sb_id, 0, 0, NL80211_CMD_GET_INTERFACE, 0);
		if (nla_put(sb_msg, NL80211_ATTR_IFINDEX, sizeof(uint32_t), &const_devidx) < 0)
			ERR(STRING(WIFI), "err: nla_put()")
		nl_send_auto_complete(sb_socket, sb_msg);
		if (nl_cb_set(sb_cb, NL_CB_VALID, NL_CB_CUSTOM, wifi_callback, ssid_string) < 0)
			ERR(STRING(WIFI), "err: nla_cb_set()")
		if (nl_recvmsgs(sb_socket, sb_cb) < 0)
			strncpy(ssid_string, "No Wireless Connection", STRING_LENGTH - 1);
		else
			color = COLOR1;
		
		format_wifi_status(color, ssid_string);

		wifi_connected = true;
		free(ssid_string);
	} else
		ERR(STRING(WIFI), "Error with WiFi Status")
	
	return 0;
}

static int
get_time(void)
{
	if (err_flags & 1 << TIME)
		return -1;
	
	if (!memset(STRING(TIME), '\0', STRING_LENGTH))
		ERR(STRING(TIME), "Error resetting time_string")
	
	if (strftime(STRING(TIME), STRING_LENGTH,  "%b %d - %I:%M", tm_struct) == 0)
		ERR(STRING(TIME), "Error with strftime()")
	if (tm_struct->tm_sec % 2)
		STRING(TIME)[strlen(STRING(TIME)) - 3] = ' ';
	
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
get_network(void)
{
	if (err_flags & 1 << NETWORK)
		return -1;
	
	if (!memset(STRING(NETWORK), '\0', STRING_LENGTH))
		ERR(STRING(NETWORK), "Error resetting network_usage")
	
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
			ERR(STRING(NETWORK), "Read Error")
		fgets(line, 64, fd);
		if (fclose(fd))
			ERR(STRING(NETWORK), "Close Error")
		
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
	
	snprintf(STRING(NETWORK), STRING_LENGTH, "%cnetwork:%c%3d %c/S down,%c%3d %c/S up%c ",
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
	
	bytes_used = (float)(dus->fs_stat.f_blocks - dus->fs_stat.f_bfree) * const_block_size;
	bytes_total = (float)dus->fs_stat.f_blocks *  const_block_size;
	
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
get_disk(void)
{
	if (err_flags & 1 << DISK)
		return -1;
	
	if (!memset(STRING(DISK), '\0', STRING_LENGTH))
		ERR(STRING(DISK), "Error resetting disk_string")
	
	if (const_block_size < 0)
		ERR(STRING(DISK), "   disk: err blksz  ")
	
	int rootperc;

	if (statvfs("/", &root_fs.fs_stat) < 0)
		ERR(STRING(DISK), "Error getting filesystem stats")
	
	process_stat(&root_fs);
	rootperc = rint((double)root_fs.bytes_used / (double)root_fs.bytes_total * 100);
	
	snprintf(STRING(DISK), STRING_LENGTH, " %c disk:%c%.1f%c/%.1f%c%c ", 
			rootperc >= 75? COLOR_WARNING : COLOR_HEADING,
			rootperc >= 75? COLOR_WARNING : COLOR_NORMAL,
			root_fs.bytes_used, root_fs.unit_used, root_fs.bytes_total, root_fs.unit_total,
			COLOR_NORMAL);

	return 0;
}

static int
get_RAM(void)
{
	if (err_flags & 1 << RAM)
		return -1;
	
	if (!memset(STRING(RAM), '\0', STRING_LENGTH))
		ERR(STRING(RAM), "Error resetting RAM string")
	
	int memperc;

	meminfo();
	
	memperc = rint((double)kb_active / (double)kb_main_total * 100);
	if (memperc > 99)
		memperc = 99;
	
	snprintf(STRING(RAM), STRING_LENGTH, " %c RAM:%c%2d%% used%c ",
			memperc >= 75? COLOR_WARNING : COLOR_HEADING,
			memperc >= 75? COLOR_WARNING : COLOR_NORMAL,
			memperc, COLOR_NORMAL);
	
	return 0;
}

static int
get_load(void)
{
	if (err_flags & 1 << LOAD)
		return -1;
	
	if (!memset(STRING(LOAD), '\0', STRING_LENGTH))
		ERR(STRING(LOAD), "Error resetting load_string")
	
	// why was this static?
	double av[3];
	
	loadavg(&av[0], &av[1], &av[2]);
	snprintf(STRING(LOAD), STRING_LENGTH, " %c load:%c%.2f %.2f %.2f%c ",
			av[0] > 1 ? COLOR_WARNING : COLOR_HEADING,
			av[0] > 1 ? COLOR_WARNING : COLOR_NORMAL,
			av[0], av[1], av[2], COLOR_NORMAL);
	
	return 0;
}

static int
get_cpu_usage(void)
{
	if (err_flags & 1 << CPU_USAGE)
		return -1;
	
	// calculation: sum amounts of time cpu spent working vs idle each second, calculate percentage
	if (!memset(STRING(CPU_USAGE), '\0', STRING_LENGTH))
		ERR(STRING(CPU_USAGE), "Error resetting CPU_usage_string")
	
	/* from top.c */
	FILE *fd;
	char line[64];
	int working_new = 0, working_old = 0, total_new = 0, total_old = 0;
	int working_diff, total_diff, total = 0;
	int i;
	
	static struct {
		int old[7];
		int new[7];
	} cpu;
	
	if (const_cpu_ratio < 0)
		const_cpu_ratio = 1;

	fd = fopen(CPU_USAGE_FILE, "r");
	if (!fd)
		ERR(STRING(CPU_USAGE), "Read Error")
	fgets(line, 64, fd);
	if (fclose(fd))
		ERR(STRING(CPU_USAGE), "Close Error")
	
	/* from /proc/stat, cpu time spent in each mode:
	line 1: user mode
	line 2: nice user mode
	line 3: system mode
	line 4: idle mode
	line 5: time spent waiting for I/O to complete
	line 6: time spent on interrupts
	line 7: time servicing softirqs */
	sscanf(line, "cpu %d %d %d %d", &cpu.new[0], &cpu.new[1], &cpu.new[2], &cpu.new[3],
			&cpu.new[4], &cpu.new[5], &cpu.new[6]);

	// exclude first run
	if (cpu.old[0]) {
		for (i = 0; i < 7; i++) {
			total_new += cpu.new[i];
			total_old += cpu.old[i];
			
			if (i == 3) continue;	// skip idle time
			
			working_new += cpu.new[i];
			working_old += cpu.old[i];
		}
		
		working_diff = working_new - working_old + 1;
		total_diff = total_new - total_old + 1;
		
		total = rint((double)working_diff / (double)total_diff * 100);
		total *= const_cpu_ratio;
	}
	
	for (i = 0; i < 7; i++)
		cpu.old[i] = cpu.new[i];
	
	if (total >= 100) total = 99;
	snprintf(STRING(CPU_USAGE), STRING_LENGTH, " %c CPU usage:%c%2d%%%c ",
			total >= 75 ? COLOR_WARNING : COLOR_HEADING,
			total >= 75 ? COLOR_WARNING : COLOR_NORMAL,
			total, COLOR_NORMAL);
	
	return 0;
}

static int
traverse_list(struct file_link *list, char *path, int *num, int *count)
{
	struct file_link *link;
	int i, scan_val, total;
	FILE *fd;
	char file[STRING_LENGTH];
	
	link = list;
	i = 0;
	total = 0;
	while (link != NULL) {
		snprintf(file, STRING_LENGTH, "%s%s", path, link->filename);
		
		if (!(fd = fopen(file, "r")))
			return -1;
		if (!fscanf(fd, "%d", &scan_val))
			return -1;
		if (fclose(fd))
			return -1;
		
		total += scan_val;
		
		link = link->next;
		i++;
	}
	
	*num = total;
	*count = i;
	
	return 0;
}

static int
get_cpu_temp(void)
{
	if (err_flags & 1 << CPU_TEMP)
		return -1;
	
	if (!memset(STRING(CPU_TEMP), '\0', STRING_LENGTH))
		ERR(STRING(CPU_TEMP), "Error resetting CPU_temp_string")
			
	int total, count, temp, tempperc; 
	
	if (traverse_list(therm_list, CPU_TEMP_DIR, &total, &count) < 0)
		ERR(STRING(CPU_TEMP), "Error traversing list in get_cpu_temp()")
	
	temp = total / count;
	
	if (const_temp_max < 0) {
		snprintf(STRING(CPU_TEMP), STRING_LENGTH, " %c CPU temp:%c error %c ",
				COLOR_HEADING,COLOR_WARNING, COLOR_NORMAL);
	} else {
		tempperc = rint((double)temp / (double)const_temp_max * 100);
		temp >>= 10;
		
		snprintf(STRING(CPU_TEMP), STRING_LENGTH, " %c CPU temp:%c%2d degC%c ",
				tempperc >= 75? COLOR_WARNING : COLOR_HEADING,
				tempperc >= 75? COLOR_WARNING : COLOR_NORMAL,
				temp, COLOR_NORMAL);
	}

	return 0;
}

static int
get_fan(void)
{
	if (err_flags & 1 << FAN)
		return -1;
	
	if (!memset(STRING(FAN), '\0', STRING_LENGTH))
		ERR(STRING(FAN), "Error resetting fan_string")
	
	int rpm, count, fanperc;
	
	if (traverse_list(fan_list, FAN_SPEED_DIR, &rpm, &count) < 0)
		ERR(STRING(FAN), "Error traversing list in get_fan()")
			
	if (const_fan_min < 0 || const_fan_max < 0) {
		snprintf(STRING(FAN), STRING_LENGTH, " %c fan: err%c ", COLOR_ERROR, COLOR_NORMAL);
		return -1;
	}
	
	rpm /= count;
	rpm -= const_fan_min;
	if (rpm <= 0)
		rpm = 0;
	
	fanperc = rint((double)rpm / (double)const_fan_max * 100);
	
	if (fanperc >= 100)
		snprintf(STRING(FAN), STRING_LENGTH, " %c fan: MAX%c ", COLOR_ERROR, COLOR_NORMAL);
	else
		snprintf(STRING(FAN), STRING_LENGTH, " %c fan:%c%2d%%%c ",
				fanperc >= 75? COLOR_WARNING : COLOR_HEADING,
				fanperc >= 75? COLOR_WARNING : COLOR_NORMAL,
				fanperc, COLOR_NORMAL);

	return 0;
}

static int
get_brightness(void)
{
	if (err_flags & 1 << BRIGHTNESS)
		return -1;
	
	if (!memset(STRING(BRIGHTNESS), '\0', STRING_LENGTH))
		ERR(STRING(BRIGHTNESS), "Error resetting brightness_string")
			
	if (const_screen_brightness_max < 0 || const_kbd_brightness_max < 0) {
		snprintf(STRING(BRIGHTNESS), STRING_LENGTH, " %c brightness: error %c",
			COLOR_ERROR, COLOR_NORMAL);
		return -1;
	}
	
	const char* b_files[2] = { SCREEN_BRIGHTNESS_FILE, KBD_BRIGHTNESS_FILE };
	
	int scrn, kbd;
	int scrn_perc, kbd_perc;
	
	for (int i = 0; i < 2; i++) {
		if (i == 1 && !DISPLAY_KBD) continue;
		FILE *fd;
		fd = fopen(b_files[i], "r");
		if (!fd)
			ERR(STRING(BRIGHTNESS), "Error w File Open")
		fscanf(fd, "%d", i == 0 ? &scrn : &kbd);
		if (fclose(fd))
			ERR(STRING(BRIGHTNESS), "Error File Close")
	}
	
	scrn_perc = rint((double)scrn / (double)const_screen_brightness_max * 100);
	if (DISPLAY_KBD)
		kbd_perc = rint((double)kbd / (double)const_kbd_brightness_max * 100);
	
	if (DISPLAY_KBD)
		snprintf(STRING(BRIGHTNESS), STRING_LENGTH, " %c brightness:%c%3d%%, %3d%%%c ",
			COLOR_HEADING, COLOR_NORMAL, scrn_perc, kbd_perc, COLOR_NORMAL);
	else
		snprintf(STRING(BRIGHTNESS), STRING_LENGTH, " %c brightness:%c%3d%%%c ",
			COLOR_HEADING, COLOR_NORMAL, scrn_perc, COLOR_NORMAL);
	
	return 0;
}

static int
get_volume(void)
{
	if (err_flags & 1 << VOLUME)
		return -1;
	
	if (!memset(STRING(VOLUME), '\0', STRING_LENGTH))
		ERR(STRING(VOLUME), "Error resetting volume_string")
	
	if (const_vol_range < 0)
		ERR(STRING(VOLUME), "volume: error")
	
	long pvol;
	int swch, volperc;
	
	if (snd_mixer_selem_get_playback_switch(snd_elem, SND_MIXER_SCHN_MONO, &swch))
		ERR(STRING(VOLUME), "Error Get S")
	if (!swch) {
		snprintf(STRING(VOLUME), STRING_LENGTH, " %c volume:%cmute%c ",
				COLOR_HEADING, COLOR_NORMAL, COLOR_NORMAL);
	} else {
		if (snd_mixer_selem_get_playback_volume(snd_elem, SND_MIXER_SCHN_MONO, &pvol))
			ERR(STRING(VOLUME), "Error Get V")
				
		// round to the nearest ten
		volperc = (double)pvol / const_vol_range * 100;
		volperc = rint((float)volperc / 10) * 10;
		
		snprintf(STRING(VOLUME), STRING_LENGTH, " %c volume:%c%3d%%%c ",
				COLOR_HEADING, COLOR_NORMAL, volperc, COLOR_NORMAL);
	}
	
	return 0;
}

static int
get_battery(void)
{
	if (err_flags & 1 << BATTERY)
		return -1;
	
	if (!memset(STRING(BATTERY), '\0', STRING_LENGTH))
		ERR(STRING(BATTERY), "Error resetting battery_string")
	/* from acpi.c and other acpi source files */
	FILE *fd;
	char status_string[20];
	int status; // -1 = discharging, 0 = full, 1 = charging
	int capacity;
	
	const char *filepaths[2] = { BATT_STATUS_FILE, BATT_CAPACITY_FILE };
	
	fd = fopen(BATT_STATUS_FILE, "r");
	if (!fd)
		ERR(STRING(BATTERY), "Err Open Bat Fil")
	fscanf(fd, "%s", &status_string);
	if (fclose(fd))
		ERR(STRING(BATTERY), "Err Close Bat fd")

	if (!strcmp(status_string, "Full") || !strcmp(status_string, "Unknown")) {
		status = 0;
		snprintf(STRING(BATTERY), STRING_LENGTH, " %c battery:%c full %c",
				COLOR_HEADING, COLOR1, COLOR_NORMAL);
		return 0;
	}
	
	if (!strcmp(status_string, "Discharging"))
		status = -1;
	else if (!strcmp(status_string, "Charging"))
		status = 1;
	else
		ERR(STRING(BATTERY), "Err Read Bat Sts")
		
	fd = fopen(BATT_CAPACITY_FILE, "r");
	if (!fd)
		ERR(STRING(BATTERY), "Err Open Bat Fil")
	fscanf(fd, "%d", &capacity);
	if (fclose(fd))
		ERR(STRING(BATTERY), "Err Close Bat fd")
	
	if (capacity > 99)
		capacity = 99;
		
	snprintf(STRING(BATTERY), STRING_LENGTH, " %c battery: %c%2d%% %c",
			capacity < 20 ? COLOR_ERROR : status > 0 ? COLOR2 : COLOR_WARNING,
			status > 0 ? '+' : '-', capacity, COLOR_NORMAL);
	
	return 0;
}

static int
parse_account_number_json(char *raw_json)
{
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *results, *account, *account_num;
	
	results = cJSON_GetObjectItem(parsed_json, "results");
	if (!results)
		return -1;
	account = results->child;
	if (!account)
		return -1;
	account_num = cJSON_GetObjectItem(account, "account_number");
	if (!account_num)
		return -1;
	
	strncpy(account_number, account_num->valuestring, STRING_LENGTH - 1);
	
	cJSON_Delete(parsed_json);
	return 0;
}
	
static int
get_account_number(void)
{
	if (err_flags & 1 << PORTFOLIO)
		return -1;
	
	struct json_struct account_number_struct;
	
	curl_easy_reset(sb_curl);
	
	account_number_struct.data = malloc(1);
	if (account_number_struct.data == NULL)
		INIT_ERR("error allocating account_number_struct.data", -1)
	account_number_struct.size = 0;
	
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, token_header);
	if (headers == NULL)
		INIT_ERR("error curl_slist_append() in get_account_number()", -1)
			
	if (curl_easy_setopt(sb_curl, CURLOPT_URL, "https://api.robinhood.com/accounts/") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEDATA, &account_number_struct) != CURLE_OK)
		INIT_ERR("error curl_easy_setopt() in get_account_number()", -1)
	if (curl_easy_perform(sb_curl) != CURLE_OK)
		INIT_ERR("error curl_easy_perform() in get_account_number()", -1)
		
	if (parse_account_number_json(account_number_struct.data) < 0)
		INIT_ERR("error parse_account_number_json() in get_account_number()", -1)
	
	free(account_number_struct.data);
	return 0;
}

static int
parse_token_json(char *raw_json)
{
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *token = cJSON_GetObjectItem(parsed_json, "token");
	if (!token)
		INIT_ERR("error finding \"token\" in token json", -1)
	
	snprintf(token_header, STRING_LENGTH, "Authorization: Token %s", token->valuestring);
	
	cJSON_Delete(parsed_json);
	return 0;
}
	
static int
get_token(void)
{
	if (err_flags & 1 << WIFI)
		return -1;
	
	struct json_struct token_struct;
	struct curl_slist *header = NULL;
	
	curl_easy_reset(sb_curl);
	
	token_struct.data = malloc(1);
	if (token_struct.data == NULL)
		INIT_ERR("error allocating token_struct.data", -1)
	token_struct.size = 0;
	
	header = curl_slist_append(header, "Accept: application/json");
	if (header == NULL)
		INIT_ERR("error curl_slist_append() in get_token()", -1)
			
	if (curl_easy_setopt(sb_curl, CURLOPT_URL, "https://api.robinhood.com/api-token-auth/") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_HTTPHEADER, header) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_POSTFIELDS, RH_LOGIN) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEDATA, &token_struct) != CURLE_OK)
		INIT_ERR("error curl_easy_setopt() in get_token()", -1)
	if (curl_easy_perform(sb_curl) != CURLE_OK)
		INIT_ERR("error curl_easy_perform() in get_token()", -1)
		
	if (parse_token_json(token_struct.data) < 0)
		INIT_ERR("error parse_token_json() in get_token()", -1)
	
	free(token_struct.data);
	curl_slist_free_all(header);
	return 0;
}

static int
init_portfolio()
{
	if (err_flags & 1 << PORTFOLIO)
		return -1;
	
	if (get_token() < 0)
		return -1;
	if (get_account_number() < 0)
		return -1;
	snprintf(portfolio_url, STRING_LENGTH,
			"https://api.robinhood.com/accounts/%s/portfolio/", account_number);
	portfolio_consts_found = true;
	
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
		get_network();
		get_cpu_usage();
		if (wifi_connected == true) {
			if (need_to_get_weather == true)
				if ((weather_return = get_weather()) < 0)
					if (weather_return != -2)
						break;
			if (portfolio_consts_found == false)
				init_portfolio();
		}
		
		// run every five seconds
		if (tm_struct->tm_sec % 5 == 0) {
			get_log();
			get_TODO();
			get_backup();
			if (get_portfolio() == -1)
				break;
			if (get_wifi() < 0)
				break;
			get_RAM();
			get_load();
			get_cpu_temp();
			get_fan();
			get_brightness();
			get_volume();
			get_battery();
		}
		
		// run every minute
		if (tm_struct->tm_sec == 0) {
			get_disk();
		}
		
		// run every 3 hours
		if ((tm_struct->tm_hour + 1) % 3 == 0 && tm_struct->tm_min == 0 && tm_struct->tm_sec == 0)
			if (wifi_connected == false)
				need_to_get_weather = true;
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
get_vol_range(void)
{
	long min, max;
	
	if (snd_mixer_selem_get_playback_volume_range(snd_elem, &min, &max))
		INIT_ERR("error getting volume range", -1)
		
	return max - min;
}

static int
get_kbd_brightness_max(void)
{
	if (!DISPLAY_KBD)
		return 0;
	
	char file_str[STRING_LENGTH];
	char dir_str[STRING_LENGTH];
	DIR *dir;
	struct dirent *file;
	int count = 0;
	FILE *fd;
	int max;
	
	strncpy(file_str, KBD_BRIGHTNESS_FILE, STRING_LENGTH - 1);
	strncpy(dir_str, dirname(file_str), STRING_LENGTH - 1);
	
	if ((dir = opendir(dir_str)) == NULL)
		INIT_ERR("error opening keyboard brightness directory", -1)
			
	while ((file = readdir(dir)) != NULL)
		if (!strcmp(file->d_name, "max_brightness")) {
			if (strlen(dir_str) + strlen(file->d_name) + 2 > sizeof dir_str)
				break;
			strncat(dir_str, "/", STRING_LENGTH - strlen(dir_str));
			strncat(dir_str, file->d_name, STRING_LENGTH - strlen(dir_str));
			count++;
			break;
		}
	if (!count)
		INIT_ERR("error finding keyboard brightness directory", -1)
	
	fd = fopen(dir_str, "r");
	if (!fd)
		INIT_ERR("error opening keyboard brightness file", -1)
	fscanf(fd, "%d", &max);
	
	if (fclose(fd) < 0)
		INIT_ERR("error closing keyboard brightness file", -1)
	if (closedir(dir) < 0)
		INIT_ERR("error closing keyboard brightness directory", -1)
	return max;
}

static int
get_screen_brightness_max(void)
{
	char file_str[STRING_LENGTH];
	char dir_str[STRING_LENGTH];
	DIR *dir;
	struct dirent *file;
	int count = 0;
	FILE *fd;
	int max;
	
	strncpy(file_str, SCREEN_BRIGHTNESS_FILE, STRING_LENGTH - 1);
	strncpy(dir_str, dirname(file_str), STRING_LENGTH - 1);
	
	if ((dir = opendir(dir_str)) == NULL)
		INIT_ERR("error opening screen brightness directory", -1)
			
	while ((file = readdir(dir)) != NULL)
		if (!strcmp(file->d_name, "max_brightness")) {
			if (strlen(dir_str) + strlen(file->d_name) + 2 > sizeof dir_str)
				break;
			strncat(dir_str, "/", STRING_LENGTH - strlen(dir_str));
			strncat(dir_str, file->d_name, STRING_LENGTH - strlen(dir_str));
			count++;
			break;
		}
	if (!count)
		INIT_ERR("error finding screen brightness directory", -1)
	
	fd = fopen(dir_str, "r");
	if (!fd)
		INIT_ERR("error opening screen brightness file", -1)
	fscanf(fd, "%d", &max);
	
	if (fclose(fd) < 0)
		INIT_ERR("error closing screen brightness file", -1)
	if (closedir(dir) < 0)
		INIT_ERR("error closing screen brightness directory", -1)
	return max;
}

static int
free_list(struct file_link *list)
{
	struct file_link *next;
	
	while (list != NULL) {
		next = list->next;
		free(list->filename);
		free(list);
		list = next;
	}
	
	return 0;
}

static struct file_link *
add_link(struct file_link *list, char *filename)
{
	struct file_link *new;
	int len;
	
	new = malloc(sizeof(struct file_link));
	if (new == NULL)
		return NULL;
			
	len = strlen(filename) + 1;
	new->filename = malloc(len);
	if (new->filename == NULL)
		return NULL;
	
	strncpy(new->filename, filename, len);
	new->next = NULL;
	
	if (list == NULL)
		return new;
	else {
		new->next = list;
		list = new;
	}
	
	return list;
}

static struct file_link *
populate_list(struct file_link *list, char *path, char *base, char *match)
{
	DIR *dir;
	struct dirent *file;
	int count = 0;
	
	if ((dir = opendir(path)) == NULL)
		return NULL;
		
	while ((file = readdir(dir)) != NULL) {
		if (strstr(file->d_name, base) != NULL)
			if (strstr(file->d_name, match) != NULL) {
				list = add_link(list, file->d_name);
				if (list == NULL)
					return NULL;
				count++;
			}
	}
	if (!count)
		return NULL;
	
	if (closedir(dir) < 0)
		return NULL;
	return list;
}

static int
get_gen_consts(char *path, char *base, char *match)
{
	struct file_link *list = NULL, *link;
	int total, count;
	
	list = populate_list(list, path, base, match);
	if (list == NULL)
		INIT_ERR("error populating list in get_gen_consts()", -1)
				
	if (traverse_list(list, path, &total, &count) < 0)
		INIT_ERR("Error traversing list in get_gen_consts()", -1)
	
	free_list(list);
	return total / count;
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
		INIT_ERR("error opening file in get_cpu_ratio()", -1)
	while (fgets(line, 256, fd) != NULL && strncmp(line, "cpu cores", 9));
	if (line == NULL)
		INIT_ERR("error finding line in get_cpu_ratio()", -1)
	if (fclose(fd) < 0)
		INIT_ERR("error closing file in get_cpu_ratio()", -1)
			
	token = strtok(line, ":");
	if (token == NULL)
		INIT_ERR("error parsing /proc/cpuinfo in get_cpu_ratio()", -1)
	token = strtok(NULL, ":");
	if (token == NULL)
		INIT_ERR("error parsing /proc/cpuinfo in get_cpu_ratio()", -1)
	
	cores = atoi(token);
	if ((threads = sysconf(_SC_NPROCESSORS_ONLN)) < 0)
		INIT_ERR("error getting threads in get_cpu_ratio()", -1)
	
	return threads / cores;
}

static int
get_font(char *font)
{
	// TODO strncmp() in while() loop should handle commented out lines
	FILE *fd;
	char line[256];
	char *token;
	
	if (!(fd = fopen(DWM_CONFIG_FILE, "r")))
		INIT_ERR("error opening file in get_font()", -1)
	while (fgets(line, STRING_LENGTH, fd) != NULL && strncmp(line, "static const char font", 22));
	if (line == NULL)
		INIT_ERR("no font found in config file", -1)
	if (fclose(fd) < 0)
		INIT_ERR("error closing file in get_font()", -1)
			
	token = strtok(line, "=");
	if (token == NULL)
		INIT_ERR("error parsing dwm config in get_font()", -1)
	token = strtok(NULL, "=");
	if (token == NULL)
		INIT_ERR("error parsing dwm config in get_font()", -1)
			
	while (*token != '"') token++;
	token++;
	token = strtok(token, "\"");
	if (token == NULL)
		INIT_ERR("error parsing dwm config in get_font()", -1)
			
	if (memcpy(font, token, strlen(token)) == NULL)
		INIT_ERR("error storing font in get_font()", -1)
	
	return 0;
}

static int
get_bar_max_len(Display *dpy)
{
	int width_d, width_c;
	char *fontname;
	char **miss_list, *def;
	int count;
	XFontSet fontset;
	XFontStruct *xfont;
	XRectangle rect;
	
	width_d = DisplayWidth(dpy, DefaultScreen(dpy));
	fontname = malloc(256);
	if (fontname == NULL)
		INIT_ERR("error allocating memory for fontname", -1)
	if (get_font(fontname) < 0)
		INIT_ERR("error getting font in get_bar_max_len()", -1)
	fontset = XCreateFontSet(dpy, fontname, &miss_list, &count, &def);
	if (fontset) {
		width_c = XmbTextExtents(fontset, "0", 1, NULL, &rect);
		XFreeFontSet(dpy, fontset);
	} else {
		if (!(xfont = XLoadQueryFont(dpy, fontname))
				&& !(xfont = XLoadQueryFont(dpy, "fixed")))
			INIT_ERR("error loading font for bar", -1)
		width_c = XTextWidth(xfont, "0", 1);
		XFreeFont(dpy, xfont);
	}
	
	free(fontname);
	return (width_d / width_c) - 2;
}

static int
get_block_size(void)
{
	if (statvfs("/", &root_fs.fs_stat) < 0)
		INIT_ERR("error getting root file stats", -1)
			
	return root_fs.fs_stat.f_bsize;
}

static int
get_dev_id(void)
{
	int index = if_nametoindex(WIFI_INTERFACE);
	if (!index)
		INIT_ERR("error finding index value for wireless interface", -1)
	return index;
}

static int
get_consts(Display *dpy)
{
	int err = 0;
	
	if ((const_devidx = get_dev_id()) == 0 ) {
		SET_FLAG(WIFI);
		err += -1;
		CONST_ERR("error getting device id");
	}
	if ((const_block_size = get_block_size()) < 0 ) {
		SET_FLAG(DISK);
		err += -1;
		CONST_ERR("error getting block size");
	}
	if ((const_bar_max_len = get_bar_max_len(dpy)) < 0 ) {
		SET_FLAG(BOTTOMBAR);
		err += -1;
		CONST_ERR("error calculating max bar length");
	}
	if ((const_cpu_ratio = get_cpu_ratio()) < 0 ) {
		SET_FLAG(CPU_USAGE);
		err += -1;
		CONST_ERR("error calculating cpu ratio");
	}
	if ((const_temp_max = get_gen_consts(CPU_TEMP_DIR, "temp", "max")) < 0 ) {
		SET_FLAG(CPU_TEMP);
		err += -1;
		CONST_ERR("error getting max temp");
	}
	if ((const_fan_min = get_gen_consts(FAN_SPEED_DIR, "fan", "min")) < 0 ) {
		SET_FLAG(FAN);
		err += -1;
		CONST_ERR("error getting min fan speed");
	}
	if ((const_fan_max = get_gen_consts(FAN_SPEED_DIR, "fan", "max")) < 0 ) {
		SET_FLAG(FAN);
		err += -1;
		CONST_ERR("error getting max fan speed");
	}
	if ((const_screen_brightness_max = get_screen_brightness_max()) < 0 ) {
		SET_FLAG(BRIGHTNESS);
		err += -1;
		CONST_ERR("error getting max screen brightness");
	}
	if ((const_kbd_brightness_max = get_kbd_brightness_max()) < 0 ) {
		SET_FLAG(BRIGHTNESS);
		err += -1;
		CONST_ERR("error getting max keyboard brightness");
	}
	if ((const_vol_range = get_vol_range()) < 0 ) {
		SET_FLAG(VOLUME);
		err += -1;
		CONST_ERR("error getting volume range");
	}
			
	return err;
}

static int
populate_lists(void)
{
	int err = 0;
	if ((therm_list = populate_list(therm_list, CPU_TEMP_DIR, "temp", "input")) == NULL) {
		SET_FLAG(CPU_TEMP);
		err = -1;
		INIT_ERR("error creating list of CPU temperature sensors", -1)
	}
	if ((fan_list = populate_list(fan_list, FAN_SPEED_DIR, "fan", "input")) == NULL) {
		SET_FLAG(FAN);
		err += -1;
		INIT_ERR("error creating list of fan speed sensors", -1)
	}
	
	return err;
}

static int
make_vol_singleton(void)
{
	// stolen from amixer utility from alsa-utils
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	
	if (snd_mixer_open(&handle, 0))
		INIT_ERR("error with snd_mixer_open() in make_vol_singleton()", -1)
	if (snd_mixer_attach(handle, "default"))
		INIT_ERR("error with snd_mixer_attach() in make_vol_singleton()", -1)
	if (snd_mixer_selem_register(handle, NULL, NULL))
		INIT_ERR("error with snd_mixer_selem_register() in make_vol_singleton()", -1)
	if (snd_mixer_load(handle))
		INIT_ERR("error with snd_mixer_load() in make_vol_singleton()", -1)
	
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_name(sid, "Master");
	
	if (!(snd_elem = snd_mixer_find_selem(handle, sid)))
		INIT_ERR("error with snd_mixer_find_selem() in make_vol_singleton()", -1)
			
	// if (snd_mixer_close(handle))
		// INIT_ERR("Error Close", -1)
	snd_config_update_free_global();
	
	return 0;
}

static int
make_wifi_singleton(void)
{
	sb_socket = nl_socket_alloc();
	if (!sb_socket)
		INIT_ERR("error with nl_socket_alloc() in make_wifi_singleton()", -1)
	if (genl_connect(sb_socket) < 0)
		INIT_ERR("error with genl_connect() in make_wifi_singleton()", -1)
			
	sb_id = genl_ctrl_resolve(sb_socket, "nl80211");
	if (!sb_id)
		INIT_ERR("error with genl_ctrl_resolve() in make_wifi_singleton()", -1)
	
	sb_msg = nlmsg_alloc();
	if (!sb_msg)
		INIT_ERR("error with nlmsg_alloc() in make_wifi_singleton()", -1)
			
	sb_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!sb_cb)
		INIT_ERR("error with nl_cb_alloc() in make_wifi_singleton()", -1)
			
	if (rtnl_open(&sb_rth, 0) < 0)
		INIT_ERR("error with rtnl_open() in make_wifi_singleton()", -1)
			
	// nlmsg_free(sb_msg);
	// nl_socket_free(sb_socket);
	// free(sb_cb);
	// rtnl_close(&sb_rth);
	
	return 0;
}

static int
make_curl_singleton(void)
{
	if (curl_global_init(CURL_GLOBAL_ALL))
		INIT_ERR("error making curl singleton", -1)
	if (!(sb_curl = curl_easy_init()))
		INIT_ERR("error making curl singleton", -1)
			
	// curl_easy_cleanup(sb_curl);
	curl_global_cleanup();
	return 0;
}

static int
make_singletons(void)
{
	int err = 0;
	
	if ((err += make_curl_singleton()) < 0) {
		SET_FLAG(WEATHER);
		SET_FLAG(PORTFOLIO);
		INIT_ERR("error making curl singleton", -1)
	}
	if ((err += make_wifi_singleton()) < 0) {
		SET_FLAG(WIFI);
		INIT_ERR("error making wifi singleton", -1)
	}
	if ((err += make_vol_singleton()) < 0) {
		SET_FLAG(VOLUME);
		INIT_ERR("error making volume singleton", -1)
	}
	
	return err;
}

static int
alloc_strings(void)
{
	int len;
	char *ptr;
	for (int i = 0; i < NUM_FLAGS; i++) {
		switch (i) {
			case 0: len = TOTAL_LENGTH; break;
			case 1: len = BAR_LENGTH; break;
			case 2: len = BAR_LENGTH; break;
			default: len = STRING_LENGTH;
		}
		ptr = malloc(len);
		if (ptr == NULL) {
			SET_FLAG(i);
			INIT_ERR("error allocating memory in alloc_strings()", -1)
		}
		strings[i] = ptr;
	}
}

static int
init(Display *dpy, Window root)
{
	time_t curr_time;
	int err = 0;
	
	populate_tm_struct();
	err += alloc_strings();
	err += make_singletons();
	err += populate_lists();
	err += get_consts(dpy);
	
	get_log();
	get_TODO();
	get_weather();
	get_backup();
	get_portfolio();
	get_wifi();
	get_time();
	
	get_network();
	get_disk();
	get_RAM();
	get_load();
	get_cpu_usage();
	get_cpu_temp();
	get_fan();
	
	get_brightness();
	get_volume();
	get_battery();
	
	time(&curr_time);
	err += format_string(dpy, root);
	
	return err;
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
	
	if (init(dpy, root) < 0) {
		fprintf(stderr, "%s\terror with initialization\n", asctime(tm_struct));
		perror("\tError");
	}
	switch (loop(dpy, root)) {
		case 1:
			strncpy(STRING(STATUSBAR),
					"Error getting weather. Loop broken. Check log for details.",
					STRING_LENGTH - 1);
			break;
		case 2:
			strncpy(STRING(STATUSBAR),
					"Error getting WiFi info. Loop broken. Check log for details.",
					STRING_LENGTH - 1);
			break;
		default:
			strncpy(STRING(STATUSBAR),
					"Loop broken. Check log for details.",
					STRING_LENGTH - 1);
			break;
	}
	
	XStoreName(dpy, root, STRING(STATUSBAR));
	XFlush(dpy);
	
	return 1;
}
