#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MLX90614.h>
// ==================== เพิ่มของ Cloud ของ Firebase ====================
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
// ต้องมี 2 บรรทัดนี้เสมอสำหรับการใช้ Token ของ Mobizt
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ==================== LCD & Sensor ====================
LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// ==================== Ultrasonic ====================
const int pingPin     = 5;
const int inPin       = 18;
const int BASE_HEIGHT = 200;

// ==================== BLE Configuration ====================
#define MY_SCALE_MAC        "70:87:9E:98:B1:D6"
#define WEIGHT_SERVICE_UUID "0000181d-0000-1000-8000-00805f9b34fb"
#define WEIGHT_CHAR_UUID    "00002a9d-0000-1000-8000-00805f9b34fb"
#define MI_SERVICE_UUID     "0000181b-0000-1000-8000-00805f9b34fb"
#define MI_WEIGHT_CHAR_UUID "00002a9c-0000-1000-8000-00805f9b34fb"
#define MLX_SDA 25
#define MLX_SCL 26

// ==================== PulseSensor ====================
#define PULSE_PIN      34
#define THRESHOLD      2048
#define HYSTERESIS     150
#define MIN_MS         300
#define MAX_MS         1500
#define AVERAGE_SIZE   5

// ==================== ตั้งค่า WiFi & Firebase ====================
#define WIFI_SSID "TP-Link_8054"
#define WIFI_PASSWORD "18224818"
#define API_KEY "AIzaSyDunuaEB9WxsQyLRHoH1ro-dYzb9g9nuek"
#define DATABASE_URL "healthcare-3fc3b-default-rtdb.asia-southeast1.firebasedatabase.app"

// ตัวแปรสำหรับใช้งาน Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

int beatTimes[AVERAGE_SIZE];
int beatIndex        = 0;
bool beatFull        = false;
unsigned long lastBeatTime = 0;
bool aboveThreshold  = false;
int lastBPM          = 0;

// ==================== Global Variables ====================
bool isWiFiInitiated = false; // ธงสำหรับเช็คว่าเริ่มเชื่อม WiFi หรือยัง
bool needUpdateLCD = false;   // ตัวแปรสำหรับบอกให้บอร์ดรู้ว่าต้องอัปเดตจอ
float lastWeight = 0.0;
long  lastHeight = 0;
float lastTemp   = 0.0;
String firebasePath = "/healthcare/latest";
unsigned long lastFirebaseUpdateTime = 0;
const unsigned long FIREBASE_UPDATE_INTERVAL = 5000; // 5 seconds


// ==================== Global BLE ====================
static BLEAdvertisedDevice* myDevice = nullptr;
static BLEClient* pClient            = nullptr;
static boolean doConnect             = false;
static boolean connected             = false;
static boolean doScan                = false;
unsigned long lastDataTime           = 0;
const unsigned long DEBOUNCE_TIME    = 1000;

// ==================== BPM Functions ====================
int getAverageBPM() {
    int count = beatFull ? AVERAGE_SIZE : beatIndex;
    if (count == 0) return 0;
    long sum = 0;
    for (int i = 0; i < count; i++) sum += beatTimes[i];
    return 60000 / (sum / count);
}

void readPulse() {
    int signal = analogRead(PULSE_PIN);
    unsigned long now = millis();

    if (signal > THRESHOLD && !aboveThreshold) {
        aboveThreshold = true;
        unsigned long interval = now - lastBeatTime;
        if (interval > MIN_MS && interval < MAX_MS) {
            beatTimes[beatIndex] = interval;
            beatIndex = (beatIndex + 1) % AVERAGE_SIZE;
            if (beatIndex == 0) beatFull = true;
        }
        lastBeatTime = now;
    }

    if (signal < THRESHOLD - HYSTERESIS && aboveThreshold) {
        aboveThreshold = false;
    }

    int bpm = getAverageBPM();
    if (bpm > 0) lastBPM = bpm;
}

// ==================== Update LCD ====================
void updateLCD() {
    // Row 0: Temperature
    lcd.setCursor(0, 0);
    lcd.print("Temp  :");
    if (lastTemp > 0) {
        char buf[10];
        dtostrf(lastTemp, 4, 1, buf);
        lcd.print(buf);
        lcd.print((char)223);
        lcd.print("C   ");
    } else {
        lcd.print("--.-");
        lcd.print((char)223);
        lcd.print("C   ");
    }

    // Row 1: Weight
    lcd.setCursor(0, 1);
    lcd.print("Weight:");
    if (lastWeight > 0) {
        lcd.print(lastWeight, 2);
        lcd.print("kg  ");
    } else {
        lcd.print("--.-kg  ");
    }

    // Row 2: Height
    lcd.setCursor(0, 2);
    lcd.print("Height:");
    if (lastHeight > 0) {
        lcd.print(lastHeight);
        lcd.print("cm   ");
    } else {
        lcd.print("---cm  ");
    }

    // Row 3: BPM
    lcd.setCursor(0, 3);
    lcd.print("BPM   :");
    if (lastBPM > 0) {
        lcd.print(lastBPM);
        lcd.print("bpm   ");
    } else {
        lcd.print("---bpm  ");
    }
}
// ==================== ปรับปรุงฟังก์ชันส่งข้อมูล (เช็ค WiFi ก่อนส่ง) ====================


