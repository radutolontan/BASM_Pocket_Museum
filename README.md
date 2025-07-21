
# Hardware Design
## Board ID - Development V0
![screenshot](Resources/schematic_functional.png)

# Software Architecture
The software stack can be split between the following tasks:
## 1. Sensor Task
Responsible with initializing, managing, collecting and processing sensor data. Pushes each new reading to the ***shared_data_buffer***.
### States
* **BOOT** - default state on boot-up; transition to INIT triggered from main.cpp
* **INIT** - initializes I2C bus for comms to sensors, configures and confirms communications to sensors
* **READ** - captures one sample for each sensor value. New, raw values are pushed to the ***shared_data_buffer***.
* **PROCESS** - post-processing required to obtain associated values *(ex. acceleration is integrated once to obtain velocity)*
* **SLEEP** - the state machine waits in this state until a new **READ->PROCESS** cycle is started

### ***shared_data_buffer***
The shared data space for all tasks

 To allow HW expansions and IC replacements, the Sensor Task will make use of a Hardware Abstraction Layer (HAL).

## 2. Evaluator Task
**MVP** - Responsible for gauging completion of missions. The evaluator will be a class-instance structure where missions can be defined based on a time - value scenario *(ex. gravity is aligned with the i_hat + j_hat vector of the IMU for 10 seconds)*. The evaluator task constantly checks if any of the missions has been completed & uploads mission logs and play time to the uSD card.
## 3. Display Task
**MVP** - Manages the primary user feedback tool - the 2x3 RGB LED display. This will be used to show - 
#### a. Raw sensor outputs  - from Sensor Task
#### b. Successful completion of missions - from Evaluator Task
#### c. Recording status - from Logger Task
Furthermore, it will be used on boot-up to indicate SW version as well as successful initialization of all tasks.


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


flowchart TD
    %% Core Tasks
    SensorTask[SensorTask\n(reads sensors, writes data)]
    DisplayTask[DisplayTask\n(reads data, updates LEDs)]
    SDManager[SDManager\n(logs data to SD card)]

    %% Shared Resource
    SharedBuffer[(SharedDataBuffer\n(FIFO + Mutex))]

    %% HAL Interfaces
    ICP[ICP201XXHAL\n(Pressure Sensor)]
    LSM[LSM6DXXHAL\n(IMU)]

    %% External Interfaces
    NeoPixel[NeoPixel LED Strip]
    Button[Pushbutton GPIO]
    SD[SD Card]

    %% Aggregates
    Aggregates[Aggregated Stats\n(min/max/mean/stddev)]

    %% Relationships
    SensorTask --> ICP
    SensorTask --> LSM
    SensorTask -->|addReading()| SharedBuffer

    DisplayTask -->|getReadings(), getAggregatedStats()| SharedBuffer
    DisplayTask --> NeoPixel
    Button -->|State Toggle| DisplayTask

    SDManager -->|getReadings()| SharedBuffer
    SDManager --> SD

    SharedBuffer --> Aggregates

    %% Styling
    style SharedBuffer fill:#f0f0f0,stroke:#333,stroke-width:2px
    style SensorTask fill:#c6e2ff,stroke:#333
    style DisplayTask fill:#ffd580,stroke:#333
    style SDManager fill:#d1ffbd,stroke:#333
    style Aggregates fill:#fdfd96,stroke:#666,stroke-dasharray: 5 5