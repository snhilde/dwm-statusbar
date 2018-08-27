#include "config.h"
#include "dwm-statusbar.h"

static struct string_link *
get_string_link(int id)
{
	struct string_link *link = string_list;
	
	while (link != NULL) {
		if (link->id == id)
			break;
		link = link->next;
	}
	
	return link;
}

static int
get_padding(char *ptr)
{
	int pad;
	
	if (const_bar_max_len > bottom_length)
		pad = (const_bar_max_len - bottom_length) / 2;
	else
		pad = 0;
	
	memset(ptr, ' ', pad);
	
	return pad;
}

static int
format_top_bar(char **ptr)
{
	// TODO does not handle the resizing of TODO_string
	// if ((*ptr - statusbar) > const_bar_max_len - 32) {
		// memset(statusbar + const_bar_max_len - 35, '.', 3);
		// *ptr = statusbar + const_bar_max_len - 32;
	// }
	**ptr = ';';
	(*ptr)++;

	return 0;
}

static int
get_length(struct string_link *link)
{
	if (GET_FLAG(err, link->id))
		return strlen(link->heading) + 9;
	else {
		if (*link->info)
			return strlen(link->heading) + strlen(link->info);
		else
			return 0;
	}
}

static int
copy_string(struct string_link *link, char *start)
{
	int head_len, info_len, total_len;
	
	head_len = strlen(link->heading);
	info_len = strlen(link->info);
	total_len = link->len;
	
	if (link->id == TODO) {
		if (head_len < total_len)
			info_len = total_len - head_len;
		else {
			head_len = total_len;
			info_len = 0;
		}
	}
	
	if (total_len != head_len + info_len)
		ERR(link->id, "error calculating correct lengths in copy_string()", 0);
	
	memcpy(start, link->heading, head_len);
	memcpy(start + head_len, link->info, info_len);
	
	return total_len;
}

static int
handle_TODO(void)
{
	struct string_link *link;
	int new_len;
	
	link = get_string_link(TODO);
	if (!link)
		ERR(TODO, "error getting TODO link in handle_TODO()", -1);
			
	new_len = MIN(strlen(link->heading) + strlen(link->info),
					const_bar_max_len - 32 - *link->bar_len); // 32 for tags on left side
	
	if (new_len != link->len) {
		link->len = new_len;
		update_all = true;
	}
	
	return 0;
}

static int
handle_strings(Display *dpy, Window root)
{
	char *start;
	struct string_link *link;
	
	if (update_all || GET_FLAG(updated, TODO))
		handle_TODO();
	
	start = statusbar;
	link = string_list;
	
	while (link) {
		if (link->len) {
			if (update_all || GET_FLAG(updated, link->id)) {
				start += copy_string(link, start);
				REMOVE_FLAG(updated, link->id);
			} else {
				start += link->len;
			}
		}
		if (separator == link->id) {
			format_top_bar(&start);
			start += get_padding(start);
		}
		link = link->next;
	}
	*start = '\0';
	update_all = false;
	
	if (!XStoreName(dpy, root, statusbar))
		ERR(STATUSBAR, "error with XStoreName() in handle_strings()", -1);
	if (!XFlush(dpy))
		ERR(STATUSBAR, "error with XFlush() in handle_strings()", -1);
	
	return 0;
}

static int
get_log(void)
{
	if (GET_FLAG(err, LOG))
		return -1;
	
	struct string_link *link;
	struct stat sb_stat;
	struct stat dwm_stat;
	long mtime;
	static long old_mtime;

	link = get_string_link(LOG);
	if (!link)
		ERR(LOG, "error getting string link in get_log()", -1);
	
	if (stat(DWM_LOG_FILE, &dwm_stat) < 0)
		ERR(LOG, "error getting dwm.log file stats in get_log()", -1);
	if (stat(STATUSBAR_LOG_FILE, &sb_stat) < 0)
		ERR(LOG, "error getting dwm-statusbar.log file stats in get_log()", -1);
			
	mtime = MAX(dwm_stat.st_mtime, sb_stat.st_mtime);
	if (old_mtime != mtime) {
		old_mtime = mtime;
			
		if ((intmax_t)dwm_stat.st_size > 1)
			snprintf(link->info, STRING_LENGTH, "%c Check DWM Log%c ", COLOR_ERROR, COLOR_NORMAL);
		else if ((intmax_t)sb_stat.st_size > 1)
			snprintf(link->info, STRING_LENGTH, "%c Check SB Log%c ", COLOR_ERROR, COLOR_NORMAL);
		else if (*link->info)
			memset(link->info, '\0', strlen(link->info));
		
		SET_FLAG(updated, LOG);
		CHECK_LENGTH(LOG);
	}
	return 0;
}

static int
get_TODO(void)
{
	if (GET_FLAG(err, TODO))
		return -1;
	
	struct stat file_stat;
	static long old_mtime;
	if (stat(TODO_FILE, &file_stat) < 0)
		ERR(TODO, "error getting TODO file stats in get_TODO()", -1);
	if (old_mtime != file_stat.st_mtime) {
		old_mtime = file_stat.st_mtime;
		
		// dumb function
		struct string_link *link;
		FILE *fd;
		char line[STRING_LENGTH];
		
		link = get_string_link(TODO);
		if (!link)
			ERR(TODO, "error getting string link in get_TODO()", -1);
	
		fd = fopen(TODO_FILE, "r");
		if (!fd)
			ERR(TODO, "error opening TODO file in get_TODO()", -1);
				
		// line 1
		if (!fgets(line, STRING_LENGTH, fd)) {
			strncpy(link->info, "All tasks completed!", STRING_LENGTH - 1);
			return 0;
		}
		line[strlen(line) - 1] = '\0'; // remove weird characters at end
		snprintf(link->info, STRING_LENGTH, "%c%s ", COLOR_NORMAL, line);
		
		// lines 2 and 3
		for (int i = 0; i < 2; i++) {
			memset(line, '\0', STRING_LENGTH);
			if (!fgets(line, STRING_LENGTH, fd)) break;
			line[strlen(line) - 1] = '\0'; // remove weird characters at end
			switch (line[i]) {
				case '\0': break;
				case '\n': break;
				case '\t':
					memmove(line, line + i + 1, strlen(line));
					strncat(link->info, " -> ", -2);
					strncat(link->info, line, STRING_LENGTH - strlen(link->info));
					break;
				default:
					if (i == 1) break;
					strncat(link->info, " | ", STRING_LENGTH - strlen(link->info));
					strncat(link->info, line, STRING_LENGTH - strlen(link->info));
					break;
			}
		}
		
		if (fclose(fd))
			ERR(TODO, "error closing TODO file in get_TODO()", -1);
				
		SET_FLAG(updated, TODO);
	}
			
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
parse_forecast_json(char *raw_json, char *info)
{
	// for 5-day forecast (sent as 3-hour intervals for 5 days)
	// only able to handle rain currently
	
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *list_array, *list_child;
	cJSON *main_dict, *temp_obj;
	cJSON *rain_dict, *rain_obj;
	int i;
	char  tmp_str[64];
	
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
		ERR(WEATHER, "error finding 'list' in forecast json in parse_forecast_json()", -1);
	cJSON_ArrayForEach(list_child, list_array) {
		int f_day = get_index(cJSON_GetObjectItem(list_child, "dt"));
		if (f_day > 3)
			break;
		
		main_dict = cJSON_GetObjectItem(list_child, "main");
		if (!main_dict)
			ERR(WEATHER, "error finding 'main_dict' in forecast json in parse_forecast_json()", -1);
		temp_obj = cJSON_GetObjectItem(main_dict, "temp");
		if (!temp_obj)
			ERR(WEATHER, "error finding 'temp_obj' in forecast json in parse_forecast_json()", -1);
		data[f_day].high = (int)fmax(data[f_day].high, temp_obj->valueint);
		data[f_day].low = (int)fmin(data[f_day].low, temp_obj->valueint);
		
		if (rain_dict = cJSON_GetObjectItem(list_child, "rain"))
			if (rain_obj = rain_dict->child)
				data[f_day].precipitation += rain_obj->valuedouble;
	}
	
	for (i = 0; i < 4; i++) {
		snprintf(tmp_str, 64, "%c %s(%2d/%2d)%c ",
				data[i].precipitation >= 3 ? COLOR_WARNING : COLOR_NORMAL,
				i > 0 ? days_of_week[day_safe + i - 1] : "",
				data[i].high, data[i].low, COLOR_NORMAL);
		strncat(info, tmp_str, STRING_LENGTH - strlen(info));
	}
	
	cJSON_Delete(parsed_json);
	return 0;
}

static int
parse_weather_json(char *raw_json, char *info)
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
		ERR(WEATHER, "error finding 'main' in weather json in parse_weather_json()", -1);
	temp_obj = cJSON_GetObjectItem(main_dict, "temp");
	if (!temp_obj)
		ERR(WEATHER, "error finding 'temp' in weather json in parse_weather_json()", -1);
	temp_today = temp_obj->valueint;
	
	weather_dict = cJSON_GetObjectItem(parsed_json, "weather");
	if (!weather_dict)
		ERR(WEATHER, "error finding 'weather' in weather json in parse_weather_json()", -1);
	weather_dict = weather_dict->child;
	if (!weather_dict)
		ERR(WEATHER, "error finding 'weather' in weather json in parse_weather_json()", -1);
	id = cJSON_GetObjectItem(weather_dict, "id")->valueint;
	if (!id)
		ERR(WEATHER, "error getting id from weather json in parse_weather_json()", -1);
	
	snprintf(info, STRING_LENGTH, "%c %2d F",
			id < 800 ? COLOR_WARNING : COLOR_NORMAL, temp_today);
	
	cJSON_Delete(parsed_json);
	return 0;
}

