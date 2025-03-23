#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>
#include <time.h>
#include <syslog.h>

// Function declarations
int loadConfig(const char* filename);
void freeConfig(void);


typedef struct {
    char* address;
    char* clientid;
    char* username;
    char* password;
    int qos;
    long timeout;
    int keepalive;
    int cleansession;
    int retained;
} mqttConfig;

typedef struct {
    char* switch_name;
    char* unique_id;
    char* command_topic;
    char* state_topic;
    char* availability_topic;
    char* ts_topic;
    char* payload_on;
    char* payload_off;
    char* payload_available;
    char* payload_not_available;
    char* state_on;
    char* state_off;
} hassSwitchConfig;