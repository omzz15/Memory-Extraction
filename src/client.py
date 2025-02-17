import serial

PORT = "/dev/ttyACM0"
BAUD = 115200

def verify_cmd_success() -> bool:
    """
    Used to verify the exit code is good after sending a command. Else it will print the error code.
    
    Returns true is command was successful and false otherwise.
    """
    res = con.read()
    if(res[0] != 0x00): # success
        print(f"Error: Device responded with unsuccesful exit code: {res}")
        return False
    
    return True

def send_addr(addr: int) -> None:
    """
    Handles converting the decimal address to bytes and sending it to the microcontroller.
    
    You should not need to use this command as read_range and read handle this for you.
    """
    con.write(int.to_bytes(addr, length=ADDR_BYTE_SIZE, byteorder='little'))

def read_range(start: int, end: int) -> bytes:
    """
    Reads a range of addresses from start (inclusive) to end (inclusive).

    Returns a byte array of the data or None if the read failed.
    """
    if(start > end):
        print("Error: start address is more than end so no range to read")
        return None
    if(end > MAX_ADDR):
        print("Warning: end address is more than max address, truncating...")
        end = MAX_ADDR
        if(start > end):
            print("Error: start address is also more than max address")
            return None

    con.write(int.to_bytes(2))
    send_addr(start)
    send_addr(end)
    
    if(not verify_cmd_success()):
        return None
    
    data = con.read((end-start + 1)*BUS_BYTE_SIZE)
    if(len(data) != (end-start + 1)*BUS_BYTE_SIZE):
        print("Error: incorrect amount of data recived from read range")
        return None
    return data

def read(addr: int) -> bytes:
    """
    Reads a single address and returns a byte array of the data or None if the read failed.
    """
    if(addr > MAX_ADDR):
        print("Error: address is more than max address.")
        return None
    
    con.write(int.to_bytes(1))
    send_addr(addr)

    if(not verify_cmd_success()):
        return None
    
    data = con.read(BUS_BYTE_SIZE)
    if(len(data) != BUS_BYTE_SIZE):
        print("Error: incorrect amount of data recived from read")
        return None
    
    return data

    

print("Connecting to device")

con = serial.Serial(PORT, BAUD, timeout=10)
con.reset_input_buffer()

print("Connected! Getting data")
con.write(int.to_bytes(0)) # get data
con.flush()

if(not verify_cmd_success()):
   exit(1)
    
data = con.read(5)
if(data[0] != 0x10): # magic verification number
    print("Error: did not recive valid data")
    exit(1)

ADDR_SIZE = data[1]
MAX_ADDR = 2**ADDR_SIZE - 1
BUS_SIZE = data[2]
ADDR_BYTE_SIZE = data[3]
BUS_BYTE_SIZE = data[4]

print("Device data:")
print(f"\tAddress size: {ADDR_SIZE}")
print(f"\tMax address: {MAX_ADDR}")
print(f"\tBus size: {BUS_SIZE}")

print("Getting all data in 4 kword blocks...")

with open("data.bin", "wb") as f:
    BLOCK_SIZE = 4096
    MAX_RETRIES = 5

    current_address = 0
    retries = 0
    while(current_address < MAX_ADDR):
        print(f"Reading block {current_address // BLOCK_SIZE}, address {current_address} - {current_address + BLOCK_SIZE}")
        data = read_range(current_address, current_address + BLOCK_SIZE)
        if(data == None):
            retries += 1
            if(retries > MAX_RETRIES):
                print(f"Failed to read data! End address {current_address}")
                exit(1)
            print(f"Failed to read data... retry {retries}/{MAX_RETRIES}")
            continue

        f.write(data)
        current_address += BLOCK_SIZE
        retries = 0

print("Complete!")