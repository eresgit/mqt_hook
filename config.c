#include "config.h"
#include <json-c/json.h>
#include <unistd.h>

// Global config variables
hassSwitchConfig gConfHass;
mqttConfig gConfMqtt;

static char* expandEnvironmentVariables(const char* value) {
    if (!value) return NULL;
    if (value[0] != '$') return strdup(value);  // No substitution needed

    // Remove ${} and get env var name
    const char* start = strchr(value, '{');
    const char* end = strchr(value, '}');
    if (!start || !end) return strdup(value);  // Malformed variable

    // Extract variable name
    int len = end - start - 1;
    char env_name[256];
    strncpy(env_name, start + 1, len);
    env_name[len] = '\0';

    // Get environment variable value
    const char* env_value = getenv(env_name);
    if (!env_value) {
        syslog(LOG_ERR, "Environment variable %s not found\n", env_name);
        return NULL;
    }

    return strdup(env_value);
}

// Helper function to safely copy string from JSON with environment variable expansion
static char* copyJsonString(struct json_object* obj, const char* field) {
    struct json_object* str_obj;
    if (json_object_object_get_ex(obj, field, &str_obj)) {
        const char* value = json_object_get_string(str_obj);
        // First try to expand environment variables, if any
        return expandEnvironmentVariables(value);
    }
    return NULL;
}

static int writeDefaultConfig(const char* filename) {
    syslog(LOG_INFO, "Creating configuration file [%s] with default settings\n", filename);
    struct json_object *root, *hass_config, *mqtt_config;
    
    root = json_object_new_object();
    hass_config = json_object_new_object();
    mqtt_config = json_object_new_object();
    
    // Add default HASS switch config
    json_object_object_add(hass_config, "switch_name", json_object_new_string("Default Switch"));
    json_object_object_add(hass_config, "unique_id", json_object_new_string("default_switch_001"));
    json_object_object_add(hass_config, "command_topic", json_object_new_string("homeassistant/switch/default_switch_001/set"));
    json_object_object_add(hass_config, "state_topic", json_object_new_string("homeassistant/switch/default_switch_001/state"));
    json_object_object_add(hass_config, "availability_topic", json_object_new_string("homeassistant/switch/default_switch_001/available"));
    json_object_object_add(hass_config, "ts_topic", json_object_new_string("homeassistant/switch/default_switch_001/timestamp"));
    json_object_object_add(hass_config, "payload_on", json_object_new_string("ON"));
    json_object_object_add(hass_config, "payload_off", json_object_new_string("OFF"));
    json_object_object_add(hass_config, "payload_available", json_object_new_string("online"));
    json_object_object_add(hass_config, "payload_not_available", json_object_new_string("offline"));
    json_object_object_add(hass_config, "state_on", json_object_new_string("ON"));
    json_object_object_add(hass_config, "state_off", json_object_new_string("OFF"));
    
    // Add default MQTT config
    json_object_object_add(mqtt_config, "host", json_object_new_string("${MQTT_HOST}"));
    json_object_object_add(mqtt_config, "clientId", json_object_new_string("mqtt_default_switch_001"));
    json_object_object_add(mqtt_config, "username", json_object_new_string("${MQTT_USERNAME}"));
    json_object_object_add(mqtt_config, "password", json_object_new_string("${MQTT_PASSWORD}"));
    json_object_object_add(mqtt_config, "qos", json_object_new_int(1));
    json_object_object_add(mqtt_config, "timeout", json_object_new_int(10000));
    json_object_object_add(mqtt_config, "keepalive", json_object_new_int(20));
    json_object_object_add(mqtt_config, "cleansession", json_object_new_int(1));
    json_object_object_add(mqtt_config, "retained", json_object_new_int(1));
    
    // Add sections to root
    json_object_object_add(root, "hassSwitchConfig", hass_config);
    json_object_object_add(root, "mqttConfig", mqtt_config);
    
    // Write to file
    if (json_object_to_file_ext(filename, root, JSON_C_TO_STRING_PRETTY) < 0) {
        syslog(LOG_ERR, "Failed to write default config to [%s]\n", filename);
        json_object_put(root);
        return -1;
    }
    
    json_object_put(root);
    printf("configuration file [%s] with default settings was created..\n", filename);
    return 0;
}

