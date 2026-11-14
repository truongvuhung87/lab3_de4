#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ================= 1. ĐỊNH NGHĨA CHÂN KẾT NỐI (KHỚP MẠCH WOKWI) =================
#define TRIG_PIN          5    // Chân TRIG cảm biến HC-SR04
#define ECHO_PIN          18   // Chân ECHO cảm biến HC-SR04
#define ONE_WIRE_BUS      4    // Chân DATA cảm biến nhiệt độ DS18B20

#define PUMP_RELAY_PIN    25   // Relay 1 (Phía trên): Đóng/ngắt nguồn Máy bơm
#define PUMP_PWM_LED_PIN  26   // LED 1 (Bên trái): Băm xung PWM hiển thị 50% / 100% công suất Máy bơm

#define VALVE_RELAY_PIN   27   // Relay 2 (Phía dưới): Đóng/mở Van thoát
#define VALVE_LED_PIN     14   // LED 2 (Bên phải): Đèn báo trạng thái Van thoát đang mở

// ================= 2. CẤU HÌNH WIFI & MQTT BROKER =================
const char* ssid            = "Wokwi-GUEST";
const char* password        = "";
const char* mqtt_server     = "broker.emqx.io";
const int   mqtt_port       = 1883;

// MQTT Topic dành cho Đề 4
const char* topic_telemetry = "utc/iot_lab/de4/telemetry";
const char* topic_control   = "utc/iot_lab/de4/control";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

// ================= 3. CẤU TRÚC DỮ LIỆU & HÀNG ĐỢI FREERTOS =================
typedef struct {
  float waterLevel;   // Mực nước (m)
  float temperature;  // Nhiệt độ (°C)
} SensorData_t;

QueueHandle_t dataQueue; // Hàng đợi gửi dữ liệu sang Task truyền data (Core 0)
QueueHandle_t dataAlert; // Hàng đợi gửi mực nước sang Task cảnh báo (Core 1)

volatile bool  isManualMode       = false;
volatile int   currentPumpPercent = 0;
volatile bool  currentValveState  = false;
volatile float latestWaterLevel   = 1.27;
volatile float latestTemperature  = 22.0;

// ================= 4. HÀM ĐIỀU KHIỂN CƠ CẤU CHẤP HÀNH =================
void setPumpPower(int percent) {
  currentPumpPercent = constrain(percent, 0, 100);
  int dutyCycle = map(currentPumpPercent, 0, 100, 0, 255);
  
  // Xuất xung PWM ra LED 1 (GPIO 26) hiển thị độ sáng 50% hoặc 100%
  analogWrite(PUMP_PWM_LED_PIN, dutyCycle);
  
  // Đóng/ngắt Relay 1 (GPIO 25)
  digitalWrite(PUMP_RELAY_PIN, (currentPumpPercent > 0) ? HIGH : LOW);
}

void setValveState(bool open) {
  currentValveState = open;
  
  // Kích hoạt đồng thời Relay 2 (GPIO 27) và LED 2 (GPIO 14)
  digitalWrite(VALVE_RELAY_PIN, open ? HIGH : LOW);
  digitalWrite(VALVE_LED_PIN,   open ? HIGH : LOW);
}

// Hàm đồng bộ ngay trạng thái mới lên Web sau khi bấm nút để không bị nhảy trạng thái
void publishStateNow() {
  if (!mqttClient.connected()) return;
  char jsonPayload[256];
  snprintf(jsonPayload, sizeof(jsonPayload),
           "{\"Mực nước\":%.2f,\"Nhiệt độ\":%.2f,\"Máy bơm\":\"%s\",\"Van thoát\":\"%s\",\"Chế độ\":\"%s\"}",
           latestWaterLevel,
           latestTemperature,
           currentPumpPercent == 0 ? "ĐÓNG" : (currentPumpPercent == 50 ? "50%" : "100%"),
           currentValveState ? "MỞ" : "ĐÓNG",
           isManualMode ? "THỦ CÔNG" : "TỰ ĐỘNG");
  mqttClient.publish(topic_telemetry, jsonPayload);
}