// ==================== BLE Notify ====================
static void notifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData, size_t length, bool isNotify) {

    unsigned long currentTime = millis();
    if ((currentTime - lastDataTime) < 200) return;

    if (length >= 10) {
        uint8_t controlByte = pData[0];
        bool hasStabilized  = (controlByte & 0x20) != 0;
        bool isLbs          = (controlByte & 0x01) != 0;

        uint16_t weightRaw = pData[1] | (pData[2] << 8);
        float weight = weightRaw / (isLbs ? 100.0 : 200.0);
        if (isLbs) weight *= 0.453592;

        if (weight > 0) {
            lastWeight   = weight;
            lastDataTime = currentTime;
            needUpdateLCD = true; // อัปเดตจอน้ำหนักทันที
        }

        if (hasStabilized) {
            Serial.printf("✅ Weight STABLE: %.2f kg\n", weight);

        }
    }
}

// ==================== Client Callback ====================
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        connected = true;
        Serial.println("✅ BLE Connected!");
    }
    void onDisconnect(BLEClient* pclient) {
        connected = false;
        doScan    = true;
        Serial.println("❌ BLE Disconnected");
    }
};

// ==================== Connect Function ====================
bool connectToServer() {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    if (!pClient->connect(myDevice)) return false;

    BLERemoteService* pRemoteService = pClient->getService(BLEUUID(MI_SERVICE_UUID));
    if (!pRemoteService)
        pRemoteService = pClient->getService(BLEUUID(WEIGHT_SERVICE_UUID));
    if (!pRemoteService) { pClient->disconnect(); return false; }

    BLERemoteCharacteristic* pChar = pRemoteService->getCharacteristic(BLEUUID(MI_WEIGHT_CHAR_UUID));
    if (!pChar)
        pChar = pRemoteService->getCharacteristic(BLEUUID(WEIGHT_CHAR_UUID));
    if (!pChar) { pClient->disconnect(); return false; }

    if (pChar->canNotify() || pChar->canIndicate()) {
        pChar->registerForNotify(notifyCallback);
    } else { pClient->disconnect(); return false; }

    connected = true;
    return true;
}

// ==================== Scan Callback ====================
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String currentAddress = advertisedDevice.getAddress().toString().c_str();
        currentAddress.toUpperCase();
        String targetAddress = MY_SCALE_MAC;
        targetAddress.toUpperCase();

        if (currentAddress == targetAddress) {
            BLEDevice::getScan()->stop();
            myDevice  = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan    = false;
            Serial.println("🔵 Scale Found!");
        }
    }
};

// ==================== Read Height ====================
long readHeight() {
    pinMode(pingPin, OUTPUT);
    digitalWrite(pingPin, LOW);  delayMicroseconds(2);
    digitalWrite(pingPin, HIGH); delayMicroseconds(5);
    digitalWrite(pingPin, LOW);
    pinMode(inPin, INPUT);
    
    long duration = pulseIn(inPin, HIGH, 30000); 
    if (duration == 0) return 0; 

    long dist     = duration / 29 / 2;
    long height   = BASE_HEIGHT - dist;

    if (height > 50 && height < 250) return height;
    return 0;
}

// ==================== Read Temp ====================
float readTemp() {
    float ambient = mlx.readAmbientTempC();
    float object  = mlx.readObjectTempC();

    if (isnan(object) || object < -40.0) {
        return lastTemp;
    }

    float temp = object + 2.0;
    if (temp >= 20.0 && temp <= 45.0) return temp;
    return lastTemp;
}

// ==================== ฟังก์ชันต่อ WiFi แบบมี Timeout (ป้องกันจอค้าง) ====================
void connectWiFiAndFirebase() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scale Linked!");
    lcd.setCursor(0, 1);
    lcd.print("Connecting WiFi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startAttemptTime = millis();
    // ให้เวลาพยายามเชื่อมต่อ 15 วินาที ถ้าไม่ได้ให้ข้ามไปก่อนเพื่อไม่ให้เครื่องค้าง
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        lcd.clear();
        lcd.print("WiFi Connected!");
        
        // ตั้งค่า Firebase
        config.api_key = API_KEY;
        config.database_url = DATABASE_URL;
        if (Firebase.signUp(&config, &auth, "", "")) {
            Serial.println("✅ Firebase Auth Ready");
        }
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        
        delay(1000);
    } else {
        Serial.println("\n❌ WiFi Timeout!");
        lcd.clear();
        lcd.print("WiFi Timeout!");
        lcd.setCursor(0,1);
        lcd.print("Check Router...");
        delay(2000);
    }

    lcd.clear();
    updateLCD();
}

