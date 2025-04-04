# mqt_hook

A home automation switch augmented with Home Assistant MQTT discovery protocol.

## Prerequisites

### System Dependencies
```bash
sudo apt-get install libjson-c-dev libpaho-mqtt-dev
```

### Build Dependencies
```bash
sudo apt-get install build-essential libssl-dev cmake gcc make cmake-gui cmake-curses-gui libssl-dev doxygen graphviz
```

### Optional Dependencies
For UPnP functionality:
```bash
sudo apt-get install miniupnpc
```

## Building

Compile the project using:
```bash
gcc -o mqt_hook mqt_hook.c config.c -ljson-c -lpaho-mqtt3c
```

## Service Setup

1. Create a systemd service file:
```bash
sudo nano /etc/systemd/system/mqt_hook.service
```

2. Add the following content to /etc/systemd/system/mqt_hook.service
```ini
[Unit]
Description=MQTT Hook Service
After=network.target

[Service]
Environment="MQTT_HOST=your_mqtt_host"
Environment="MQTT_USERNAME=your_username"
Environment="MQTT_PASSWORD=your_password"
ExecStart=/home/pi/c/mqt_hook/mqt_hook
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
```

Note: You can optionally specify a config file path: `/home/pi/c/mqt_hook/config.json`

## Service Management

Check service status:
```bash
sudo systemctl status mqt_hook.service
```

Restart service:
```bash
sudo systemctl restart mqt_hook.service
```

## Monitoring

View service logs:
```bash
sudo journalctl -f -n 100 | grep mqt_hook
```