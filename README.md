# mqt_hook
a augmented switch implemented with the home assistant mqtt discovery protocol

# Requirements
sudo apt-get install 
- libjson-c-dev
- libpaho-mqtt-dev
# General requirements:
- build-essential libssl-dev cmake gcc make cmake-gui cmake-curses-gui libssl-dev doxygen graphviz

# To get upnp stats:
'''
sudo apt-get install miniupnpc
'''
# Building only with mqt_hook.c:
gcc -o mqt_hook mqt_hook.c -lpaho-mqtt3c

# Building with config files:
gcc -o mqt_hook_test mqt_hook.c config.c -ljson-c -lpaho-mqtt3c

# Reading logs from syslog:
sudo journalctl -f -n 100 | grep mqtt_hook_test