// ================= 5. HÀM NHẬN LỆNH ĐIỀU KHIỂN TỪ WEB QUA MQTT =================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  if (message == "MODE_AUTO") {
    isManualMode = false;
    Serial.println("[MQTT Điều khiển] Nhận lệnh từ Web: Chuyển về chế độ Tự động");
    Serial.println("-> Chế độ: TỰ ĐỘNG");
    float wl = latestWaterLevel;
    xQueueSend(dataAlert, &wl, 0);
  } 
  else if (message == "MODE_MANUAL") {
    isManualMode = true;
    Serial.println("[MQTT Điều khiển] Nhận lệnh từ Web: Chuyển sang chế độ Thủ công");
    Serial.println("-> Chế độ: THỦ CÔNG");
  }
  else if (message == "PUMP_50") {
    isManualMode = true;
    setPumpPower(50);
    Serial.println("[MQTT Điều khiển] Nhận lệnh từ Web: Bật Máy bơm 50% công suất");
    Serial.println("-> [Chế độ: THỦ CÔNG] Máy bơm: BẬT (50%)");
  }
  else if (message == "PUMP_ON") {
    isManualMode = true;
    setPumpPower(100);
    Serial.println("[MQTT Điều khiển] Nhận lệnh từ Web: Bật Máy bơm 100% công suất");
    Serial.println("-> [Chế độ: THỦ CÔNG] Máy bơm: BẬT (100%)");
  }
  else if (message == "PUMP_OFF") {
    isManualMode = true;
    setPumpPower(0);
    Serial.println("[MQTT Điều khiển] Nhận lệnh từ Web: Đóng Máy bơm");
    Serial.println("-> [Chế độ: THỦ CÔNG] Máy bơm: ĐÓNG");
  }
  else if (message == "VALVE_ON") {
    isManualMode = true;
    setValveState(true);
    Serial.println("[MQTT Điều khiển] Nhận lệnh từ Web: Mở Van thoát");
    Serial.println("-> [Chế độ: THỦ CÔNG] Van thoát: MỞ");
  }
  else if (message == "VALVE_OFF") {
    isManualMode = true;
    setValveState(false);
    Serial.println("[MQTT Điều khiển] Nhận lệnh từ Web: Đóng Van thoát");
    Serial.println("-> [Chế độ: THỦ CÔNG] Van thoát: ĐÓNG");
  }

  // Gửi phản hồi ngay lập tức về Web để giữ đúng mức 50% / 100% vừa chọn
  publishStateNow();
}

