#include <Arduino.h>
#include <SoftwareSerial.h>         
#include <SerialCommand.h>                  // https://github.com/db-electronics/ArduinoSerialCommand
#include <USBSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <STM32SD.h>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_gpio.h>

#include "umdBoardDefinitions.h"
#include "i2cScanner.h"
#include "mcp23008.h"
#include "umdDisplay.h"

#ifndef SD_DETECT_PIN
#define SD_DETECT_PIN PD0
#endif  

SdFatFs fatFs;
SerialCommand SCmd;
MCP23008 onboardMCP23008;
MCP23008 adapterMCP23008;
UMDDisplay umdDisplay;

void scmdScanI2C(void);
void inputInterrupt(void);

uint8_t inputs;
bool newInputs;

unsigned long previousTicks;
unsigned long umdTicks = 0;
int displayYOffset = 1;

// I2C display
// https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/

Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET);

// HAL Code in here? https://www.stm32duino.com/viewtopic.php?t=82

void setup() {

  // setup USB serial
  // https://primalcortex.wordpress.com/2020/10/11/stm32-blue-pill-board-arduino-core-and-usb-serial-output-on-platformio/
  SerialUSB.begin(460800);  // no need for a parameter here maybe?
  pinMode(PA10, OUTPUT);
  digitalWrite(PA10, LOW);
  delay(10);
  digitalWrite(PA10, HIGH);
  SerialUSB.println(F("UMDv3"));

  // setup I2C
  // https://github.com/stm32duino/Arduino_Core_STM32/wiki/API#i2c
  // Wire.setSCL(PB8);
  // Wire.setSDA(PB9);
  Wire.begin();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) { 
    SerialUSB.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text

  delay(1000);
  umdDisplay.begin(&display);
  umdDisplay.clear();
  umdDisplay.print("/UMDv3/", 0);
  umdDisplay.redraw();

  // sd card
  umdDisplay.print("init SD card", 1, 2);
  umdDisplay.redraw();
  SD.setDx(PC8, PC9, PC10, PC11);
  SD.setCMD(PD2);
  SD.setCK(PC12);
  if(!SD.begin(SD_DETECT_PIN)){
    umdDisplay.print(" failed", 1);
    while(1);
  }

  // setup onboard mcp23008, GP6 and GP7 LED outputs
  umdDisplay.print("init MCP23008", 2, 2);
  umdDisplay.redraw();

  if(!onboardMCP23008.begin(UMD_BOARD_MCP23008_ADDRESS)){
    SerialUSB.println(F("onboard MCP23008 error"));
    //display.println(F("onboard MCP23008 error"));
    for(;;); // Don't proceed, loop forever
  }

  onboardMCP23008.pinMode(UMD_BOARD_LEDS, OUTPUT);
  onboardMCP23008.setPullUpResistors(UMD_BOARD_PUSHBUTTONS, true);
  onboardMCP23008.setDefaultValue(UMD_BOARD_PUSHBUTTONS, HIGH);
  onboardMCP23008.setInterruptControl(UMD_BOARD_PUSHBUTTONS, onboardMCP23008.PREVIOUS);
  onboardMCP23008.setInterruptOnChange(UMD_BOARD_PUSHBUTTONS, true);
  onboardMCP23008.setInterruptEnable(UMD_BOARD_PUSHBUTTONS, true);
  onboardMCP23008.digitalWrite(UMD_BOARD_LEDS, LOW);
  newInputs = false;
  inputs = onboardMCP23008.readGPIO();
  // interrupt line tied to PD1
  pinMode(UMD_MCP23008_INTERRUPT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(UMD_MCP23008_INTERRUPT_PIN), inputInterrupt, FALLING);

  // setup adapter mcp23008, read adapter id
  if(!adapterMCP23008.begin(UMD_ADAPTER_MCP23008_ADDRESS)){
    SerialUSB.println(F("adapter MCP23008 error"));
    for(;;); // Don't proceed, loop forever
  }

  adapterMCP23008.pinMode(0xFF, INPUT);
  uint8_t adapterId = adapterMCP23008.readGPIO();
  umdDisplay.print("adapter id = 0x01", 3, 2);
  umdDisplay.redraw();

  //register callbacks for SerialCommand related to the cartridge
  SCmd.addCommand("scani2c", scmdScanI2C);

  previousTicks = HAL_GetTick();
  // uint32_t currentTicks = HAL_GetTick();
  // while(HAL_GetTick() < (currentTicks + 5000));

  // fill display buffer for scroll test
  for(int i = 4; i<32; i++){
    umdDisplay.print("line ", i);
    umdDisplay.print(i, i);
  }
}

void loop() {

  // Reminder: when debugging ticks isn't accurate at all
  uint32_t currentTicks = HAL_GetTick();
  if( (currentTicks - previousTicks) < 250){
    return;
  }
  previousTicks = currentTicks;
  umdTicks++;

  if(umdTicks & 0x01){
    onboardMCP23008.tooglePins(MCP23008_GP7 | MCP23008_GP6);
    umdDisplay.scrollRotateDown(1, 1);
    //displayYOffset = -displayYOffset;
  }else{
    onboardMCP23008.tooglePins(MCP23008_GP7);
  }

  if(newInputs)
  {
    newInputs = false;
    inputs = onboardMCP23008.readInterruptCapture();
    SerialUSB.print(F("inputs = 0x"));
    SerialUSB.println(inputs, HEX);
  }

  umdDisplay.redraw();
}


void scmdScanI2C(void)
{
    // char *arg;
    // uint32_t address, value;
    
    // this is the address to poke
    // arg = SCmd.next();
    // address = (uint32_t)strtoul(arg, (char**)0, 0);

    // value = *(__IO uint32_t *)(address);
    // SerialUSB.print(value, HEX);
    I2CScanner scanner;
    std::vector<uint8_t> addresses;
    addresses = scanner.findDevices(&Wire);

    for (uint8_t address : addresses)
    {
      SerialUSB.println(address,HEX);
    }

}

void inputInterrupt(void)
{
  newInputs = true;
}