size_t
curl_callback(char *weather_json, size_t size, size_t nmemb, void *userdata)
{
	const size_t received_size = size * nmemb;
	struct data_struct *tmp;
	
	tmp = (struct data_struct *)userdata;
	
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
	if (GET_FLAG(err, WEATHER))
		return -1;
	
	struct string_link *link;
	int i;
	struct data_struct json_structs[2];
	static const char *urls[2] = { WEATHER_URL, FORECAST_URL };
	
	link = get_string_link(WEATHER);
	if (!link)
		ERR(WEATHER, "error getting string link in get_weather()", -1);
	
	snprintf(link->info, STRING_LENGTH, "%cN/A ", COLOR_NORMAL);
	if (!internet_connected) {
		need_to_get_weather = true;
		SET_FLAG(updated, WEATHER);
		CHECK_LENGTH(WEATHER);
		return -1;
	}
	
	curl_easy_reset(sb_curl);
	day_safe = tm_struct->tm_wday;
	
	for (i = 0; i < 2; i++) {
		json_structs[i].data = malloc(1);
		if (!json_structs[i].data)
			ERR(WEATHER, "error allocating memory for data_struct in get_weather()", -1);
		json_structs[i].size = 0;
		
		if (curl_easy_setopt(sb_curl, CURLOPT_URL, urls[i]) != CURLE_OK ||
				curl_easy_setopt(sb_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
				curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
				curl_easy_setopt(sb_curl, CURLOPT_WRITEDATA, &json_structs[i]) != CURLE_OK)
			ERR(WEATHER, "error with curl_easy_setops() in get_weather()", -1);
		if (curl_easy_perform(sb_curl) == CURLE_OK) {
			if (!i) {
				if (parse_weather_json(json_structs[i].data, link->info) < 0)
					ERR(WEATHER, "error parsing weather json in get_weather()", -1);
			} else {
				if (parse_forecast_json(json_structs[i].data, link->info) < 0)
					ERR(WEATHER, "error parsing forecast json in get_weather()", -1);
			}
			need_to_get_weather = false;
		} else
			ERR(WEATHER, "error performing curl_easy_perform() in get_weather()", -1);
		
		free(json_structs[i].data);
	}
	
	SET_FLAG(updated, WEATHER);
	CHECK_LENGTH(WEATHER);
	
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
	if (GET_FLAG(err, BACKUP))
		return -1;
	
	struct stat file_stat;
	static long old_mtime;
	
	if (stat(BACKUP_STATUS_FILE, &file_stat) < 0)
		ERR(BACKUP, "error getting backup file stats in get_backup()", -1);
	if (old_mtime != file_stat.st_mtime) {
		old_mtime = file_stat.st_mtime;
		backup_occurring = false;
		
		struct string_link *link;
		FILE *fd;
		char line[32], status[16], color = COLOR_ERROR;
		int value, len;
		time_t curr_time;
		time_t t_diff;
		
		link = get_string_link(BACKUP);
		if (!link)
			ERR(BACKUP, "error getting string link in get_backup()", -1);
		
		len = sizeof status;
		
		if (!(fd = fopen(BACKUP_STATUS_FILE, "r")))
			ERR(BACKUP, "error opening backup status file in get_backup()", -1);
				
		if (!fgets(line, 32, fd))
			ERR(BACKUP, "backup history file is empty in get_backup()", -1);
				
		if (fclose(fd))
			ERR(BACKUP, "error closing backup status file in get_backup()", -1);
				
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
		
		snprintf(link->info, STRING_LENGTH, "%c %s%c ",
				color, status, COLOR_NORMAL);
		
		SET_FLAG(updated, BACKUP);
		CHECK_LENGTH(BACKUP);
		backup_occurring = true;
	}
		
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
		ERR(PORTFOLIO, "error finding 'equity' in portfolio json in parse_portfolio_json()", -1);
	
	extended_hours_equity_obj = cJSON_GetObjectItem(parsed_json, "extended_hours_equity");
	if (!extended_hours_equity_obj)
		ERR(PORTFOLIO, "error finding 'extended_hours_equity' in portfolio json in parse_portfolio_json()", -1);
	
	if (!extended_hours_equity_obj->valuestring)
		equity_f = atof(equity_obj->valuestring);
	else
		equity_f = atof(extended_hours_equity_obj->valuestring);
	
	if (need_equity_previous_close) {
		equity_previous_close_obj = cJSON_GetObjectItem(parsed_json, "equity_previous_close");
		if (!equity_previous_close_obj)
			ERR(PORTFOLIO, "error finding 'equity_previous_close' in portfolio json in parse_portfolio_json()", -1);
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
	
	if (!internet_connected)
		return 2;
	
	if (!portfolio_consts_found)
		return 2;
	
	if (equity_found) {
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
	if (GET_FLAG(err, PORTFOLIO))
		return -1;
	
	struct string_link *link;
	struct data_struct portfolio_jstruct;
	double equity;
	static double old_equity;
	
	link = get_string_link(PORTFOLIO);
	if (!link)
		ERR(PORTFOLIO, "error getting string link in get_portfolio()", -1);
	
	switch (run_or_skip()) {
		case 0: break;
		case 1: return 0;
		case 2: snprintf(link->info, STRING_LENGTH, "%cN/A ", COLOR_NORMAL);
				SET_FLAG(updated, PORTFOLIO);
				CHECK_LENGTH(PORTFOLIO);
				return -2;
	}
	
	curl_easy_reset(sb_curl);
	
	portfolio_jstruct.data = malloc(1);
	if (!portfolio_jstruct.data)
		ERR(PORTFOLIO, "error allocating memory for portfolio_jstruct in get_portfolio()", -1);
	portfolio_jstruct.size = 0;
	
	if (curl_easy_setopt(sb_curl, CURLOPT_URL, portfolio_url) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEDATA, &portfolio_jstruct) != CURLE_OK)
		ERR(PORTFOLIO, "error with curl_easy_setops() in get_portfolio()", -1);
	if (curl_easy_perform(sb_curl) == CURLE_OK) {
		if ((equity = parse_portfolio_json(portfolio_jstruct.data)) < 0)
			ERR(PORTFOLIO, "error parsing portfolio json in get_portfolio()", -1);
				
		if (old_equity != equity) {
			old_equity = equity;
			
			snprintf(link->info, STRING_LENGTH, "%c%.2lf ",
					equity >= equity_previous_close ? GREEN_TEXT : RED_TEXT, equity);
			
			SET_FLAG(updated, PORTFOLIO);
			CHECK_LENGTH(PORTFOLIO);
			equity_found = true;
		}
	} else
		ERR(PORTFOLIO, "error performing curl_easy_perform() in get_portfolio()", -1);
	
	free(portfolio_jstruct.data);
	return 0;
}

static int
free_wifi_list(struct nlmsg_list *list)
{
	struct nlmsg_list *next;
	
	while (list) {
		next = list->next;
		free(list);
		list = next;
	}
	
	return 0;
}

static int
format_wifi_status(char *ssid_string, char *info, char color)
{
	if (GET_FLAG(err, WIFI))
		return 0;
	
	if (strlen(ssid_string) > STRING_LENGTH - 12)
		memset(ssid_string + STRING_LENGTH - 15, '.', 3);
	
	snprintf(info, STRING_LENGTH, "%c %s%c ",
			color, ssid_string, COLOR_NORMAL);
	
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
			snprintf(tmp_str, STRING_LENGTH, "%c", data[i]);
		else if (data[i] == ' ' && (i != 0 && i != len - 1))
			snprintf(tmp_str, STRING_LENGTH, " ");
		else
			snprintf(tmp_str, STRING_LENGTH, "\\x%.2x", data[i]);
		strncat(ssid_string, tmp_str, STRING_LENGTH - strlen(ssid_string));
	}
	
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
	if (!*linfo)
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
		ERR(WIFI, "error with rtnl_wilddump_request() in ip_check()", -1);
	if (rtnl_dump_filter(&sb_rth, store_nlmsg, &linfo) < 0)
		ERR(WIFI, "error with rtnl_dump_filter() in ip_check()", -1);
	
	head = linfo;
	for (int i = 1; i < const_devidx; i++, linfo = linfo->next);
	ifi = NLMSG_DATA(&linfo->h);
	if (!ifi)
		ERR(WIFI, "error initializing ifi in ip_check()", -1);
			
	len = linfo->h.nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));
	parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);
	
	if (flag)
		// 2 if down, 6 if up, 5 if up but not connected
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
	if (GET_FLAG(err, WIFI))
		return -1;
	
	struct string_link *link;
	int ifi_flag;
	int op_state;
	char color = COLOR2;
	char *ssid_string;
	
	link = get_string_link(WIFI);
	if (!link)
		ERR(WIFI, "error getting string link in get_wifi()", -1);
	
	ifi_flag = ip_check(0);
	if (ifi_flag == -1) return -1;
	op_state = ip_check(1);
	if (op_state == -1) return -1;
	
	ssid_string = calloc(1, STRING_LENGTH);
	if (!ssid_string)
		ERR(WIFI, "error allocating memory for ssid_string in get_wifi()", -1);
	
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
		strncpy(ssid_string, "Not Connected", STRING_LENGTH - 1);
		wifi_connected = false;
	} else if (ifi_flag && op_state == 6) {
		if (wifi_connected)
			return 0;
	
		genlmsg_put(sb_msg, 0, 0, sb_id, 0, 0, NL80211_CMD_GET_INTERFACE, 0);
		if (nla_put(sb_msg, NL80211_ATTR_IFINDEX, sizeof(uint32_t), &const_devidx) < 0)
			ERR(WIFI, "error with nla_put() in get_wifi()", -1);
		nl_send_auto_complete(sb_socket, sb_msg);
		if (nl_cb_set(sb_cb, NL_CB_VALID, NL_CB_CUSTOM, wifi_callback, ssid_string) < 0)
			ERR(WIFI, "error with nla_cb_set() in get_wifi()", -1);
		if (nl_recvmsgs(sb_socket, sb_cb) < 0)
			strncpy(ssid_string, "No Wireless Connection", STRING_LENGTH - 1);
		else
			color = COLOR1;

		wifi_connected = true;
	} else
		ERR(WIFI, "error finding wifi status in get_wifi()", -1);
			
	format_wifi_status(ssid_string, link->info, color);
	SET_FLAG(updated, WIFI);		
	CHECK_LENGTH(WIFI);		
	
	free(ssid_string);
	return 0;
}