// ==================== Send Data to Firebase ====================
void sendDataToFirebase() {
    if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
        FirebaseJson json;
        json.set("weight", lastWeight);
        json.set("height", lastHeight);
        json.set("temp", lastTemp);
        json.set("bpm", lastBPM);
        json.set("timestamp/.sv", "timestamp"); // Firebase Server Timestamp

        Serial.println("Pushing data to Firebase...");
        if (Firebase.RTDB.setJSON(&fbdo, firebasePath.c_str(), &json)) {
            Serial.println("✅ Data sent successfully");
        } else {
            Serial.printf("❌ Failed to send data: %s\n", fbdo.errorReason().c_str());
        }
    }
}

// ==================== Setup ====================
void setup() {
    Serial.begin(115200);
    analogReadResolution(12);

    Wire.begin();
    Wire1.begin(MLX_SDA, MLX_SCL);
    lcd.init();
    lcd.backlight();
    lcd.clearWriteError();
    lcd.setCursor(0, 0);
    lcd.print("1. Scanning Scale..."); // โชว์ว่ากำลังหาสเกล

    if (!mlx.begin(0x5A, &Wire1)) {
        Serial.println("❌ MLX90614 Not Found");
        lcd.setCursor(0, 1);
        lcd.print("MLX ERROR!");
        while (1);
    }

    // เริ่มสแกน BLE ทันที โดยยังไม่ต่อ WiFi
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
    doScan = true;
}

// ==================== Loop ====================
void loop() {
    // 1. จัดการการเชื่อมต่อ BLE
    if (doConnect) {
        connectToServer();
        doConnect = false;
    }
    
    if (doScan) {
        BLEDevice::getScan()->start(2, false);
        doScan = false;
    }

    // 2. ถ้าต่อบลูทูธเสร็จแล้ว และยังไม่ได้ต่อ WiFi -> ให้เริ่มต่อ WiFi ทันที
    if (connected && !isWiFiInitiated) {
        connectWiFiAndFirebase();
        isWiFiInitiated = true; // เปลี่ยนธงว่าเชื่อมต่อแล้ว
    }

    // 3. ถ้าบลูทูธหลุด หรือหาไม่เจอ และยังไม่ได้เริ่ม WiFi -> ให้สแกนหาเรื่อยๆ ทุก 10 วิ
    if (!connected && !doConnect && !isWiFiInitiated) {
        static unsigned long lastScanTime = 0;
        if (millis() - lastScanTime > 10000) {
            lastScanTime = millis();
            doScan = true; 
        }
    }

    // 4. อ่านค่าเซ็นเซอร์
    readPulse();

    unsigned long currentMillis = millis();

    // 5. อัปเดตเซ็นเซอร์และหน้าจอทุกๆ ครึ่งวินาที (500 ms)
    static unsigned long lastSensorReadTime = 0;
    if (currentMillis - lastSensorReadTime >= 500) {
        lastSensorReadTime = currentMillis;

        long h = readHeight();
        if (h > 0) lastHeight = h;

        float t = readTemp();
        if (t != lastTemp) {
            lastTemp = t;
        }

        // ✅ เรียกใช้อัปเดตหน้าจอตรงนี้ เพื่อให้ทุกค่าโชว์ตลอดเวลา
        if (isWiFiInitiated) { // อัปเดตจอเฉพาะตอนที่หน้าจอพร้อมแล้ว (ผ่านหน้าตั้งค่า WiFi มาแล้ว)
            updateLCD(); 
        }
    }

    // 6. เช็คธงการอัปเดตหน้าจอแบบ Real-time (สำหรับน้ำหนัก)
    if (needUpdateLCD && isWiFiInitiated) {
        updateLCD();          
        needUpdateLCD = false; 
    }

    // 7. Print BPM
    static unsigned long lastPrintTime = 0;
    if (currentMillis - lastPrintTime >= 1000) {
        lastPrintTime = currentMillis;
        if (lastBPM > 0) {
            Serial.printf("💓 BPM: %d\n", lastBPM);
        }
    }

    // 8. ส่งข้อมูลขึ้น Firebase ทุกๆ 5 วินาที
    if (isWiFiInitiated && currentMillis - lastFirebaseUpdateTime >= FIREBASE_UPDATE_INTERVAL) {
        lastFirebaseUpdateTime = currentMillis;
        sendDataToFirebase();
    }

    delay(5); 
}