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

# Building files:
gcc -o mqt_hook mqt_hook.c config.c -ljson-c -lpaho-mqtt3c

# Setting up serivce:
nano /etc/systemd/system/mqt_hook.service
'''
[Unit]
Description=MQTT Hook Service
After=network.target

[Service]
Environment="MQTT_HOST=your_mqtt_host"
Environment="MQTT_USERNAME=your_username"
Environment="MQTT_PASSWORD=your_password"
ExecStart=/home/pi/c/mqt_hook/mqt_hook (Optional: /home/pi/c/mqt_hook/config.json)
WorkingDirectory=/home/pi/c/mqt_hook
Restart=always
RestartSec=30
User=pi
Group=pi
Environment="LD_LIBRARY_PATH=/usr/local/lib"
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
'''
# Checking service status:
sudo systemctl status mqt_hook.service
sudo systemctl restart mqt_hook.service

# Reading logs from syslog:
sudo journalctl -f -n 100 | grep mqt_hook