static int
get_time(void)
{
	if (GET_FLAG(err, TIME))
		return -1;
	
	struct string_link *link;
	
	link = get_string_link(TIME);
	if (!link)
		ERR(TIME, "error getting string link in get_time()", -1);
	
	if (strftime(link->info, STRING_LENGTH,  "%b %d - %I:%M", tm_struct) == 0)
		ERR(TIME, "error with strftime() in get_time()", -1);
	if (tm_struct->tm_sec % 2)
		link->info[strlen(link->info) - 3] = ' ';
	
	SET_FLAG(updated, TIME);
	CHECK_LENGTH(TIME);
	
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
	if (GET_FLAG(err, NETWORK))
		return -1;
	
	/* from top.c */
	struct string_link *link;
	const char* files[2] = { NET_RX_FILE, NET_TX_FILE };
	FILE *fd;
	char line[64];
	static long rx_old, tx_old, old_rx_bps, old_tx_bps;
	long rx_new, tx_new;
	long rx_bps, tx_bps;
	int step = 0;
	char rx_unit, tx_unit;
	static char old_rx_unit, old_tx_unit;
	
	link = get_string_link(NETWORK);
	if (!link)
		ERR(NETWORK, "error getting string link in get_network()", -1);
	
	for (int i = 0; i < 2; i++) {
		fd = fopen(files[i], "r");
		if (!fd)
			ERR(NETWORK, "error opening a network file in get_network()", -1);
		fgets(line, 64, fd);
		if (fclose(fd))
			ERR(NETWORK, "error closing a network file in get_network()", -1);
		
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
	
	rx_old = rx_new;
	tx_old = tx_new;
	
	if (old_rx_bps != rx_bps ||
		old_tx_bps != tx_bps ||
		old_rx_unit != rx_unit ||
		old_tx_unit != tx_unit) {
		old_rx_bps = rx_bps;
		old_tx_bps = tx_bps;
		old_rx_unit = rx_unit;
		old_tx_unit = tx_unit;
	
		snprintf(link->info, STRING_LENGTH, "%c%3d %c/S down,%c%3d %c/S up%c ",
				rx_unit == 'M' ? COLOR_WARNING : COLOR_NORMAL, rx_bps, rx_unit,
				tx_unit == 'M' ? COLOR_WARNING : COLOR_NORMAL, tx_bps, tx_unit, COLOR_NORMAL);
	
		SET_FLAG(updated, NETWORK);
		CHECK_LENGTH(NETWORK);
	}
	
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
	if (GET_FLAG(err, DISK))
		return -1;
	
	struct string_link *link;
	int rootperc;
	static int old_rootperc;

	link = get_string_link(DISK);
	if (!link)
		ERR(DISK, "error getting string link in get_disk()", -1);
	
	if (statvfs("/", &root_fs.fs_stat) < 0)
		ERR(DISK, "error getting filesystem stats in get_disk()", -1);
	
	process_stat(&root_fs);
	rootperc = rint((double)root_fs.bytes_used / (double)root_fs.bytes_total * 100);
	
	if (old_rootperc != rootperc) {
		old_rootperc = rootperc;
	
		snprintf(link->info, STRING_LENGTH, "%c %.1f%c/%.1f%c%c ", 
				rootperc >= 75? COLOR_WARNING : COLOR_NORMAL,
				root_fs.bytes_used, root_fs.unit_used, root_fs.bytes_total, root_fs.unit_total,
				COLOR_NORMAL);
		
		SET_FLAG(updated, DISK);
		CHECK_LENGTH(DISK);
	}

	return 0;
}

static int
get_RAM(void)
{
	if (GET_FLAG(err, RAM))
		return -1;
	
	struct string_link *link;
	int memperc;
	static int old_memperc;

	link = get_string_link(RAM);
	if (!link)
		ERR(RAM, "error getting string link in get_RAM()", -1);
	
	meminfo();
	
	memperc = rint((double)kb_active / (double)kb_main_total * 100);
	if (memperc > 99)
		memperc = 99;
	
	if (old_memperc != memperc) {
		old_memperc = memperc;
	
		snprintf(link->info, STRING_LENGTH, "%c %2d%% used%c ",
				memperc >= 75? COLOR_WARNING : COLOR_NORMAL,
				memperc, COLOR_NORMAL);
	
		SET_FLAG(updated, RAM);
		CHECK_LENGTH(RAM);
	}
	
	return 0;
}

static int
get_load(void)
{
	if (GET_FLAG(err, LOAD))
		return -1;
	
	struct string_link *link;
	double loadavg[3];
	static int old_loadavg[3];
	
	link = get_string_link(LOAD);
	if (!link)
		ERR(LOAD, "error getting string link in get_load()", -1);
	
	getloadavg(loadavg, 3);
	
	if (old_loadavg[0] != loadavg[0] * 100 ||
		old_loadavg[1] != loadavg[1] * 100 ||
		old_loadavg[2] != loadavg[2] * 100) {
		for (int i = 0; i < 3; i++)
			old_loadavg[i] = loadavg[i] * 100;
		
		snprintf(link->info, STRING_LENGTH, "%c %.2f %.2f %.2f%c ",
				loadavg[0] > 1 ? COLOR_WARNING : COLOR_NORMAL,
				loadavg[0], loadavg[1], loadavg[2], COLOR_NORMAL);
		
		SET_FLAG(updated, LOAD);
		CHECK_LENGTH(LOAD);
	}
	
	return 0;
}

static int
get_cpu_usage(void)
{
	if (GET_FLAG(err, CPU_USAGE))
		return -1;
	
	// calculation: sum amounts of time cpu spent working vs idle each second, calculate percentage
	/* from top.c */
	struct string_link *link;
	FILE *fd;
	char line[64];
	int working_new = 0, working_old = 0, total_new = 0, total_old = 0;
	int working_diff, total_diff, total;
	int i;
	static int old_total;
	
	link = get_string_link(CPU_USAGE);
	if (!link)
		ERR(CPU_USAGE, "error getting string link in get_cpu_usage()", -1);
	
	static struct {
		int old[7];
		int new[7];
	} cpu;
	
	if (const_cpu_ratio < 0)
		const_cpu_ratio = 1;

	fd = fopen(CPU_USAGE_FILE, "r");
	if (!fd)
		ERR(CPU_USAGE, "error opening CPU usage file in get_cpu_usage()", -1);
	fgets(line, 64, fd);
	if (fclose(fd))
		ERR(CPU_USAGE, "error closing CPU usage file in get_cpu_usage()", -1);
	
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
	
	if (old_total != total) {
		old_total = total;
		
		snprintf(link->info, STRING_LENGTH, "%c %2d%%%c ",
				total >= 75 ? COLOR_WARNING : COLOR_NORMAL,
				total, COLOR_NORMAL);
		
		SET_FLAG(updated, CPU_USAGE);
		CHECK_LENGTH(CPU_USAGE);
	}
	
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
	while (link) {
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
	if (GET_FLAG(err, CPU_TEMP))
		return -1;
	
	struct string_link *link;
	int total, count, temp, tempperc; 
	static int old_temp;
	
	link = get_string_link(CPU_TEMP);
	if (!link)
		ERR(CPU_TEMP, "error getting string link in get_cpu_temp()", -1);
	
	if (traverse_list(therm_list, CPU_TEMP_DIR, &total, &count) < 0)
		ERR(CPU_TEMP, "error traversing list in get_cpu_temp()", -1);
	
	temp = total / count;
	
	tempperc = rint((double)temp / (double)const_temp_max * 100);
	temp >>= 10;
	
	
	if (old_temp != temp) {
		old_temp = temp;
		
		snprintf(link->info, STRING_LENGTH, "%c %2d degC%c ",
				tempperc >= 75? COLOR_WARNING : COLOR_NORMAL,
				temp, COLOR_NORMAL);
		
		SET_FLAG(updated, CPU_TEMP);
		CHECK_LENGTH(CPU_TEMP);
	}

	return 0;
}

static int
get_fan(void)
{
	if (GET_FLAG(err, FAN))
		return -1;
	
	struct string_link *link;
	int rpm, count, fanperc;
	static int old_fanperc;
	
	link = get_string_link(FAN);
	if (!link)
		ERR(FAN, "error getting string link in get_fan()", -1);
	
	if (traverse_list(fan_list, FAN_SPEED_DIR, &rpm, &count) < 0)
		ERR(FAN, "error traversing list in get_fan()", -1);
			
	rpm /= count;
	rpm -= const_fan_min;
	if (rpm <= 0)
		rpm = 0;
	
	fanperc = rint((double)rpm / (double)const_fan_max * 100);
	
	if (old_fanperc != fanperc) {
		old_fanperc = fanperc;
	
		if (fanperc >= 100)
			snprintf(link->info, STRING_LENGTH, "%c MAX %c ", COLOR_WARNING, COLOR_NORMAL);
		else
			snprintf(link->info, STRING_LENGTH, "%c %2d%% %c ",
					fanperc >= 75? COLOR_WARNING : COLOR_NORMAL,
					fanperc, COLOR_NORMAL);
		
		SET_FLAG(updated, FAN);
		CHECK_LENGTH(FAN);
	}

	return 0;
}

static int
get_brightness(void)
{
	if (GET_FLAG(err, BRIGHTNESS))
		return -1;
	
	const char* b_files[2] = { SCREEN_BRIGHTNESS_FILE, KBD_BRIGHTNESS_FILE };
	
	struct string_link *link;
	int scrn, kbd;
	int scrn_perc, kbd_perc = 0;
	static int old_scrn_perc, old_kbd_perc;
	
	link = get_string_link(BRIGHTNESS);
	if (!link)
		ERR(BRIGHTNESS, "error getting string link in get_brightness()", -1);
	
	for (int i = 0; i < 2; i++) {
		if (i == 1 && !DISPLAY_KBD) continue;
		FILE *fd;
		fd = fopen(b_files[i], "r");
		if (!fd)
			ERR(BRIGHTNESS, "error opening a file in get_brightness()", -1);
		fscanf(fd, "%d", i == 0 ? &scrn : &kbd);
		if (fclose(fd))
			ERR(BRIGHTNESS, "error closing a file in get_brightness()", -1);
	}
	
	scrn_perc = rint((double)scrn / (double)const_screen_brightness_max * 100);
	if (DISPLAY_KBD)
		kbd_perc = rint((double)kbd / (double)const_kbd_brightness_max * 100);
	
	if (old_scrn_perc != scrn_perc ||
		old_kbd_perc != kbd_perc) {
		old_scrn_perc = scrn_perc;
		old_kbd_perc = kbd_perc;
		
		if (DISPLAY_KBD)
			snprintf(link->info, STRING_LENGTH, "%c%3d%%, %3d%%%c ",
				COLOR_NORMAL, scrn_perc, kbd_perc, COLOR_NORMAL);
		else
			snprintf(link->info, STRING_LENGTH, "%c%3d%%%c ",
				COLOR_NORMAL, scrn_perc, COLOR_NORMAL);
		
		SET_FLAG(updated, BRIGHTNESS);
		CHECK_LENGTH(BRIGHTNESS);
	}
	
	return 0;
}

static int
get_volume(void)
{
	if (GET_FLAG(err, VOLUME))
		return -1;
	
	struct string_link *link;
	long pvol;
	int swch, volperc;
	static int old_vol;
	
	link = get_string_link(VOLUME);
	if (!link)
		ERR(VOLUME, "error getting string link in get_volume()", -1);
	
	if (snd_mixer_selem_get_playback_switch(snd_elem, SND_MIXER_SCHN_MONO, &swch))
		ERR(VOLUME, "error with snd_mixer_selem_get_playback_switch() in get_volume()", -1);
	if (!swch) {
		if (!old_vol && link->len)
			return 0;
		else {
			old_vol = swch;
			
			snprintf(link->info, STRING_LENGTH, "%cmute%c ",
					COLOR_NORMAL, COLOR_NORMAL);
			
			SET_FLAG(updated, VOLUME);
			CHECK_LENGTH(VOLUME);
		}
	} else {
		if (snd_mixer_selem_get_playback_volume(snd_elem, SND_MIXER_SCHN_MONO, &pvol))
			ERR(VOLUME, "error with snd_mixer_selem_get_playback_volume() in get_volume()", -1);
				
		// round to the nearest ten
		volperc = (double)pvol / const_vol_range * 100;
		volperc = rint((float)volperc / 10) * 10;
		
		if (old_vol != volperc) {
			old_vol = volperc;
		
			snprintf(link->info, STRING_LENGTH, "%c%3d%%%c ",
					COLOR_NORMAL, volperc, COLOR_NORMAL);
				
			SET_FLAG(updated, VOLUME);
			CHECK_LENGTH(VOLUME);
		}
	}
	
	return 0;
}

static int
get_battery(void)
{
	if (GET_FLAG(err, BATTERY))
		return -1;
	
	/* from acpi.c and other acpi source files */
	struct string_link *link;
	FILE *fd;
	char status_string[20];
	int status; // -1 = discharging, 0 = full, 1 = charging
	int capacity;
	static int old_status, old_capacity;
	
	link = get_string_link(BATTERY);
	if (!link)
		ERR(BATTERY, "error getting string link in get_battery()", -1);
	
	fd = fopen(BATT_STATUS_FILE, "r");
	if (!fd)
		ERR(BATTERY, "error opening battery status file in get_battery()", -1);
	fscanf(fd, "%s", &status_string);
	if (fclose(fd))
		ERR(BATTERY, "error closing battery status file in get_battery()", -1);

	if (!strcmp(status_string, "Full") || !strcmp(status_string, "Unknown")) {
		if (!old_status && link->len)
			return 0;
		else {
			old_status = 0;
			
			snprintf(link->info, STRING_LENGTH, "%c full %c ",
					COLOR1, COLOR_NORMAL);
			
			SET_FLAG(updated, BATTERY);
			CHECK_LENGTH(BATTERY);
			
			return 0;
		}
	}
	
	if (!strcmp(status_string, "Discharging"))
		status = -1;
	else if (!strcmp(status_string, "Charging"))
		status = 1;
	else
		ERR(BATTERY, "error parsing battery status file in get_battery()", -1);
		
	fd = fopen(BATT_CAPACITY_FILE, "r");
	if (!fd)
		ERR(BATTERY, "error opening battery capacity file in get_battery()", -1);
	fscanf(fd, "%d", &capacity);
	if (fclose(fd))
		ERR(BATTERY, "error closing battery capacity file in get_battery()", -1);
	
	if (capacity > 99)
		capacity = 99;
	
	if (old_capacity != capacity) {
		old_capacity = capacity;
			
		snprintf(link->info, STRING_LENGTH, " %c %c%2d%% %c ",
				capacity < 20 ? COLOR_ERROR : status > 0 ? COLOR2 : COLOR_WARNING,
				status > 0 ? '+' : '-', capacity, COLOR_NORMAL);
		
		SET_FLAG(updated, BATTERY);
		CHECK_LENGTH(BATTERY);
	}
	
	return 0;
}

static int
parse_account_number_json(char *raw_json)
{
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *results, *account, *account_num;
	
	results = cJSON_GetObjectItem(parsed_json, "results");
	if (!results)
		ERR(PORTFOLIO, "error finding 'results' in parsed_json in parse_account_number_json()", -1);
	account = results->child;
	if (!account)
		ERR(PORTFOLIO, "error getting child in parsed_json in parse_account_number_json()", -1);
	account_num = cJSON_GetObjectItem(account, "account_number");
	if (!account_num)
		ERR(PORTFOLIO, "error finding 'account number' in parsed_json in parse_account_number_json()", -1);
	
	strncpy(account_number, account_num->valuestring, STRING_LENGTH - 1);
	
	cJSON_Delete(parsed_json);
	return 0;
}
	
static int
get_account_number(void)
{
	if (GET_FLAG(err, PORTFOLIO))
		return -1;
	
	struct data_struct account_number_struct;
	
	curl_easy_reset(sb_curl);
	
	account_number_struct.data = malloc(1);
	if (!account_number_struct.data)
		ERR(PORTFOLIO, "error allocating memory for account_number_struct.data in get_account_number()", -1);
	account_number_struct.size = 0;
	
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, token_header);
	if (!headers)
		ERR(PORTFOLIO, "error with curl_slist_append() in get_account_number()", -1);
			
	if (curl_easy_setopt(sb_curl, CURLOPT_URL, "https://api.robinhood.com/accounts/") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEDATA, &account_number_struct) != CURLE_OK)
		ERR(PORTFOLIO, "error with curl_easy_setopt() in get_account_number()", -1);
	if (curl_easy_perform(sb_curl) == CURLE_OK) {
		if (parse_account_number_json(account_number_struct.data) < 0)
			ERR(PORTFOLIO, "error parsing account number json in get_account_number()", -1);
	} else
		ERR(PORTFOLIO, "error with curl_easy_perform() in get_account_number()", -1);
	
	free(account_number_struct.data);
	return 0;
}

