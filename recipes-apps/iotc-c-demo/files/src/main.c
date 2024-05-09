/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */


/* TODO 
    finish parse_device_json()
    Store attributes in 2d array like the scripts

    hook up jsons for connection
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "iotcl.h"
#include "iotconnect.h"

// #include "app_config.h"
#include "cJSON.h"

#include <time.h>
#include <sys/stat.h>

#include <dirent.h>

// windows compatibility
#if defined(_WIN32) || defined(_WIN64)
#define F_OK 0
#include <Windows.h>
#include <io.h>
#define sleep Sleep
#define access _access_s
#else
#include <unistd.h>
#endif

#define APP_VERSION "00.01.00"
#define STRINGS_ARE_EQUAL 0
#define FREE(x)     \
    if ((x))        \
    {               \
        free(x);    \
        (x) = NULL; \
    }
#define REPEAT_SENT_TELEMETRY

static volatile sig_atomic_t keep_running = 1;

static void sig_handler(int _)
{
    (void)_;
    keep_running = 0;
}

char *json_path = NULL;
char device_id[256] = {};
char company_id[256] = {};
char environment[256] = {};
char iotc_server_cert_path[4096] = {};
char sdk_id[256] = {};

char connection_type_str[256] = {};
IotConnectConnectionType connection_type = 0;

char auth_type[256] = {};
char commands_list_path[4096] = {};

char **available_scripts = NULL;
int available_scripts_count = 0;

char *ver = NULL;
char *pf = NULL;
char *cpid = NULL;
char *env = NULL;
char *uid = NULL;
char *did = NULL;
int at = 0;
char *disc = NULL;
char *iotc_root_ca_path_az = NULL;
char *iotc_root_ca_path_aws = NULL;
char *iotc_x509_client_key_path = NULL;
char *iotc_x509_client_cert_path = NULL;
char *iotc_commands_path = NULL;
char *iotc_telemetry_path = NULL;
int authType = 0;
int dataFrequency = 0;

static void free_local_data()
{
    free(ver);
    free(pf);
    free(cpid);
    free(env);
    free(uid);
    free(did);
    free(disc);
    free(iotc_root_ca_path_az);
    free(iotc_root_ca_path_aws);
    free(iotc_x509_client_key_path);
    free(iotc_x509_client_cert_path);
    free(iotc_commands_path);
    free(iotc_telemetry_path);
}

static void cleanup_string(char** ptr)
{
    printf("calling %s\n", __func__);
    free(*ptr);
}

static void cleanup_FD(FILE** ptr)
{
    printf("calling %s\n", __func__);
    fclose(*ptr);
}

static void cleanup_cJSON(cJSON** ptr)
{
    printf("calling %s\n", __func__);
    cJSON_Delete(*ptr);
}

static void cleanup_DIR(DIR** ptr)
{
    printf("calling %s\n", __func__);
    closedir(*ptr);
}

typedef struct telemetry_attribute
{
    char *name;
    int name_len;

    char *path;
    int path_len;

    bool read_ascii;
    time_t last_accessed;

} telemetry_attribute_t;

static void on_connection_status(IotConnectMqttStatus status)
{
    // Add your own status handling
    switch (status)
    {
    case IOTC_CS_MQTT_CONNECTED:
        printf("IoTConnect Client Connected notification.\n");
        break;
    case IOTC_CS_MQTT_DISCONNECTED:
        printf("IoTConnect Client Disconnected notification.\n");
        break;
    case IOTC_CS_MQTT_DELIVERED:
        printf("IoTConnect Client message delivered.\n");
        break;
    case IOTC_CS_MQTT_SEND_FAILED:
        printf("IoTConnect Client message send failed!\n");
        break;
    default:
        printf("IoTConnect Client ERROR notification\n");
        break;
    }
}

static void on_command(IotclC2dEventData data)
{
    const char *command = iotcl_c2d_get_command(data);
    const char *ack_id = iotcl_c2d_get_ack_id(data);

    if (command == NULL)
    {
        printf("Failed to parse command\n");
        if (ack_id)
        {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Internal error");
        }
        return;
    }

    printf("Command %s received with %s ACK ID\n", command, ack_id ? ack_id : "no");

    int delim_pos = strlen(command);
    for (int i = 0; i < (int)strlen(command); i++)
    {
        if (command[i] == ' ')
        {
            delim_pos = i;
            break;
        }
    }

    bool command_exists = false;
    for (int i = 0; i < available_scripts_count; i++)
    {
        if (strncmp(available_scripts[i], command, delim_pos) == STRINGS_ARE_EQUAL)
        {
            command_exists = true;
            break;
        }
    }

    if (!command_exists)
    {
        if (ack_id)
        {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Command does not exist locally, Skipping");
        }
        printf("Command does not exist locally, Skipping\n");
        // free((char*)command);
        return;
    }

    bool need_forward_slash = (commands_list_path[strlen(commands_list_path) - 1] != '/');
    int total_command_length = strlen(commands_list_path) + strlen(command) + (int)need_forward_slash;
    char *final_command_path = calloc(total_command_length, sizeof(command[0]));

    strcpy(final_command_path, commands_list_path);
    if (need_forward_slash)
    {
        final_command_path[strlen(commands_list_path)] = '/';
    }
    strcpy(final_command_path + strlen(final_command_path), command);
    // free((char*)command);

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // Execute script
    FILE *fp = (FILE *)popen(final_command_path, "r");
    free(final_command_path);

    if (!fp)
    {
        if (ack_id)
        {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Failed to execute commnand, Skipping");
        }
        printf("Failed to execute commnand, Skipping\n");
        return;
    }

    // Read stdout
    while ((read = getline(&line, &len, fp)) != -1)
    {
    }

    // if we have not read the entire file then something is wrong
    if (!feof(fp))
    {
        if (ack_id)
        {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Failed to read stdout commnand, Skipping");
        }
        printf("Failed to execute commnand, Skipping\n");
        free(line);
        pclose(fp);
        return;
    }

    // Close the stdout stream and get the return code
    int return_code = pclose(fp);

    if (ack_id)
    {
        iotcl_mqtt_send_cmd_ack(ack_id, (return_code == 0) ? IOTCL_C2D_EVT_CMD_SUCCESS : IOTCL_C2D_EVT_CMD_FAILED, line);
    }

    printf("Script exited with status %d\n", return_code);
    free(line);
}

static bool is_app_version_same_as_ota(const char *version)
{
    return strcmp(APP_VERSION, version) == 0;
}

static bool app_needs_ota_update(const char *version)
{
    return strcmp(APP_VERSION, version) < 0;
}

// This sample OTA handling only checks the version and verifies if the firmware needs an update but does not download.
static void on_ota(IotclC2dEventData data)
{
    const char *message = NULL;
    const char *url = iotcl_c2d_get_ota_url(data, 0);
    const char *ack_id = iotcl_c2d_get_ack_id(data);
    bool success = false;
    if (NULL != url)
    {
        printf("Download URL is: %s\n", url);
        const char *version = iotcl_c2d_get_ota_sw_version(data);
        if (is_app_version_same_as_ota(version))
        {
            printf("OTA request for same version %s. Sending success\n", version);
            success = true;
            message = "Version is matching";
        }
        else if (app_needs_ota_update(version))
        {
            printf("OTA update is required for version %s.\n", version);
            success = false;
            message = "Not implemented";
        }
        else
        {
            printf("Device firmware version %s is newer than OTA version %s. Sending failure\n", APP_VERSION,
                   version);
            // Not sure what to do here. The app version is better than OTA version.
            // Probably a development version, so return failure?
            // The user should decide here.
            success = false;
            message = "Device firmware version is newer";
        }
    }

    iotcl_mqtt_send_ota_ack(ack_id, (success ? IOTCL_C2D_EVT_OTA_SUCCESS : IOTCL_C2D_EVT_OTA_DOWNLOAD_FAILED), message);
}

static void publish_telemetry(int number_of_attributes, telemetry_attribute_t *telemetry)
{
    IotclMessageHandle msg = iotcl_telemetry_create();

    for (int i = 0; i < number_of_attributes; i++)
    {
        if (access(telemetry[i].path, F_OK) != 0)
        {
            printf("failed to access input telemetry path - %s ; Skipping\n", telemetry[i].path);
            continue;
        }

#ifndef REPEAT_SENT_TELEMETRY
        struct stat file_stat;
        if (stat(telemetry[i].path, &file_stat) == -1)
        {
            printf("failed to access input telemetry stat - %s ; Skipping\n", telemetry[i].path);
            continue;
        }

        time_t modified_time = file_stat.st_mtime;
        if (modified_time <= telemetry[i].last_accessed)
        {
            printf("telemetry not updated since last send - %s ; Skipping\n", telemetry[i].path);
            continue;
        }
        telemetry[i].last_accessed = modified_time;
#endif

        FILE *fp = fopen(telemetry[i].path, "r");
        char *buffer = NULL;
        size_t len = 0;

        ssize_t read = 0;
        read = getline(&buffer, &len, fp);
        if (read != -1)
        {
            iotcl_telemetry_set_string(msg, telemetry[i].name, buffer);
        }

        free(buffer);
        fclose(fp);
    }

    iotcl_mqtt_send_telemetry(msg, false);
    iotcl_telemetry_destroy(msg);
}

static bool string_ends_with(const char *needle, const char *haystack)
{
    const char *str_end = haystack + strlen(haystack) - strlen(needle);
    return (strncmp(str_end, needle, strlen(needle)) == 0);
}

static int parse_raw_json_to_string(char *output, const char *const raw_json_str, char *key)
{
    const cJSON *value = NULL;
    cJSON __attribute__((__cleanup__(cleanup_cJSON))) *json = cJSON_Parse(raw_json_str);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return EXIT_FAILURE;
    }

    value = cJSON_GetObjectItemCaseSensitive(json, key);
    if (cJSON_IsString(value) && (value->valuestring != NULL))
    {
        strncpy(output, value->valuestring, strlen(value->valuestring));

        return EXIT_SUCCESS;
    }

    printf("failed to get \"%s\" from json\n", key);
    return EXIT_FAILURE;
}

static int parse_raw_json_to_int(int *output, const char *const raw_json_str, char *key)
{
    const cJSON *value = NULL;
    cJSON __attribute__((__cleanup__(cleanup_cJSON))) *json = cJSON_Parse(raw_json_str);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return EXIT_FAILURE;
    }

    value = cJSON_GetObjectItemCaseSensitive(json, key);
    if (cJSON_IsNumber(value))
    {
        *output = value->valueint;

        return EXIT_SUCCESS;
    }

    printf("failed to get \"%s\" from json\n", key);
    return EXIT_FAILURE;
}

static int parse_raw_json_to_alloc_string(char **output, const char *const raw_json_str, char *key)
{
    const cJSON *value = NULL;
    cJSON __attribute__((__cleanup__(cleanup_cJSON))) *json = cJSON_Parse(raw_json_str);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return EXIT_FAILURE;
    }

    value = cJSON_GetObjectItemCaseSensitive(json, key);
    if (cJSON_IsString(value) && (value->valuestring != NULL))
    {
        *output = calloc(strlen(value->valuestring) + 1, sizeof(char));
        if (!*output)
        {
            printf("failed to get alloc string\n");
            return EXIT_FAILURE;
        }

        strncpy(*output, value->valuestring, strlen(value->valuestring));

        return EXIT_SUCCESS;
    }

    printf("failed to get \"%s\" from json\n", key);
    return EXIT_FAILURE;
}

static int is_key_in_json(const char *const raw_json_str, char *key)
{
    const cJSON *value = NULL;
    cJSON __attribute__((__cleanup__(cleanup_cJSON))) *json = cJSON_Parse(raw_json_str);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return EXIT_FAILURE;
    }

    value = cJSON_GetObjectItemCaseSensitive(json, key);
    if (value)
    {
        return EXIT_SUCCESS;
    }
    if (cJSON_IsString(value) && (value->valuestring != NULL))
    {
        // strncpy(output,value->valuestring, strlen(value->valuestring));
        return EXIT_SUCCESS;
    }

    printf("failed to get \"%s\" from json\n", key);
    return EXIT_FAILURE;
}

static int parse_json_to_string(char *output, cJSON *json, char *key)
{
    const cJSON *value = NULL;
    value = cJSON_GetObjectItemCaseSensitive(json, key);
    if (cJSON_IsString(value) && (value->valuestring != NULL))
    {
        strncpy(output, value->valuestring, strlen(value->valuestring));

        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

static int init_scripts()
{

    if (access(commands_list_path, F_OK) != 0)
    {
        printf("failed to access scripts path - %s ; Aborting\n", commands_list_path);
        return EXIT_FAILURE;
    }

    DIR __attribute__((__cleanup__(cleanup_DIR))) *dir = NULL;
    struct dirent *entry;
    if ((dir = opendir(commands_list_path)) == NULL)
    {
        perror("opendir() error");
    }

    // Get the total scripts count
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == STRINGS_ARE_EQUAL || strcmp(entry->d_name, "..") == STRINGS_ARE_EQUAL)
        {
            continue;
        }
        available_scripts_count++;
    }

    // Re-read the dir to reset to seek back to the start
    if ((dir = opendir(commands_list_path)) == NULL)
    {
        perror("opendir() error");
    }

    available_scripts = calloc(available_scripts_count, sizeof(char *));

    int itr = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == STRINGS_ARE_EQUAL || strcmp(entry->d_name, "..") == STRINGS_ARE_EQUAL)
        {
            continue;
        }
        available_scripts[itr] = calloc(strlen(entry->d_name), sizeof(char));
        strncpy(available_scripts[itr], entry->d_name, strlen(entry->d_name));
        itr++;
    }

    return EXIT_SUCCESS;
}

static bool is_json_and_accessible(char *path)
{
    if (!string_ends_with(".json", path))
    {
        printf("File extension is not .json of filename %s\n", path);
        return false;
    }

    if (access(path, F_OK) != 0)
    {
        printf("failed to access input json file - %s ; Aborting\n", path);
        return false;
    }

    return true;
}

static char *open_json_and_return_allocated_string(const char *path)
{
    FILE __attribute__((__cleanup__(cleanup_FD))) *fd = fopen(path, "r");
    if (!fd)
    {
        printf("File failed to open - %s", path);
        return NULL;
    }
    fseek(fd, 0l, SEEK_END);
    long file_len = ftell(fd);

    if (file_len <= 0)
    {
        printf("failed calculating file length: %ld. Aborting\n", file_len);
        fclose(fd);
        return NULL;
    }
    rewind(fd);

    char *json_str = (char *)calloc(file_len + 1, sizeof(char));
    if (!json_str)
    {
        printf("failed to calloc. Aborting\n");
        fclose(fd);
        json_str = NULL;
        return NULL;
    }

    for (int i = 0; i < file_len; i++)
    {
        json_str[i] = fgetc(fd);
    }
    // fclose(fd);

    return json_str;
}

static bool is_credentials_json_valid(const char *json_path)
{
    printf("checking %s\n", json_path);

    char __attribute__((__cleanup__(cleanup_string))) *json_str = open_json_and_return_allocated_string(json_path);
    int json_valid = 0;
    json_valid += is_key_in_json(json_str, "ver");
    json_valid += is_key_in_json(json_str, "pf");
    json_valid += is_key_in_json(json_str, "cpid");
    json_valid += is_key_in_json(json_str, "env");
    json_valid += is_key_in_json(json_str, "uid");
    json_valid += is_key_in_json(json_str, "did");
    json_valid += is_key_in_json(json_str, "at");
    json_valid += is_key_in_json(json_str, "disc");

    if (json_valid != 0)
    {
        printf("json is missing required attribute(s); Aborting\n");
        return false;
    }

    return true;
}

static bool parse_credentials_json(const char *json_path)
{
    printf("parsing %s\n", json_path);

    char __attribute__((__cleanup__(cleanup_string))) *json_str = open_json_and_return_allocated_string(json_path);
    int json_valid = 0;

    json_valid += parse_raw_json_to_alloc_string(&ver, json_str, "ver");
    json_valid += parse_raw_json_to_alloc_string(&pf, json_str, "pf");
    json_valid += parse_raw_json_to_alloc_string(&cpid, json_str, "cpid");
    json_valid += parse_raw_json_to_alloc_string(&env, json_str, "env");
    json_valid += parse_raw_json_to_alloc_string(&uid, json_str, "uid");
    json_valid += parse_raw_json_to_alloc_string(&did, json_str, "did");
    json_valid += parse_raw_json_to_int(&at, json_str, "at");
    json_valid += parse_raw_json_to_alloc_string(&disc, json_str, "disc");


    if (json_valid != 0)
    {
        printf("json is missing required attribute(s); Aborting\n");
        return false;
    }

    return true;
}

static bool is_iotc_paths_json_valid(const char *json_path)
{
    printf("checking %s\n", json_path);

    char __attribute__((__cleanup__(cleanup_string))) *json_str = open_json_and_return_allocated_string(json_path);
    int json_valid = 0;
    json_valid += is_key_in_json(json_str, "iotc_root_ca_path_az");
    json_valid += is_key_in_json(json_str, "iotc_root_ca_path_aws");
    json_valid += is_key_in_json(json_str, "iotc_x509_client_key_path");
    json_valid += is_key_in_json(json_str, "iotc_x509_client_cert_path");
    json_valid += is_key_in_json(json_str, "iotc_commands_path");
    json_valid += is_key_in_json(json_str, "iotc_telemetry_path");


    if (json_valid != 0)
    {
        printf("json is missing required attribute(s); Aborting\n");
        return false;
    }

    return true;
}

static bool is_device_json_valid(const char *json_path)
{
    printf("checking %s\n", json_path);

    char __attribute__((__cleanup__(cleanup_string))) *json_str = open_json_and_return_allocated_string(json_path);
    int json_valid = 0;
    json_valid += is_key_in_json(json_str, "authType");
    json_valid += is_key_in_json(json_str, "attributes");
    json_valid += is_key_in_json(json_str, "commands");
    json_valid += is_key_in_json(json_str, "properties");

    if (json_valid != 0)
    {
        printf("json is missing required attribute(s); Aborting\n");
        return false;
    }

    return true;
}

static bool parse_iotc_paths_json(const char *json_path)
{
    printf("parsing %s\n", json_path);

    char __attribute__((__cleanup__(cleanup_string))) *json_str = open_json_and_return_allocated_string(json_path);
    int json_valid = 0;

    json_valid += parse_raw_json_to_alloc_string(&iotc_root_ca_path_az, json_str, "iotc_root_ca_path_az");
    json_valid += parse_raw_json_to_alloc_string(&iotc_root_ca_path_aws, json_str, "iotc_root_ca_path_aws");
    json_valid += parse_raw_json_to_alloc_string(&iotc_x509_client_key_path, json_str, "iotc_x509_client_key_path");
    json_valid += parse_raw_json_to_alloc_string(&iotc_x509_client_cert_path, json_str, "iotc_x509_client_cert_path");
    json_valid += parse_raw_json_to_alloc_string(&iotc_commands_path, json_str, "iotc_commands_path");
    json_valid += parse_raw_json_to_alloc_string(&iotc_telemetry_path, json_str, "iotc_telemetry_path");

    if (json_valid != 0)
    {
        printf("json is missing required attribute(s); Aborting\n");
        return false;
    }

    char* arr[] = {iotc_root_ca_path_az, iotc_root_ca_path_aws, iotc_x509_client_key_path, iotc_x509_client_cert_path,iotc_commands_path,iotc_telemetry_path};
    int arr_size = sizeof(arr) / sizeof(arr[0]);

    for (int i = 0; i < arr_size; i++)
    {
        if (access(arr[i], F_OK) != 0)
        {
            printf("failed to access input json file - %s ; Aborting\n", arr[i]);
            return false;
        }
    }

    return true;
}

static bool parse_device_json(const char *json_path)
{
    printf("parsing %s\n", json_path);

    char __attribute__((__cleanup__(cleanup_string))) *json_str = open_json_and_return_allocated_string(json_path);
    int json_valid = 0;

    json_valid += parse_raw_json_to_int(&authType, json_str, "authType");
    
    // TODO FINISH THIS
    
    json_valid += parse_raw_json_to_alloc_string(&iotc_root_ca_path_aws, json_str, "iotc_root_ca_path_aws");
    json_valid += parse_raw_json_to_alloc_string(&iotc_x509_client_key_path, json_str, "iotc_x509_client_key_path");
    json_valid += parse_raw_json_to_alloc_string(&iotc_x509_client_cert_path, json_str, "iotc_x509_client_cert_path");
    json_valid += parse_raw_json_to_alloc_string(&iotc_commands_path, json_str, "iotc_commands_path");
    json_valid += parse_raw_json_to_alloc_string(&iotc_telemetry_path, json_str, "iotc_telemetry_path");

    if (json_valid != 0)
    {
        printf("json is missing required attribute(s); Aborting\n");
        return false;
    }

    char* arr[] = {iotc_root_ca_path_az, iotc_root_ca_path_aws, iotc_x509_client_key_path, iotc_x509_client_cert_path,iotc_commands_path,iotc_telemetry_path};
    int arr_size = sizeof(arr) / sizeof(arr[0]);

    for (int i = 0; i < arr_size; i++)
    {
        if (access(arr[i], F_OK) != 0)
        {
            printf("failed to access input json file - %s ; Aborting\n", arr[i]);
            return false;
        }
    }

    return true;
}


int main(int argc, char *argv[])
{
    signal(SIGINT, sig_handler);

    IotConnectClientConfig config;
    iotconnect_sdk_init_config(&config);

    telemetry_attribute_t *telemetry = NULL;

    char *iotc_paths_json_path = "/home/akarnil/work/meta-iotconnect-demos/recipes-apps/iotc-c-demo/files/src/iotc_paths.json";
    char *credentials_json_path = "/home/akarnil/work/meta-iotconnect-demos/recipes-apps/iotc-c-demo/files/src/credentials.json";
    char *device_json_path = "/home/akarnil/work/meta-iotconnect-demos/recipes-apps/iotc-c-demo/files/src/device.json";

    if (!is_json_and_accessible(iotc_paths_json_path))
    {
        return EXIT_FAILURE;
    }

    if (!is_json_and_accessible(credentials_json_path))
    {
        return EXIT_FAILURE;
    }

    if (!is_json_and_accessible(device_json_path))
    {
        return EXIT_FAILURE;
    }

    if (!is_credentials_json_valid(credentials_json_path))
    {
        printf("credentials.json is missing required attribute(s); Aborting\n");
        return EXIT_FAILURE;
    }

    if (!is_iotc_paths_json_valid(iotc_paths_json_path))
    {
        printf("iotc_paths.json is missing required attribute(s); Aborting\n");
        return EXIT_FAILURE;
    }

    if (!is_device_json_valid(device_json_path))
    {
        printf("device.json is missing required attribute(s); Aborting\n");
        return EXIT_FAILURE;
    }

    if (!parse_credentials_json(credentials_json_path))
    {
        printf("credentials.json failed to parse; Aborting\n");
        return EXIT_FAILURE;
    }

    if (!parse_iotc_paths_json(iotc_paths_json_path))
    {
        printf("iotc_paths.json failed to parse; Aborting\n");
        return EXIT_FAILURE;
    }

    if (!parse_device_json(device_json_path))
    {
        printf("device.json failed to parse; Aborting\n");
        return EXIT_FAILURE;
    }

    // if (argc != 2)
    // {
    //     printf("json file not provided; Aborting\n");
    //     return EXIT_FAILURE;
    // }

    // if (!string_ends_with(".json", argv[1]))
    // {
    //     printf("File extension is not .json of filename %s\n", argv[1]);
    //     return EXIT_FAILURE;
    // }
    // json_path = argv[1];

    // if (access(json_path, F_OK) != 0)
    // {
    //     printf("failed to access input json file - %s ; Aborting\n", json_path);
    //     return EXIT_FAILURE;
    // }

    // FILE* fd = fopen(json_path, "r");
    // if (!fd)
    // {
    //     printf("File failed to open - %s", json_path);
    //     return EXIT_FAILURE;
    // }
    // fseek(fd, 0l, SEEK_END);
    // long file_len = ftell(fd);

    // if (file_len <= 0)
    // {
    //     printf("failed calculating file length: %ld. Aborting\n", file_len);
    //     return EXIT_FAILURE;
    // }
    // rewind(fd);

    // char* json_str = (char*)calloc(file_len+1, sizeof(char));
    // if (!json_str)
    // {
    //     printf("failed to calloc. Aborting\n");
    //     json_str = NULL;
    //     return EXIT_FAILURE;
    // }

    // for (int i = 0; i < file_len; i++)
    // {
    //     json_str[i] = fgetc(fd);
    // }
    // fclose(fd);

    char *json_str = open_json_and_return_allocated_string(credentials_json_path);
    // free(json_path);

    cJSON *json_parser = NULL;
    json_parser = cJSON_Parse(json_str);
    if (!json_parser)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        cJSON_Delete(json_parser);
        return EXIT_FAILURE;
    }

    int parsing_result = 0;
    parsing_result += parse_raw_json_to_string(device_id, json_str, "duid");
    parsing_result += parse_raw_json_to_string(company_id, json_str, "cpid");
    parsing_result += parse_raw_json_to_string(environment, json_str, "env");
    parsing_result += parse_raw_json_to_string(iotc_server_cert_path, json_str, "iotc_server_cert");

    if (access(iotc_server_cert_path, F_OK) != 0)
    {
        printf("failed to access iotc_server_cert_path - %s ; Aborting\n", iotc_server_cert_path);
        return EXIT_FAILURE;
    }

    parsing_result += parse_raw_json_to_string(sdk_id, json_str, "sdk_id");
    parsing_result += parse_raw_json_to_string(connection_type_str, json_str, "connection_type");

    if (parsing_result != 0)
    {
        return EXIT_FAILURE;
    }

    cJSON *auth_parser = cJSON_GetObjectItemCaseSensitive(json_parser, "auth");
    parsing_result += parse_json_to_string(auth_type, auth_parser, "auth_type");

    if (strcmp(connection_type_str, "IOTC_CT_AWS") == STRINGS_ARE_EQUAL)
    {
        connection_type = IOTC_CT_AWS;
    }
    else if (strcmp(connection_type_str, "IOTC_CT_AZURE") == STRINGS_ARE_EQUAL)
    {
        connection_type = IOTC_CT_AZURE;
    }

    if (strcmp(auth_type, "IOTC_AT_X509") == STRINGS_ARE_EQUAL)
    {
        config.auth_info.type = IOTC_AT_X509;
        char client_key[256] = {};
        char client_cert[256] = {};

        cJSON *params_parser = cJSON_GetObjectItemCaseSensitive(auth_parser, "params");
        parsing_result += parse_json_to_string(client_key, params_parser, "client_key");
        parsing_result += parse_json_to_string(client_cert, params_parser, "client_cert");
        cJSON_free(params_parser);

        if (access(client_key, F_OK) != 0)
        {
            printf("failed to access client_key - %s ; Aborting\n", client_key);
            return EXIT_FAILURE;
        }

        if (access(client_cert, F_OK) != 0)
        {
            printf("failed to access client_cert - %s ; Aborting\n", client_cert);
            return EXIT_FAILURE;
        }

        config.auth_info.data.cert_info.device_cert = client_cert;
        config.auth_info.data.cert_info.device_key = client_key;
    }
    else if (strcmp(auth_type, "IOTC_AT_SYMMETRIC_KEY") == STRINGS_ARE_EQUAL)
    {
        char primary_key[256] = {};

        cJSON *params_parser = cJSON_GetObjectItemCaseSensitive(auth_parser, "params");
        parsing_result += parse_json_to_string(primary_key, params_parser, "primary_key");
        cJSON_free(params_parser);

        config.auth_info.data.symmetric_key = primary_key;
    }
    else if (strcmp(auth_type, "IOTC_AT_TPM") == STRINGS_ARE_EQUAL)
    {
        // config.auth_info.type= IOTC_AT_TPM;
    }
    else if (strcmp(auth_type, "IOTC_AT_TOKEN") == STRINGS_ARE_EQUAL)
    {
        // config.auth_info.type= IOTC_AT_TOKEN;
    }
    else
    {
        printf("unsupported auth type. Aborting\r\n");
        return EXIT_FAILURE;
    }

    cJSON *device_parser = cJSON_GetObjectItemCaseSensitive(json_parser, "device");
    parsing_result += parse_json_to_string(commands_list_path, device_parser, "commands_list_path");

    if (parsing_result != 0)
    {
        return EXIT_FAILURE;
    }

    cJSON *attribute = NULL;
    cJSON *attributes_parser = cJSON_GetObjectItemCaseSensitive(device_parser, "attributes");
    int number_of_attributes = 0;
    cJSON_ArrayForEach(attribute, attributes_parser)
    {
        number_of_attributes++;
    }
    telemetry = (telemetry_attribute_t *)calloc(number_of_attributes, sizeof(telemetry_attribute_t));
    telemetry_attribute_t *telem_ptr = telemetry;
    cJSON_ArrayForEach(attribute, attributes_parser)
    {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(attribute, "name");
        telem_ptr->name_len = strlen(name->valuestring);
        telem_ptr->name = calloc(telem_ptr->name_len, sizeof(char));
        strncpy(telem_ptr->name, name->valuestring, telem_ptr->name_len);

        cJSON *path = cJSON_GetObjectItemCaseSensitive(attribute, "private_data");
        telem_ptr->path_len = strlen(path->valuestring);
        telem_ptr->path = calloc(telem_ptr->path_len, sizeof(char));
        strncpy(telem_ptr->path, path->valuestring, telem_ptr->path_len);

        cJSON *read_type = cJSON_GetObjectItemCaseSensitive(attribute, "private_data_type");
        if (strncmp(read_type->valuestring, "ascii", strlen("ascii")) == STRINGS_ARE_EQUAL)
        {
            telem_ptr->read_ascii = true;
        }

        telem_ptr++;
    }

    cJSON_free(attributes_parser);
    cJSON_free(device_parser);
    cJSON_free(auth_parser);
    cJSON_free(json_parser);
    free(json_str);

    if (init_scripts() != 0)
    {
        return EXIT_FAILURE;
    }

    (void)argc;
    (void)argv;

    config.cpid = company_id;
    config.env = environment;
    config.duid = device_id;
    config.connection_type = connection_type;
    config.auth_info.trust_store = iotc_server_cert_path;
    config.verbose = true;
    config.status_cb = on_connection_status;
    config.ota_cb = on_ota;
    config.cmd_cb = on_command;

    // initialize random seed for the telemetry test
    srand((unsigned int)time(NULL));

    // run a dozen connect/send/disconnect cycles with each cycle being about a minute
    int ret = iotconnect_sdk_init(&config);
    if (0 != ret)
    {
        printf("iotconnect_sdk_init() exited with error code %d\n", ret);
        return ret;
    }

    ret = iotconnect_sdk_connect();
    if (0 != ret)
    {
        printf("iotconnect_sdk_connect() exited with error code %d\n", ret);
        return ret;
    }

    while (iotconnect_sdk_is_connected() && keep_running)
    {
        publish_telemetry(number_of_attributes, telemetry);
        sleep(5);
    }

    iotconnect_sdk_disconnect();
    iotconnect_sdk_deinit();

    printf("Basic sample demo is complete. Exiting.\n");

    free_local_data();

    // free attributes
    for (int i = 0; i < number_of_attributes; i++)
    {
        // printf("%s %s\n", telemetry[i].name, telemetry[i].path);
        free(telemetry[i].name);
        free(telemetry[i].path);
    }
    free(telemetry);

    // free available scripts
    for (int i = 0; i < available_scripts_count; i++)
    {
        free(available_scripts[i]);
    }
    free(available_scripts);

    return 0;
}