// ================= 6. TASK ĐỌC DATA (CORE 1) =================
void TaskReadData(void *pvParameters) {
  SensorData_t sensorData;

  for (;;) {
    // 1. Đọc khoảng cách từ cảm biến HC-SR04
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    float distance_cm = duration * 0.0343 / 2.0;
    float distance_m  = distance_cm / 100.0; // Đổi sang đơn vị mét (m)

    // 2. Đọc nhiệt độ từ cảm biến DS18B20
    tempSensor.requestTemperatures();
    float tempC = tempSensor.getTempCByIndex(0);
    if (tempC == DEVICE_DISCONNECTED_C) {
      tempC = 25.0;
    }

    latestWaterLevel       = distance_m;
    latestTemperature      = tempC;
    sensorData.waterLevel  = distance_m;
    sensorData.temperature = tempC;

    // 3. Hiển thị tiếng Việt qua giao tiếp Serial
    Serial.println("--------------------------------------------------------------------------------");
    Serial.printf("[Task đọc data - Core %d] Mực nước: %.2f m | Nhiệt độ: %.2f °C | Máy bơm: %s | Van thoát: %s | Chế độ: %s\n",
                  xPortGetCoreID(),
                  sensorData.waterLevel,
                  sensorData.temperature,
                  currentPumpPercent == 0 ? "ĐÓNG" : (currentPumpPercent == 50 ? "50%" : "100%"),
                  currentValveState ? "MỞ" : "ĐÓNG",
                  isManualMode ? "THỦ CÔNG" : "TỰ ĐỘNG");

    // 4. Gửi dữ liệu vào 2 hàng đợi
    xQueueSend(dataQueue, &sensorData, pdMS_TO_TICKS(100));
    if (!isManualMode) {
      xQueueSend(dataAlert, &distance_m, pdMS_TO_TICKS(100));
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ================= 7. TASK CẢNH BÁO (CORE 1) - THỜI GIAN 10s - 20s - 10s =================
void TaskAlert(void *pvParameters) {
  float waterLevel_m = 0.0;

  for (;;) {
    if (xQueueReceive(dataAlert, &waterLevel_m, portMAX_DELAY) == pdPASS) {
      if (isManualMode) {
        continue;
      }

      // 1. Mực nước 1.0m - 1.5m: Bật 50% công suất Máy bơm trong 10s (50 lần x 200ms)
      if (waterLevel_m >= 1.0 && waterLevel_m < 1.5) {
        Serial.printf("[Task cảnh báo - Core %d] Mực nước: %.2fm (1.0m - 1.5m) -> Bật Máy bơm 50%% trong 10s\n",
                      xPortGetCoreID(), waterLevel_m);
        setValveState(false);
        setPumpPower(50);

        for (int i = 0; i < 50 && !isManualMode; i++) {
          vTaskDelay(pdMS_TO_TICKS(200));
        }
        if (!isManualMode) {
          setPumpPower(0);
          Serial.println("[Task cảnh báo] Đã đủ 10s -> Đóng Máy bơm.");
        }
        xQueueReset(dataAlert);
      }
      // 2. Mực nước 1.5m - 2.0m: Bật 100% công suất Máy bơm trong 20s (100 lần x 200ms)
      else if (waterLevel_m >= 1.5 && waterLevel_m <= 2.0) {
        Serial.printf("[Task cảnh báo - Core %d] Mực nước: %.2fm (1.5m - 2.0m) -> Bật Máy bơm 100%% trong 20s\n",
                      xPortGetCoreID(), waterLevel_m);
        setValveState(false);
        setPumpPower(100);

        for (int i = 0; i < 100 && !isManualMode; i++) {
          vTaskDelay(pdMS_TO_TICKS(200));
        }
        if (!isManualMode) {
          setPumpPower(0);
          Serial.println("[Task cảnh báo] Đã đủ 20s -> Đóng Máy bơm.");
        }
        xQueueReset(dataAlert);
      }
      // 3. Mực nước < 0.5m: Mở Van thoát trong 10s (50 lần x 200ms)
      else if (waterLevel_m < 0.5) {
        Serial.printf("[Task cảnh báo - Core %d] Mực nước: %.2fm (< 0.5m) -> Mở Van thoát trong 10s\n",
                      xPortGetCoreID(), waterLevel_m);
        setPumpPower(0);
        setValveState(true);

        for (int i = 0; i < 50 && !isManualMode; i++) {
          vTaskDelay(pdMS_TO_TICKS(200));
        }
        if (!isManualMode) {
          setValveState(false);
          Serial.println("[Task cảnh báo] Đã đủ 10s -> Đóng Van thoát.");
        }
        xQueueReset(dataAlert);
      }
      // 4. Trong ngưỡng bình thường (0.5m - 1.0m)
      else {
        if (!isManualMode) {
          setPumpPower(0);
          setValveState(false);
        }
      }
    }
  }
}

// ================= 8. TASK TRUYỀN DATA (CORE 0) =================
void TaskTransmitData(void *pvParameters) {
  SensorData_t receivedData;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password, 6); // Chỉ định kênh 6 cho mạng Wokwi-GUEST
  Serial.printf("[Task truyền data - Core %d] Đang kết nối WiFi", xPortGetCoreID());
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }
  Serial.println("\n[Task truyền data] Đã kết nối WiFi!");

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512); // Đảm bảo gửi trọn vẹn gói JSON tiếng Việt

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password, 6);
      vTaskDelay(pdMS_TO_TICKS(5000));
      continue;
    }

    if (!mqttClient.connected()) {
      String clientId = "ESP32_De4_" + String(random(0xffff), HEX);
      if (mqttClient.connect(clientId.c_str())) {
        Serial.println("[Task truyền data] Đã kết nối MQTT Broker!");
        mqttClient.subscribe(topic_control);
      } else {
        vTaskDelay(pdMS_TO_TICKS(2000));
        continue;
      }
    }

    mqttClient.loop();

    if (xQueueReceive(dataQueue, &receivedData, pdMS_TO_TICKS(100)) == pdPASS) {
      char jsonPayload[256];
      snprintf(jsonPayload, sizeof(jsonPayload),
               "{\"Mực nước\":%.2f,\"Nhiệt độ\":%.2f,\"Máy bơm\":\"%s\",\"Van thoát\":\"%s\",\"Chế độ\":\"%s\"}",
               receivedData.waterLevel,
               receivedData.temperature,
               currentPumpPercent == 0 ? "ĐÓNG" : (currentPumpPercent == 50 ? "50%" : "100%"),
               currentValveState ? "MỞ" : "ĐÓNG",
               isManualMode ? "THỦ CÔNG" : "TỰ ĐỘNG");

      mqttClient.publish(topic_telemetry, jsonPayload);
      Serial.printf("[Task truyền data - Core %d] Gửi MQTT:\n%s\n", xPortGetCoreID(), jsonPayload);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= 9. SETUP & LOOP =================
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(PUMP_PWM_LED_PIN, OUTPUT);
  pinMode(VALVE_RELAY_PIN, OUTPUT);
  pinMode(VALVE_LED_PIN, OUTPUT);

  setPumpPower(0);
  setValveState(false);

  tempSensor.begin();
  tempSensor.setResolution(9); // Đọc nhanh cảm biến nhiệt độ DS18B20

  dataQueue = xQueueCreate(10, sizeof(SensorData_t));
  dataAlert = xQueueCreate(10, sizeof(float));

  if (dataQueue != NULL && dataAlert != NULL) {
    xTaskCreatePinnedToCore(TaskTransmitData, "TaskTransmitData", 8192, NULL, 1, NULL, 0); // Core 0
    xTaskCreatePinnedToCore(TaskReadData,     "TaskReadData",     4096, NULL, 2, NULL, 1); // Core 1
    xTaskCreatePinnedToCore(TaskAlert,        "TaskAlert",        4096, NULL, 2, NULL, 1); // Core 1
  }
}

void loop() {
  vTaskDelete(NULL);
}