static int
parse_token_json(char *raw_json)
{
	cJSON *parsed_json = cJSON_Parse(raw_json);
	cJSON *token = cJSON_GetObjectItem(parsed_json, "token");
	if (!token)
		ERR(PORTFOLIO, "error finding 'token' in json in parse_token_json()", -1);
	
	snprintf(token_header, STRING_LENGTH, "Authorization: Token %s", token->valuestring);
	
	cJSON_Delete(parsed_json);
	return 0;
}
	
static int
get_token(void)
{
	struct data_struct token_struct;
	struct curl_slist *header = NULL;
	
	curl_easy_reset(sb_curl);
	
	token_struct.data = malloc(1);
	if (!token_struct.data)
		ERR(PORTFOLIO, "error allocating memory for token_struct.data in get_token()", -1);
	token_struct.size = 0;
	
	header = curl_slist_append(header, "Accept: application/json");
	if (!header)
		ERR(PORTFOLIO, "error with curl_slist_append() in get_token()", -1);
			
	if (curl_easy_setopt(sb_curl, CURLOPT_URL, "https://api.robinhood.com/api-token-auth/") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_HTTPHEADER, header) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_POSTFIELDS, RH_LOGIN) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0") != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, curl_callback) != CURLE_OK ||
			curl_easy_setopt(sb_curl, CURLOPT_WRITEDATA, &token_struct) != CURLE_OK)
		ERR(PORTFOLIO, "error with curl_easy_setopt() in get_token()", -1);
	if (curl_easy_perform(sb_curl) == CURLE_OK) {
		if (parse_token_json(token_struct.data) < 0)
			ERR(PORTFOLIO, "error with parse_token_json() in get_token()", -1);
	} else
		ERR(PORTFOLIO, "error with curl_easy_perform() in get_token()", -1);
		
	free(token_struct.data);
	curl_slist_free_all(header);
	return 0;
}

