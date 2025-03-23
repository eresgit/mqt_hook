#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>
#include <time.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>

#include "config.h"

// Switch configuration
const char* switch_name = "MQTT Hook";
const char* unique_id = "mqt_hook";
const char* command_topic = "homeassistant/switch/mqt_hook/set";
const char* state_topic = "homeassistant/switch/mqt_hook/state";
const char* availability_topic = "homeassistant/switch/mqt_hook/availability";
const char* ts_topic = "homeassistant/switch/mqt_hook/time";
const char* payload_on = "ON";
const char* payload_off = "OFF";
const char* payload_available = "online";
const char* payload_not_available = "offline";
const char* state_on = "ON";
const char* state_off = "OFF";


static volatile sig_atomic_t keep_running = 1;

// Global config variables from config.c
extern hassSwitchConfig gConfHass;
extern mqttConfig gConfMqtt;

// Flag to indicate that the switch is on or off to main loop
uint8_t gSwitch_state = 0;
uint8_t gPrevious_state = 0;

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    /*
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);
    */
    MQTTClient client = (MQTTClient)context;
    char* payload = message->payload;
    if (strcmp(topicName, gConfHass.command_topic) == 0) {
        if (strcmp(payload, gConfHass.payload_on) == 0) {
            gSwitch_state = 1;
            syslog(LOG_INFO, "msgarrvd: Turning switch ON\n");
        } else if (strcmp(payload, gConfHass.payload_off) == 0) {
            gSwitch_state = 0;
            syslog(LOG_INFO, "msgarrvd: Turning switch OFF\n");
        }
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    syslog(LOG_ERR, "\nConnection lost\n");
    syslog(LOG_ERR, "     cause: %s\n", cause);
}

// Add this signal handler function before main()
static void signal_handler(int signum) {
    syslog(LOG_INFO, "Received signal %d, cleaning up...\n", signum);
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    // Setup signal handling
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = signal_handler;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    // Initialize syslog
    openlog("mqtt_hook_test", LOG_PID|LOG_CONS, LOG_USER);

    // Use argv[1] as config file name if provided, else use config.json:
    if (argc > 1) {
        if (loadConfig(argv[1]) != 0) {
            syslog(LOG_ERR, "Failed to load configuration %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    } else if (loadConfig("config.json") != 0) {
        syslog(LOG_ERR, "Failed to load configuration config.json\n");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "MQTT Hook started\n");
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    // Replace hardcoded values with config values
    MQTTClient_create(&client, gConfMqtt.address, gConfMqtt.clientid, 
    MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = gConfMqtt.keepalive;
    conn_opts.cleansession = gConfMqtt.cleansession;
    conn_opts.username = gConfMqtt.username;
    conn_opts.password = gConfMqtt.password;

/*     MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = USERNAME;
    conn_opts.password = PASSWORD; */

    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    will_opts.topicName = availability_topic;
    will_opts.message = payload_not_available;
    will_opts.qos = QOS;
    will_opts.retained = 1;
    conn_opts.will = &will_opts;

/*     // Add last will and testament:
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    will_opts.topicName = gConfHass.availability_topic;
    will_opts.message = gConfHass.payload_not_available;
    will_opts.qos = gConfMqtt.qos;
    will_opts.retained = gConfMqtt.retained;
    conn_opts.will = &will_opts; */

    if ((rc = MQTTClient_setCallbacks(client, client, NULL, msgarrvd, NULL)) != MQTTCLIENT_SUCCESS) {
        syslog(LOG_ERR, "Failed to set callbacks, return code %d\n", rc);
        exit(EXIT_FAILURE);
    } else {
        syslog(LOG_INFO, "Callbacks set successfully\n");
    }

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        syslog(LOG_ERR, "Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    if ((rc = MQTTClient_subscribe(client, gConfHass.command_topic, gConfMqtt.qos)) != MQTTCLIENT_SUCCESS) {
        syslog(LOG_ERR, "Failed to subscribe to topic %s, return code %d\n", gConfHass.command_topic, rc);
        exit(EXIT_FAILURE);
    } else {
        syslog(LOG_INFO, "Successfully subscribed to topic %s\n", gConfHass.command_topic);
    }

    // Publish the switch configuration to the discovery topic
    char discovery_topic[256];
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/switch/%s/config", gConfHass.unique_id);
    char switch_config[512];
    snprintf(switch_config, sizeof(switch_config),
                "{\"name\":\"%s\",\"unique_id\":\"%s\",\"command_topic\":\"%s\",\"state_topic\":\"%s\",\"availability_topic\":\"%s\",\"payload_on\":\"%s\",\"payload_off\":\"%s\",\"payload_available\":\"%s\",\"payload_not_available\":\"%s\",\"state_on\":\"%s\",\"state_off\":\"%s\"}",
                gConfHass.switch_name, gConfHass.unique_id, gConfHass.command_topic, gConfHass.state_topic, gConfHass.availability_topic, gConfHass.payload_on, gConfHass.payload_off, gConfHass.payload_available, gConfHass.payload_not_available, gConfHass.state_on, gConfHass.state_off);
    MQTTClient_publish(client, discovery_topic, strlen(switch_config), switch_config, gConfMqtt.qos, 1, NULL);

    // Publish the availability state as online
    MQTTClient_publish(client, gConfHass.availability_topic, strlen(gConfHass.payload_available), gConfHass.payload_available, gConfMqtt.qos, 1, NULL);

    // Publish the initial state of the switch
    MQTTClient_publish(client, gConfHass.state_topic, strlen(gConfHass.state_off), gConfHass.state_off, gConfMqtt.qos, 1, NULL);

    // Start the MQTT loop
    while (keep_running) {
        // Seems like we get here after every callback :)
        // Check if global flag is set, publish the state of the switch accordingly:
        if (gSwitch_state != gPrevious_state) {
            // Also publish the epoch timestamp and the switch state to a special topic
            time_t now;
            time(&now);
            char state_payload[256];
            if (gSwitch_state) {
                MQTTClient_publish(client, gConfHass.state_topic, strlen(gConfHass.state_on), gConfHass.state_on, gConfMqtt.qos, 1, NULL);
                snprintf(state_payload, sizeof(state_payload), "{\"time\":\"%ld\",\"state\":\"%s\"}", now, gConfHass.state_on);
            } else {
                MQTTClient_publish(client, gConfHass.state_topic, strlen(gConfHass.state_off), gConfHass.state_off, gConfMqtt.qos, 1, NULL);
                snprintf(state_payload, sizeof(state_payload), "{\"time\":\"%ld\",\"state\":\"%s\"}", now, gConfHass.state_off);
            }
            MQTTClient_publish(client, gConfHass.ts_topic, strlen(state_payload), state_payload, gConfMqtt.qos, 1, NULL);
            gPrevious_state = gSwitch_state;
        }
        MQTTClient_yield();
    }
    // Cleanup code
    syslog(LOG_INFO, "MQTT Hook shutting down..\n");
    freeConfig();
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    printf("MQTT Hook stopped: %d\n",rc);
    return rc;
}