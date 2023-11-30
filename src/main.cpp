/*
*******************************************************************************
* Copyright (c) 2021 by M5Stack
*                  Equipped with M5StickC sample source code
*                          配套  M5StickC 示例源代码
* Visit for more information: https://docs.m5stack.com/en/core/m5stickc
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/core/m5stickc
*
* Product:  Angle.  角度计
* Date: 2021/8/9
*******************************************************************************
  Description: Please connect to Port,Read the Angle of the angometer and
convert it to digital display 请连接端口,读取角度计的角度，并转换为数字量显示
*/

#include <M5StickC.h>

#include "BLEDevice.h"

#define BUTTON 37

TaskHandle_t TaskHandle_buttonRead;
TaskHandle_t TaskHandle_Led;

int buttonLastState = HIGH;
int buttonCurrentState;
int buttonCount = 1;
bool forwardCommand = false;
bool backwardCommand = false;
bool direction = false;
int sensorPin =
    33;  // set the input pin for the potentiometer.  设置角度计的输入引脚

int last_sensorValue =
    100;  // Stores the value last read by the sensor.  存储传感器上次读取到的值
int cur_sensorValue = 0;  // Stores the value currently read by the sensor.
                          // 存储传感器当前读取到的值


// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID charUUIDSpeed("beb5483f-36e1-4688-b7f5-ea07361b26a8");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristicSpeed;
static BLEAdvertisedDevice* myDevice;


void buttonRead(void* pvParameters) {
  while (true) {
    buttonCurrentState = digitalRead(BUTTON);
    if (buttonLastState == LOW && buttonCurrentState == HIGH) {
      Serial.println("Button Pressed!");
      if (direction == false) {
        forwardCommand = true;
        direction = true;
      } else {
        backwardCommand = true;
        direction = false;
      }
      // pRemoteCharacteristic->setValue("forward");
      // pRemoteCharacteristic->notify();
      buttonCount++;
      if (buttonCount > 6) {
        buttonCount = 1;
      }
    }
    buttonLastState = buttonCurrentState;
    vTaskDelay(10 / portTICK_RATE_MS);
  }
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");


  // Obtain a reference to the characteristic in the service of the remote BLE server.


  
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    pRemoteCharacteristicSpeed = pRemoteService->getCharacteristic(charUUIDSpeed);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

   if (pRemoteCharacteristicSpeed == nullptr) {
    Serial.print("Failed to find our speed characteristic UUID: ");
    Serial.println(charUUIDSpeed.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our speed characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

   if (pRemoteCharacteristicSpeed->canRead()) {
    std::string value = pRemoteCharacteristicSpeed->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}
/**
* Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }  // Found our server
  }    // onResult
};     // MyAdvertisedDeviceCallbacks

void setup() {
    M5.begin();    
    Serial.begin(115200);         // Init M5StickC.  初始化 M5StickC
    //M5.Lcd.setRotation(2);  // Rotate the screen.  旋转屏幕
    M5.Lcd.setTextSize(1);  // Set the font size to 2.  设置字体大小为2
    pinMode(
        sensorPin,
        INPUT);  // Sets the specified pin to input mode. 设置指定引脚为输入模式
    M5.Lcd.print("the value of ANGLE: ");

    pinMode(BUTTON, INPUT_PULLUP);

  xTaskCreate(buttonRead, "buttonRead", 2048 * 1, nullptr, 128 * 10, &TaskHandle_buttonRead);


  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {

  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // String newValue = "Time since boot: " + String(millis()/1000);



    // Set the characteristic's value to be the array of bytes that is actually a string.
    if (forwardCommand == true) {
      String newValue = "forward";

      Serial.println("Setting new characteristic value to \"" + newValue + "\"");
      //     String newValue = "forward";
      pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
      forwardCommand = false;
    }

    if (backwardCommand == true) {
      String newValue = "backward";

      Serial.println("Setting new characteristic value to \"" + newValue + "\"");
      //     String newValue = "forward";
      pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
      backwardCommand = false;
    }
    //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }



  
    cur_sensorValue = analogRead(
        sensorPin);  // read the value from the sensor.  读取当前传感器的值
    M5.Lcd.setCursor(0, 40);  // Place the cursor at (0,40).  将光标固定在(0,40)
    if (abs(cur_sensorValue - last_sensorValue) >
        10) {  // If the difference is more than 10.  如果差值超过10
        M5.Lcd.fillRect(0, 40, 100, 25, BLACK);
        M5.Lcd.print(cur_sensorValue);
        last_sensorValue = cur_sensorValue;
         Serial.println("Setting new speed characteristic value to \"" + String(cur_sensorValue) + "\"");
         String myString = String(cur_sensorValue);
      pRemoteCharacteristicSpeed->writeValue(myString.c_str(), myString.length());
    }
    delay(80);
}