static int
init_portfolio()
{
	if (!sb_curl)
		return -1;
	
	if (get_token() < 0)
		ERR(PORTFOLIO, "error getting token in init_portfolio()", -1);
	if (get_account_number() < 0)
		ERR(PORTFOLIO, "error getting account number in init_portfolio()", -1);
	snprintf(portfolio_url, STRING_LENGTH,
			"https://api.robinhood.com/accounts/%s/portfolio/", account_number);
	portfolio_consts_found = true;
	
	return 0;
}

size_t
discard_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	// same as writing to /dev/null
	return size * nmemb;
}

static int
check_conn(void)
{
	curl_easy_reset(sb_curl);
	CURLcode code;
	
	curl_easy_setopt(sb_curl, CURLOPT_URL, "https://www.google.com/");
	curl_easy_setopt(sb_curl, CURLOPT_WRITEFUNCTION, discard_callback);
	code = curl_easy_perform(sb_curl);
	if (code == CURLE_OK)
		internet_connected = true;
	else
		internet_connected = false;
	
	return 0;
}

static int
populate_tm_struct(void)
{
	time_t tval;
	time(&tval);
	tm_struct = localtime(&tval);
	if (!tm_struct)
		ERR(TIME, "error with localtime() in populate_tm_struct()", -1);
	
	return 0;
}