int loadConfig(const char* filename) {
    struct json_object *root;

    // Check if file exists
    if (access(filename, F_OK) != 0) {
        syslog(LOG_INFO, "Config file [%s] not found!\n", filename);
        if (writeDefaultConfig(filename) != 0) {
            return -1;
        }
    }
    // Load JSON file
    root = json_object_from_file(filename);
    if (!root) {
        syslog(LOG_ERR, "Error loading config file: [%s]\n", filename);
        return -1;
    }
    // Parse HASS Switch Config
    struct json_object *hass_config;
    if (!json_object_object_get_ex(root, "hassSwitchConfig", &hass_config)) {
        syslog(LOG_ERR, "hassSwitchConfig section not found in config file\n");
        json_object_put(root);
        return -1;
    }
    // Initialize all pointers to NULL
    memset(&gConfHass, 0, sizeof(hassSwitchConfig));
    memset(&gConfMqtt, 0, sizeof(mqttConfig));

    // Parse HASS Switch Config with error checking
    gConfHass.switch_name = copyJsonString(hass_config, "switch_name");
    gConfHass.unique_id = copyJsonString(hass_config, "unique_id");
    gConfHass.command_topic = copyJsonString(hass_config, "command_topic");
    gConfHass.state_topic = copyJsonString(hass_config, "state_topic");
    gConfHass.availability_topic = copyJsonString(hass_config, "availability_topic");
    gConfHass.ts_topic = copyJsonString(hass_config, "ts_topic");
    gConfHass.payload_on = copyJsonString(hass_config, "payload_on");
    gConfHass.payload_off = copyJsonString(hass_config, "payload_off");
    gConfHass.payload_available = copyJsonString(hass_config, "payload_available");
    gConfHass.payload_not_available = copyJsonString(hass_config, "payload_not_available");
    gConfHass.state_on = copyJsonString(hass_config, "state_on");
    gConfHass.state_off = copyJsonString(hass_config, "state_off");

    // Verify all required fields are present
    if (!gConfHass.switch_name || !gConfHass.unique_id || !gConfHass.command_topic ||
        !gConfHass.state_topic || !gConfHass.availability_topic || !gConfHass.ts_topic ||
        !gConfHass.payload_on || !gConfHass.payload_off || !gConfHass.payload_available ||
        !gConfHass.payload_not_available || !gConfHass.state_on || !gConfHass.state_off) {
        syslog(LOG_ERR, "Missing required fields in hassSwitchConfig\n");
        freeConfig();
        json_object_put(root);
        return -1;
    }

    // Parse MQTT Config
    struct json_object *mqtt_config;
    if (!json_object_object_get_ex(root, "mqttConfig", &mqtt_config)) {
        syslog(LOG_ERR, "mqttConfig section not found in config file\n");
        freeConfig();
        json_object_put(root);
        return -1;
    }

    gConfMqtt.address = copyJsonString(mqtt_config, "host");
    gConfMqtt.clientid = copyJsonString(mqtt_config, "clientId");
    gConfMqtt.username = copyJsonString(mqtt_config, "username");
    gConfMqtt.password = copyJsonString(mqtt_config, "password");

    // Verify MQTT required fields
    if (!gConfMqtt.address || !gConfMqtt.clientid || 
        !gConfMqtt.username || !gConfMqtt.password) {
        syslog(LOG_ERR, "Missing required fields in mqttConfig\n");
        freeConfig();
        json_object_put(root);
        return -1;
    }
    
    struct json_object *num_obj;
    if (json_object_object_get_ex(mqtt_config, "qos", &num_obj)) {
        gConfMqtt.qos = json_object_get_int(num_obj);
    } else {
        syslog(LOG_ERR, "QOS field not found in mqttConfig\n");
        freeConfig();
        json_object_put(root);
        return -1;
    }

    if (json_object_object_get_ex(mqtt_config, "timeout", &num_obj)) {
        gConfMqtt.timeout = json_object_get_int(num_obj);
    } else {
        syslog(LOG_ERR, "Timeout field not found in mqttConfig\n");
        freeConfig();
        json_object_put(root);
        return -1;
    }

    if (json_object_object_get_ex(mqtt_config, "keepalive", &num_obj)) {
        gConfMqtt.keepalive = json_object_get_int(num_obj);
    } else {
        syslog(LOG_ERR, "keepalive field not found in mqttConfig\n");
        freeConfig();
        json_object_put(root);
        return -1;
    }

    if (json_object_object_get_ex(mqtt_config, "cleansession", &num_obj)) {
        gConfMqtt.cleansession = json_object_get_int(num_obj);
    } else {
        syslog(LOG_ERR, "cleansession field not found in mqttConfig\n");
        freeConfig();
        json_object_put(root);
        return -1;
    }

    if (json_object_object_get_ex(mqtt_config, "retained", &num_obj)) {
        gConfMqtt.retained = json_object_get_int(num_obj);
    } else {
        syslog(LOG_ERR, "retained field not found in mqttConfig\n");
        freeConfig();
        json_object_put(root);
        return -1;
    }

    json_object_put(root);
    syslog(LOG_INFO, "Configuration [%s] loaded successfully\n", filename);
    return 0;
}

void freeConfig() {
    // Free HASS Switch Config
    free(gConfHass.switch_name);
    free(gConfHass.unique_id);
    free(gConfHass.command_topic);
    free(gConfHass.state_topic);
    free(gConfHass.availability_topic);
    free(gConfHass.ts_topic);
    free(gConfHass.payload_on);
    free(gConfHass.payload_off);
    free(gConfHass.payload_available);
    free(gConfHass.payload_not_available);
    free(gConfHass.state_on);
    free(gConfHass.state_off);

    // Free MQTT Config
    free(gConfMqtt.address);
    free(gConfMqtt.clientid);
    free(gConfMqtt.username);
    free(gConfMqtt.password);
}