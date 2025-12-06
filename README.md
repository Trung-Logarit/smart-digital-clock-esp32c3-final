# Smart Digital Clock with ESP32-C3 (Final Report)

## Course: Embedded Systems (Lecturer: Ho Viet Viet)

## Team Members (RUNNER-UP)

| **Name**              | **Class**   | **Student ID** |
|-----------------------|-------------|----------------|
| **Nguyen Ngoc Trung** | 21KTMT2     | 106210257      |
| **Le Duong Khang**    | 21KTMT2     | 106210242      |
| **Hoang Bao Long**    | 21KTMT1     | 106210049      |

---

## Demo Video
Watch Demo Video: https://dutudn-my.sharepoint.com/:f:/g/personal/106210257_sv1_dut_udn_vn/EigLzfVeIWNColr-CX5cwPYBjQlyvzLXpPhvysLrDujbmA?e=7NNd9V

---

## Slide
Report File: https://www.canva.com/design/DAG5-IJQnEA/mzOvdsf9NDlQdDD-We-umw/edit?utm_content=DAG5-IJQnEA&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton

---
## Overview
This project implements a **Smart Digital Clock** on the **ESP32-C3** platform using **FreeRTOS**.  
The system provides clock functionality along with alarm, stopwatch, countdown timer, and temperature/humidity monitoring.  

---

## Features
- **Basic Clock Display**: Show current time and date on LED matrix.  
- **Alarm**: Set alarms and trigger buzzer + LED when active.  
- **Stopwatch**: Start, pause, resume, and reset time measurement.  
- **Countdown Timer**: Configurable countdown with buzzer alert at zero.  
- **Temperature & Humidity**: Collected from the DHT11 sensor.  
- **ESP Rainmaker**: Set alarm via ESP Rainmaker.
- **NTP Time Sync**: Synchronize time over Wi-Fi with an NTP server.
- **ESP Insights**: Monitor Wifi, Heap, Sensor DHT11.

---

## Hardware
- **ESP32-C3** – main microcontroller  
- **MAX7219 LED Matrix** – display for time and data  
- **DHT11 Sensor** – temperature and humidity sensing  
- **Buzzer + LEDs** – alarm notifications  
- **Push Buttons** – user input for control  

---

## Quick Start

### Requirements
- ESP-IDF v5.x installed and configured
- ESP32-C3 development board
- USB cable for flashing

### Build & Flash
```bash
# Clone repository
git clone [https://github.com/Trung-Logarit/smart-digital-clock-esp32c3](https://github.com/Trung-Logarit/smart-digital-clock-esp32c3-final.git)
cd smart-digital-clock-esp32c3-final

# Set target chip
idf.py set-target esp32c3

# Build project
idf.py build

# Flash to board and monitor output
idf.py -p <PORT> flash monitor
```
---

## License
- This project is licensed under the MIT License – see the LICENSE file for details.
## Contact
- Email: trungnguyenraz@gmail.com (Trung Logaric)