static int
loop(Display *dpy, Window root)
{
	int err = 0;
	
	while (1) {
		err += populate_tm_struct();
		if (!internet_connected)
			check_conn();
		
		// // run every second
		if (GET_FLAG(func, TIME)) get_time();
		if (GET_FLAG(func, NETWORK)) get_network();
		if (GET_FLAG(func, CPU_USAGE)) get_cpu_usage();
		if (GET_FLAG(func, VOLUME)) get_volume();
		if (internet_connected) {
			if (GET_FLAG(func, WEATHER) && need_to_get_weather)
				get_weather();
			if (GET_FLAG(func, PORTFOLIO) && !portfolio_consts_found)
				init_portfolio();
		}
		if (GET_FLAG(func, BACKUP) && backup_occurring)
			get_backup();
		
		// run every five seconds
		if (tm_struct->tm_sec % 5 == 0) {
			if (GET_FLAG(func, LOG)) get_log();
			if (GET_FLAG(func, TODO)) get_TODO();
			if (GET_FLAG(func, BACKUP)) get_backup();
			if (GET_FLAG(func, PORTFOLIO)) get_portfolio();
			if (GET_FLAG(func, WIFI)) get_wifi();
			if (GET_FLAG(func, RAM)) get_RAM();
			if (GET_FLAG(func, LOAD)) get_load();
			if (GET_FLAG(func, CPU_TEMP)) get_cpu_temp();
			if (GET_FLAG(func, FAN)) get_fan();
			if (GET_FLAG(func, BRIGHTNESS)) get_brightness();
			if (GET_FLAG(func, BATTERY)) get_battery();
		}
		
		// run every minute
		if (tm_struct->tm_sec == 0) {
			if (GET_FLAG(func, DISK)) get_disk();
		}
		
		// run every 3 hours
		if ((tm_struct->tm_hour + 1) % 3 == 0 && tm_struct->tm_min == 0 && tm_struct->tm_sec == 0) {
			if (GET_FLAG(func, WEATHER)) get_weather();
		}
		
		err += handle_strings(dpy, root);
		err += sleep(1);
	}
	
	return err;
}

static int
get_vol_range(void)
{
	long min, max;
	
	if (snd_mixer_selem_get_playback_volume_range(snd_elem, &min, &max))
		ERR(VOLUME, "error with snd_mixer_selem_get_playback_volume_range() in get_vol_range()", -1);
		
	return max - min;
}

static int
get_brightness_max(char *brightness_file)
{
	if (!DISPLAY_KBD && !strcmp(brightness_file, KBD_BRIGHTNESS_FILE))
		return 0;
	
	char file_str[STRING_LENGTH];
	char dir_str[STRING_LENGTH];
	DIR *dir;
	struct dirent *file;
	int count = 0;
	FILE *fd;
	int max;
	
	strncpy(file_str, brightness_file, STRING_LENGTH - 1);
	strncpy(dir_str, dirname(file_str), STRING_LENGTH - 1);
	
	if (!(dir = opendir(dir_str)))
		ERR(BRIGHTNESS, "error opening brightness directory in get_brightness_max", -1);
			
	while ((file = readdir(dir)))
		if (!strcmp(file->d_name, "max_brightness")) {
			if (strlen(dir_str) + strlen(file->d_name) + 2 > sizeof dir_str)
				break;
			strncat(dir_str, "/", STRING_LENGTH - strlen(dir_str));
			strncat(dir_str, file->d_name, STRING_LENGTH - strlen(dir_str));
			count++;
			break;
		}
	if (!count)
		ERR(BRIGHTNESS, "error finding brightness directory in get_brightness_max", -1);
	
	fd = fopen(dir_str, "r");
	if (!fd)
		ERR(BRIGHTNESS, "error opening brightness file in get_brightness_max", -1);
	fscanf(fd, "%d", &max);
	
	if (fclose(fd) < 0)
		ERR(BRIGHTNESS, "error closing brightness file in get_brightness_max", -1);
	if (closedir(dir) < 0)
		ERR(BRIGHTNESS, "error closing brightness directory in get_brightness_max", -1);
	return max;
}

static int
free_list(struct file_link *list)
{
	struct file_link *next;
	
	while (list) {
		next = list->next;
		free(list->filename);
		free(list);
		list = next;
	}
	
	return 0;
}

static struct file_link *
add_file_link(struct file_link *list, char *filename)
{
	struct file_link *new;
	int len;
	
	new = malloc(sizeof(struct file_link));
	if (!new)
		ERR(63, "error allocating memory for new link in add_file_link()", NULL);
			
	len = strlen(filename) + 1;
	new->filename = malloc(len);
	if (!new->filename)
		ERR(63, "error allocating memory for filename in new link in add_file_link()", NULL);
	
	strncpy(new->filename, filename, len);
	new->next = NULL;
	
	if (!list)
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
	
	if (!(dir = opendir(path)))
		ERR(63, "error opening directory in populate_list()", NULL);
		
	while ((file = readdir(dir))) {
		if (strstr(file->d_name, base))
			if (strstr(file->d_name, match)) {
				list = add_file_link(list, file->d_name);
				if (!list)
					ERR(63, "error adding link to list in populate_list()", NULL);
				count++;
			}
	}
	if (!count)
		ERR(63, "error finding any files in populate_list()", NULL);
	
	if (closedir(dir) < 0)
		ERR(63, "error closing directory in populate_list()", NULL);
	return list;
}

