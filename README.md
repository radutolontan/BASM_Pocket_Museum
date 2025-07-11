
# Hardware Design
## Board ID - Development V0
![screenshot](Resources/schematic_functional.png)

# Software Design
The software stack can be split between the following tasks:
## 1. Sensor Task
**MVP** - Responsible with initializing, collecting and processing sensor data. By processing we refer to filtering of raw data and post-rpocessing required to obtain associated values *(ex. acceleration is integrated once to obtain velocity)*. To allow HW expansions and IC replacements, the Sensor Task will make use of a Hardware Abstraction Layer (HAL).
## 2. Evaluator Task
**MVP** - Responsible for gauging completion of missions. The evaluator will be a class-instance structure where missions can be defined based on a time - value scenario *(ex. gravity is aligned with the i_hat + j_hat vector of the IMU for 10 seconds)*. The evaluator task constantly checks if any of the missions has been completed & uploads mission logs and play time to the uSD card.
## 3. Display Task
**MVP** - Manages the primary user feedback tool - the 2x3 RGB LED display. This will be used to show - 
#### a. Raw sensor outputs  - from Sensor Task
#### b. Successful completion of missions - from Evaluator Task
#### c. Recording status - from Logger Task
Furthermore, it will be used on boot-up to indicate SW version as well as successful initialization of all tasks.
## 4. MP3-Player Task
**NOT MVP** - Manages the secondary user feedback tool - an MP3 player which loads audio files from the SD cards and allows the user to control playback using sensor inputs *(ex. increasing air pressure for 5 sec. by 5 hPa will increase the volume by one unit)* . Similar to controlling the Sensor task, the MP3-Player will make use of a HAL, allowing for future changes to the HW.
## 5. Logger Task
**NOT MVP** - Manages logging of sensor data to the on-board uSD card. Will be activated using one of the on-board tactile push-buttons, and when enabled, it will provide feedback to the user on logging status using the Display Task.

# SD Card Formatting
In order to read and write to the SD card, it has to be formatted to **FAT32** and **MBR**. Insert the microSD card into a compatible card reader, connect to a Linux machine, and locate it using the following Terminal command: use a Terminal to:
```bash
lsblk
``` 
Then, in a Terminal, using its ***/dev/sdX*** identity:
```bash
sudo umount /dev/sdX*
sudo wipefs --all /dev/sdX
sudo fdisk /dev/sdX
``` 
Within ***fdisk***, configure the following proerties:
* Press ***o*** → create a new DOS (MBR) partition table 
* Press ***n*** → create a new primary partition
* Accept all defaults
* Press ***t***, then type ***c*** → change type to W95 FAT32 (LBA)
* Press ***w*** → write changes

Finally, format to FAT32 using:
```bash
sudo mkfs.vfat -F 32 -n SDCARD /dev/sdX1
``` 