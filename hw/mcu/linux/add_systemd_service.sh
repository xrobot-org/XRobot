#!/bin/bash
sudo echo "[Unit]
Description=XRobot Service
After=network.target

[Service]
Type=simple
Restart=always
RestartSec=5s
ExecStart=bash -c \"export HOME=${HOME} && nohup /usr/bin/xrobot > null\"
ExecReload=bash -c \"export HOME=${HOME} && nohup /usr/bin/xrobot > null\"

[Install]
WantedBy=multi-user.target" > /etc/systemd/system/xrobot.service