static int
get_gen_consts(char *path, char *base, char *match)
{
	struct file_link *list = NULL;
	int total, count;
	
	list = populate_list(list, path, base, match);
	if (!list)
		ERR(63, "error populating list in get_gen_consts()", -1);
				
	if (traverse_list(list, path, &total, &count) < 0)
		ERR(63, "error traversing list in get_gen_consts()", -1);
	
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
		ERR(CPU_USAGE, "error opening /proc/cpuinfo in get_cpu_ratio()", -1);
	while (fgets(line, 256, fd) && strncmp(line, "cpu cores", 9));
	if (!line)
		ERR(CPU_USAGE, "error reading /proc/cpuinfo in get_cpu_ratio()", -1);
	if (fclose(fd) < 0)
		ERR(CPU_USAGE, "error closing /proc/cpuinfo in get_cpu_ratio()", -1);
			
	token = strtok(line, ":");
	if (!token)
		ERR(CPU_USAGE, "error parsing /proc/cpuinfo in get_cpu_ratio()", -1);
	token = strtok(NULL, ":");
	if (!token)
		ERR(CPU_USAGE, "error parsing /proc/cpuinfo in get_cpu_ratio()", -1);
	
	cores = atoi(token);
	if ((threads = sysconf(_SC_NPROCESSORS_ONLN)) < 0)
		ERR(CPU_USAGE, "error getting threads in get_cpu_ratio()", -1);
	
	return threads / cores;
}

static int
get_font(char **ptr)
{
	// TODO strncmp() in while() loop should handle commented out lines
	FILE *fd;
	char line[256];
	char *token;
	char *font;
	
	if (!(fd = fopen(DWM_CONFIG_FILE, "r")))
		ERR(BOTTOMBAR, "error opening dwm config file in get_font()", -1);
	while (fgets(line, STRING_LENGTH, fd) && strncmp(line, "static const char font", 22));
	if (!line)
		ERR(BOTTOMBAR, "no font found in dwm config file in get_font()", -1);
	if (fclose(fd) < 0)
		ERR(BOTTOMBAR, "error closing dwm config file in get_font()", -1);
			
	token = strtok(line, "=");
	if (!token)
		ERR(BOTTOMBAR, "error parsing dwm config file in get_font()", -1);
	token = strtok(NULL, "=");
	if (!token)
		ERR(BOTTOMBAR, "error parsing dwm config file in get_font()", -1);
			
	while (*token != '"') token++;
	token++;
	token = strtok(token, "\"");
	if (!token)
		ERR(BOTTOMBAR, "error parsing dwm config file in get_font()", -1);
			
	font = malloc(strlen(token) + 1);
	if (!font)
		ERR(BOTTOMBAR, "error allocating memory for font in get_font()", -1);
	if (!strncpy(font, token, strlen(token)))
		ERR(BOTTOMBAR, "error storing font in get_font()", -1);
	*ptr = font;
	
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
	if (get_font(&fontname) < 0)
		ERR(BOTTOMBAR, "error getting font in get_bar_max_len()", -1);
	fontset = XCreateFontSet(dpy, fontname, &miss_list, &count, &def);
	if (fontset) {
		width_c = XmbTextExtents(fontset, "0", 1, NULL, &rect);
		XFreeFontSet(dpy, fontset);
	} else {
		if (!(xfont = XLoadQueryFont(dpy, fontname))
				&& !(xfont = XLoadQueryFont(dpy, "fixed")))
			ERR(BOTTOMBAR, "error loading font for bar in get_bar_max_len()", -1);
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
		ERR(DISK, "error getting root file stats in get_block_size()", -1);
			
	return root_fs.fs_stat.f_bsize;
}

static int
get_dev_id(void)
{
	int index = if_nametoindex(WIFI_INTERFACE);
	if (!index)
		ERR(WIFI, "error finding index value for wireless interface in get_dev_id()", -1);
	return index;
}

static int
get_consts(Display *dpy)
{
	int err = 0;
	
	if ((const_devidx = get_dev_id()) == 0) {
		err += -1;
		SIMPLE_ERR(WIFI, "error getting device id in get_consts()");
	}
	if ((const_block_size = get_block_size()) < 0 ) {
		err += -1;
		SIMPLE_ERR(DISK, "error getting block size in get_consts()");
	}
	if ((const_bar_max_len = get_bar_max_len(dpy)) < 0 ) {
		err += -1;
		SIMPLE_ERR(BOTTOMBAR, "error calculating maximum bar length in get_consts()");
	}
	if ((const_cpu_ratio = get_cpu_ratio()) < 0 ) {
		err += -1;
		SIMPLE_ERR(CPU_USAGE, "error calculating CPU ratio in get_consts()");
	}
	if ((const_temp_max = get_gen_consts(CPU_TEMP_DIR, "temp", "max")) < 0 ) {
		err += -1;
		SIMPLE_ERR(CPU_TEMP, "error getting maximum temp in get_consts()");
	}
	if ((const_fan_min = get_gen_consts(FAN_SPEED_DIR, "fan", "min")) < 0 ) {
		err += -1;
		SIMPLE_ERR(FAN, "error getting minimum fan speed in get_consts()");
	}
	if ((const_fan_max = get_gen_consts(FAN_SPEED_DIR, "fan", "max")) < 0 ) {
		err += -1;
		SIMPLE_ERR(FAN, "error getting maximum fan speed in get_consts()");
	}
	if ((const_screen_brightness_max = get_brightness_max(SCREEN_BRIGHTNESS_FILE)) < 0 ) {
		err += -1;
		SIMPLE_ERR(BRIGHTNESS, "error getting maximum screen brightness in get_consts()");
	}
	if ((const_kbd_brightness_max = get_brightness_max(KBD_BRIGHTNESS_FILE)) < 0 ) {
		err += -1;
		SIMPLE_ERR(BRIGHTNESS, "error getting maximum keyboard brightness in get_consts()");
	}
	if ((const_vol_range = get_vol_range()) < 0 ) {
		err += -1;
		SIMPLE_ERR(VOLUME, "error getting volume range in get_consts()");
	}
			
	return err;
}

static int
populate_lists(void)
{
	int err = 0;
	if (!(therm_list = populate_list(therm_list, CPU_TEMP_DIR, "temp", "input"))) {
		err = -1;
		ERR(CPU_TEMP, "error creating list of CPU temperature sensors in populate_lists()", -1);
	}
	if (!(fan_list = populate_list(fan_list, FAN_SPEED_DIR, "fan", "input"))) {
		err += -1;
		ERR(FAN, "error creating list of fan speed sensors in populate_lists()", -1);
	}
	
	return err;
}

static int
make_vol_singleton(void)
{
	// stolen from amixer utility from alsa-utils
	if (snd_mixer_open(&handle, 0))
		ERR(VOLUME, "error with snd_mixer_open() in make_vol_singleton()", -1);
	if (snd_mixer_attach(handle, "default"))
		ERR(VOLUME, "error with snd_mixer_attach() in make_vol_singleton()", -1);
	if (snd_mixer_selem_register(handle, NULL, NULL))
		ERR(VOLUME, "error with snd_mixer_selem_register() in make_vol_singleton()", -1);
	if (snd_mixer_load(handle))
		ERR(VOLUME, "error with snd_mixer_load() in make_vol_singleton()", -1);
	
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_name(sid, "Master");
	
	if (!(snd_elem = snd_mixer_find_selem(handle, sid)))
		ERR(VOLUME, "error with snd_mixer_find_selem() in make_vol_singleton()", -1);
			
	// if (snd_mixer_close(handle))
		// ERR(VOLUME, "error closing handle in make_vol_singleton()", -1);
	snd_config_update_free_global();
	
	return 0;
}

static int
make_wifi_singleton(void)
{
	sb_socket = nl_socket_alloc();
	if (!sb_socket)
		ERR(WIFI, "error with nl_socket_alloc() in make_wifi_singleton()", -1);
	if (genl_connect(sb_socket) < 0)
		ERR(WIFI, "error with genl_connect() in make_wifi_singleton()", -1);
			
	sb_id = genl_ctrl_resolve(sb_socket, "nl80211");
	if (!sb_id)
		ERR(WIFI, "error with genl_ctrl_resolve() in make_wifi_singleton()", -1);
	
	sb_msg = nlmsg_alloc();
	if (!sb_msg)
		ERR(WIFI, "error with nlmsg_alloc() in make_wifi_singleton()", -1);
			
	sb_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!sb_cb)
		ERR(WIFI, "error with nl_cb_alloc() in make_wifi_singleton()", -1);
			
	if (rtnl_open(&sb_rth, 0) < 0)
		ERR(WIFI, "error with rtnl_open() in make_wifi_singleton()", -1);
			
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
		ERR(PORTFOLIO, "error making curl singleton in make_curl_singleton()", -1);
	if (!(sb_curl = curl_easy_init()))
		ERR(PORTFOLIO, "error making curl singleton in make_curl_singleton()", -1);
			
	// curl_easy_cleanup(sb_curl);
	curl_global_cleanup();
	return 0;
}

