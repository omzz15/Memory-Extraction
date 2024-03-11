// Spec:
// Send command (1 byte)
// Send args (optional)
// Recv exit code (1 byte)(0 = okay)
// Recv return bytes

// Exit codes:
// 0x00: exit success
// 0x01: command not found
// 0x02: invalid args
// 0x03: read failed
// 0x04: malloc failed

// Command args
// Info(0x00): no args
// Read(0x01): address (addrByteSize)
// Read Range(0x02): address start(inclucive)(addrByteSize), address end(inclusive)(addrByteSize)

#include <Arduino.h>

#define ADDR_SIZE 24
#define BUS_SIZE 16
#define READ_DELAY 100

#define MAX_RETRIES 10 // maximum number of times a read can fail before read failed exit code is sent

const uint8_t addrByteSize = ceil(ADDR_SIZE / 8.0);
const uint8_t busByteSize = ceil(BUS_SIZE / 8.0);

// Pins
const uint8_t chipEnablePin = 14;
const uint8_t readEnablePin = 15;
const uint8_t addrPins[ADDR_SIZE] = {13,41,40,39,38,37,36,35,29,28,27,26,25,24,12,11,10,34,33,30,31,32,9,8};
const uint8_t busPins[BUS_SIZE] = {16,18,20,22,0,2,4,6,17,19,21,23,1,3,5,7};

// function declarations:
void read(byte[addrByteSize], byte[busByteSize]);
void read(size_t, byte[busByteSize]);
bool readAndValidate(byte[addrByteSize], byte[busByteSize]);
bool readAndValidate(size_t, byte[busByteSize]);

byte getByte();
void writeBytes(byte *data, size_t size);

void handleInfo();
void handleRead();
void handleReadRange();

void setup() {
  Serial.begin(115200);
  // setup pins
  pinMode(chipEnablePin, OUTPUT);
  pinMode(readEnablePin, OUTPUT);
  for(u_int8_t i = 0; i < ADDR_SIZE; i++){
    pinMode(addrPins[i], OUTPUT);
  }
  for(u_int8_t i = 0; i < BUS_SIZE; i++){
    pinMode(busPins[i], INPUT_PULLDOWN);
  }

  // enable chip and read (active low)
  digitalWrite(chipEnablePin, HIGH);
  digitalWrite(readEnablePin, HIGH);

  Serial.println("Welcome to the flash extraction system! Use the client script to interface with your flash.");
}

void loop() {
  switch (getByte())
  {
  case 0x00: // Info
    handleInfo();
    break;
  case 0x01: // Read
    handleRead();
    break;
  case 0x02:
    handleReadRange();
    break;
  default:
    Serial.write(0x01);
    break;
  }
}

// put function definitions here:
void read(byte addr[addrByteSize], byte output[busByteSize]){
  size_t addrConv = 0;
  memcpy(&addrConv, addr, addrByteSize);
  read(addrConv, output);
}

void read(size_t addr, byte output[busByteSize]){
  for(u_int8_t i = 0; i < ADDR_SIZE; i++){
    digitalWrite(addrPins[i], bitRead(addr, i));
  }

  digitalWrite(chipEnablePin,LOW);
  digitalWrite(readEnablePin, LOW);

  delayNanoseconds(READ_DELAY);

  for(uint8_t i = 0; i < busByteSize; i++){
    byte data = 0;
    for(u_int8_t j = 0; j < 8; j++){
      data |= digitalRead(busPins[i*8 + j]) << j;
    }
    output[i] = data;
  }

  digitalWrite(readEnablePin, HIGH);
  digitalWrite(chipEnablePin, HIGH);
}

bool readAndValidate(byte addr[addrByteSize], byte output[busByteSize]){
  size_t addrConv = 0;
  memcpy(&addrConv, addr, addrByteSize);
  return readAndValidate(addrConv, output);
}

bool readAndValidate(size_t addr, byte output[busByteSize]){
  uint8_t retries = 0;
  while(retries < MAX_RETRIES){
    byte out[busByteSize];
    byte val1[busByteSize];
    byte val2[busByteSize];

    read(addr, out);
    read(addr, val1);
    read(addr, val2);

    if(memcmp(out, val1, busByteSize) || memcmp(out, val2, busByteSize)){
      retries ++;
      continue;
    }

    memcpy(output, out, busByteSize);
    return true;
  }

  return false;
}

byte getByte(){
  while(!Serial.available()){}
  return Serial.read();
}

void writeBytes(byte *data, size_t size){
  for(size_t i = 0; i < size; i++)
    Serial.write(data[i]);
}

void handleInfo(){
  Serial.write(0x00); // success
  Serial.write(0x10); // magic number
  Serial.write(ADDR_SIZE);
  Serial.write(BUS_SIZE);
  Serial.write(addrByteSize);
  Serial.write(busByteSize);
}

void handleRead(){
  byte addr[addrByteSize]; 
  int len = Serial.readBytes((char*)addr, addrByteSize);
  if(len != addrByteSize){
    Serial.write(0x02); // invalid args
    return;
  }

  byte out[busByteSize];

  if(!readAndValidate(addr, out)){
    Serial.write(0x03); //read failed
    return;
  }

  Serial.write(0x00); // success
  writeBytes(out, busByteSize);
}

void handleReadRange(){
  byte args[addrByteSize*2]; 
  int len = Serial.readBytes((char*)args, addrByteSize*2);
  if(len != addrByteSize*2){
    Serial.write(0x02); // invalid args
    return;
  }

  size_t addrStart = 0;
  size_t addrEnd = 0;
  
  memcpy(&addrStart, args, addrByteSize);
  memcpy(&addrEnd, args + addrByteSize, addrByteSize);

  size_t size = addrEnd - addrStart + 1;

  if(size < 0){
    Serial.write(0x02); // invalid args
    return;
  }

  byte *data = (byte*)malloc(size*busByteSize);
  
  if(!data){
    Serial.write(0x04); // malloc failed
    return;
  }

  for(size_t addr = addrStart; addr <= addrEnd; addr++){
    byte out[busByteSize];

    if(!readAndValidate(addr,out)){
      Serial.write(0x03); //read failed
      return;
    }

    memcpy(data + (addr - addrStart) * busByteSize, out, busByteSize);
  }

  Serial.write(0x00); // success

  writeBytes(data, size * busByteSize);

  free(data);
}