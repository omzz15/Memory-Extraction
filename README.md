# Memory Extraction
This is a simple script to allow microcontrollers to read data from flash chips and communicate through a serial interface. It is ment for chips that output data based on address, chip enable, and, output enable pins (which is pretty common to my knowledge).

This was originally created to read the samsung K5L2763CAM-D770 (I could only find the [K5L2731CAM-D770](https://datasheetspdf.com/mobile/688253/SamsungElectronics/K5L2731CAM-D770/1) datasheet) but it should be easy to modify for many diffrent chips.

## Instillation
To install the code to a microcontroller:
- Clone this repo
    ```bash
    git clone https://github.com/omzz15/Memory-Extraction
    ```
- Open the project. This repository is a [platformio](https://platformio.org/) project so you can open it in vs-code with the extention.
- Configure the code. You will probabibly need to configure some things at the top of [main.cpp](./src/main.cpp) such as pin definitions and address or bus size.
- Configure your target board in [platformio.ini](./platformio.ini) (it is preconfigured for teensy 4.1 because it is fast and has a lot of I/O)
- Upload the code with the upload button in the bottom left or through the CLI ```platformio run --target upload```

## How to use
Once you have uploaded the code, you can interface with the microcontroller over serial. It uses a simple protocol like this:
```
Spec:
 Send command (1 byte)
 Send args (optional)
 Recv exit code (1 byte)(0x00 = okay)
 Recv return bytes

Exit codes:
 0x00: exit success
 0x01: command not found
 0x02: invalid args
 0x03: read failed
 0x04: malloc failed

Commands (args -> return)
 Info(0x00): no args (0 bytes) -> info about config (5 bytes)
 Read(0x01): address (addrByteSize) -> data (busByteSize)
 Read Range(0x02): address start(inclucive) (addrByteSize), address end(inclusive) (addrByteSize) -> data (# addresses * busByteSize)

Note: addrByteSize and busByteSize are defined as the number of bytes needed to store a single address and the data at that address.
```
Alternativly you can use the python script [client.py](./src/client.py) which will handle connecting and communicating with the microcontroller and has read and read_range methods that can be used to read from the flash. You may need to configure the port and baud at the top of the file.

By default this script will connect, read all data, and save it to a data.bin file. You can also edit it to do whatever you want but keep everything above where it prints the device data as that sets the values to make sure it communicates properly.