static int
make_singletons(void)
{
	int err = 0;
	
	if ((err += make_curl_singleton()) < 0) {
		SET_FLAG(err, WEATHER);
		ERR(PORTFOLIO, "error making curl singleton in make_singletons()", -1);
	}
	if ((err += make_wifi_singleton()) < 0) {
		ERR(WIFI, "error making wifi singleton in make_singletons()", -1);
	}
	if ((err += make_vol_singleton()) < 0) {
		ERR(VOLUME, "error making volume singleton in make_singletons()", -1);
	}
	
	return err;
}

static struct string_link *
make_new_link(int id, const char *heading, int bar_id, int *bar_len)
{
	struct string_link *new;
	int heading_len;
	
	new = malloc(sizeof(struct string_link));
	if (!new)
		ERR(id, "error allocating memory for new string_link in make_new_link()", NULL);
	heading_len = strlen(heading) + 5;
	new->heading = malloc(heading_len);
	if (!new->heading)
		ERR(id, "error allocating memory for string_link heading in make_new_link()", NULL);
	new->info = calloc(1, STRING_LENGTH);
	if (!new->info)
		ERR(id, "error allocating memory for string_link info in make_new_link()", NULL);
			
	new->id = id;
	new->bar_id = bar_id;
	snprintf(new->heading, heading_len, " %c %s:", COLOR_HEADING, heading);
	new->len = 0;
	new->bar_len = bar_len;
	new->next = NULL;
	
	return new;
}

static int
get_string_link_id(const char *heading)
{
	int id = 0, cmp;
	
	cmp = strncmp(heading, "Z", 1);
	if (cmp < 1) {
		if (cmp == -23) {
			if (!strcmp(heading, "CPU temp")) {
				id = CPU_TEMP;
			} else if (!strcmp(heading, "CPU usage")) {
				id = CPU_USAGE;
			}
		} else if (!strcmp(heading, "RAM")) {
			id = RAM;
		} else if (!strcmp(heading, "TODO")) {
			id = TODO;
		}
	} else {
		if (cmp < 19) {
			if (cmp == 8) {
				if (!strcmp(heading, "backup")) {
					id = BACKUP;
				} else if (!strcmp(heading, "battery")) {
					id = BATTERY;
				} else if (!strcmp(heading, "brightness")) {
					id = BRIGHTNESS;
				}
			} else if (cmp == 18) {
				if (!strcmp(heading, "load")) {
					id = LOAD;
				} else if (!strcmp(heading, "log")) {
					id = LOG;
				}
			} else if (!strcmp(heading, "disk")) {
				id = DISK;
			} else if (!strcmp(heading, "fan")) {
				id = FAN;
			}
		} else {
			if (cmp < 27) {
				if (!strcmp(heading, "network")) {
					id = NETWORK;
				} else if (!strcmp(heading, "portfolio")) {
					id = PORTFOLIO;
				} else if (!strcmp(heading, "time")) {
					id = TIME;
				}
			} else {
				if (!strcmp(heading, "volume")) {
					id = VOLUME;
				} else if (!strcmp(heading, "weather")) {
					id = WEATHER;
				} else if (!strcmp(heading, "wifi")) {
					id = WIFI;
				}
			}
		}
	}
		
	if (!id)
		ERR(63, "error matching strings in get_string_link_id()", -1);
			
	return id;
}

static struct string_link *
add_string_link(struct string_link *head, const char *heading, int bar_id, int *bar_len)
{
	int id;
	struct string_link *new, **snake;
	
	id = get_string_link_id(heading);
	if (id < 0)
		ERR(63, "error getting id in add_string_link()", NULL);
			
	new = make_new_link(id, heading, bar_id, bar_len);
	if (!new)
		ERR(id, "error making new link in add_string_link()", NULL);
	
	// add at end to keep order
	snake = &head;
	while (*snake)
		snake = &(*snake)->next;
		
	*snake = new;
	
	SET_FLAG(func, id);
	return head;
}

static int
populate_string_list(void)
{
	int i;
	
	for (i = 0; i < sizeof top_bar / sizeof *top_bar; i++) {
		string_list = add_string_link(string_list, top_bar[i], TOPBAR, &top_length);
		if (!string_list)
			ERR(63, "error adding link to string_list\n\tplease check config.h and restart", -1);
	}
	for (struct string_link *link = string_list; link; link = link->next)
		separator = link->id;
	for (i = 0; i < sizeof bottom_bar / sizeof *bottom_bar; i++) {
		string_list = add_string_link(string_list, bottom_bar[i], BOTTOMBAR, &bottom_length);
		if (!string_list)
			ERR(63, "error adding link to string_list\n\tplease check config.h and restart", -1);
	}
	
	return 0;
}

static int
init(Display *dpy, Window root)
{
	int err = 0;
	struct string_link *link;
	
	snprintf(error_string, 10, "%c Error%c ", COLOR_ERROR, COLOR_NORMAL);
	populate_tm_struct();
	if (populate_string_list() < 0)
		exit(1);
	err += make_singletons();
	err += check_conn();
	err += populate_lists();
	err += get_consts(dpy);
	err += init_portfolio();
	
	if (GET_FLAG(func, LOG)) get_log();
	if (GET_FLAG(func, TODO)) get_TODO();
	if (GET_FLAG(func, WEATHER)) get_weather();
	if (GET_FLAG(func, BACKUP)) get_backup();
	if (GET_FLAG(func, PORTFOLIO)) get_portfolio();
	if (GET_FLAG(func, WIFI)) get_wifi();
	if (GET_FLAG(func, TIME)) get_time();
	
	if (GET_FLAG(func, NETWORK)) get_network();
	if (GET_FLAG(func, DISK)) get_disk();
	if (GET_FLAG(func, RAM)) get_RAM();
	if (GET_FLAG(func, LOAD)) get_load();
	if (GET_FLAG(func, CPU_USAGE)) get_cpu_usage();
	if (GET_FLAG(func, CPU_TEMP)) get_cpu_temp();
	if (GET_FLAG(func, FAN)) get_fan();
	
	if (GET_FLAG(func, BRIGHTNESS)) get_brightness();
	if (GET_FLAG(func, VOLUME)) get_volume();
	if (GET_FLAG(func, BATTERY)) get_battery();
	
	link = string_list;
	top_length = 0;
	bottom_length = 0;
	while (link) {
		if (link->id == TODO)
			link = link->next;
		link->len = get_length(link);
		*link->bar_len += link->len;
		link = link->next;
	}
	
	memset(statusbar, '\0', TOTAL_LENGTH);
	
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
	
	if (init(dpy, root) < 0)
		SIMPLE_ERR(63, "error with initialization");
	if (loop(dpy, root) < 0)
		ERR(63, "loop broken", 1);
	
	strncpy(statusbar, "Loop broken. Please check log for details.", TOTAL_LENGTH);
	XStoreName(dpy, root, statusbar);
	XFlush(dpy);
	
	return 0;
}
