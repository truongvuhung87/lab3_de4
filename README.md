Link mô phỏng Wokwi:


https://wokwi.com/projects/480542136022290433


Link báo cáo chi tiết:


https://1drv.ms/w/c/e5ff9a7b0093a807/IQCkX35Cda36RYKKsfI3A095AfkcnL6dbx8WyKfvuVZBgLg?e=h153Pg


Link MQTT Broker:


https://mqttx.app/web-client#/recent_connections/a6d055d7-3c46-41b2-b0f0-837b71de7732


# THIẾT KẾ MẠCH TỰ ĐỘNG GIÁM SÁT THÔNG SỐ MÔI TRƯỜNG CHO BỂ NUÔI THỦY, HẢI SẢN.

* **Họ và tên:** Vũ Hùng Trường
* **Mã sinh viên:** 231402381

---

## 1. Mục tiêu bài thực hành và Các cảm biến chính sử dụng

### 1.1. Mục tiêu bài thực hành
Xây dựng và mô phỏng hoàn chỉnh hệ thống nhúng IoT tự động giám sát thông số môi trường (mực nước, nhiệt độ) và điều khiển cơ cấu chấp hành cho bể nuôi thủy, hải sản theo thời gian thực[cite: 2]. Hệ thống ứng dụng vi điều khiển lõi kép ESP32 kết hợp hệ điều hành thời gian thực FreeRTOS và giao thức truyền thông MQTT nhằm đảm bảo tính ổn định, độ trễ thấp, xử lý đa nhiệm mượt mà và hỗ trợ người dùng giám sát, điều khiển từ xa qua giao diện Web/App[cite: 2].

### 1.2. Các cảm biến chính sử dụng[cite: 2]

#### a) Cảm biến siêu âm HC-SR04[cite: 2]
![Cảm biến siêu âm HC-SR04](images/hc-sr04.png)

* HC-SR04 là cảm biến đo khoảng cách bằng sóng siêu âm[cite: 2]. Trong mô hình, cảm biến được giả định đặt phía trên bể và hướng xuống mặt nước, cho phép đo mà không cần tiếp xúc trực tiếp với nước[cite: 2].
* ESP32 phát một xung kích hoạt đến chân `TRIG`[cite: 2]. Cảm biến phát sóng siêu âm và đưa ra xung tại chân `ECHO` có độ rộng tương ứng với thời gian sóng truyền đến bề mặt rồi phản xạ trở lại[cite: 2].
* Trong Wokwi, khoảng cách mô phỏng của HC-SR04 có thể điều chỉnh từ 2 đến 400 cm[cite: 2]. Đơn vị khoảng cách sử dụng trong bài là centimet (cm)[cite: 2].
* Khoảng cách được tính gần đúng[cite: 2]:

$$d = \frac{t}{58}$$

Trong đó[cite: 2]:
* $d$: Khoảng cách từ cảm biến đến mặt nước, đơn vị cm[cite: 2].
* $t$: Độ rộng xung `ECHO`, đơn vị micro giây ($\mu\text{s}$)[cite: 2].

* Chiều cao mực nước được xác định theo công thức[cite: 2]:

$$h = H - d$$

Trong đó $H$ là khoảng cách từ vị trí đặt cảm biến đến đáy bể[cite: 2].

#### b) Cảm biến nhiệt độ DS18B20[cite: 2]
![Cảm biến nhiệt độ DS18B20](images/ds18b20.png)

* DS18B20 là cảm biến nhiệt độ số sử dụng giao tiếp 1-Wire, cho phép truyền dữ liệu qua một đường tín hiệu[cite: 2].
* Cảm biến có dải đo từ $-55^\circ\text{C}$ đến $+125^\circ\text{C}$, độ chính xác $\pm 0,5^\circ\text{C}$[cite: 2]. Đơn vị nhiệt độ sử dụng trong bài là độ C ($^\circ\text{C}$)[cite: 2].

---

## 2. Sơ đồ kết nối mạch trên Wokwi[cite: 2]

![Sơ đồ kết nối mạch trên Wokwi](images/wokwi_schematic.png)

**Bảng kết nối chân:**[cite: 2]

| Linh kiện | Tên chân linh kiện | Chân trên ESP32 |
| :--- | :--- | :--- |
| **Cảm biến khoảng cách HC-SR04** | VCC | 5V[cite: 2] |
| | TRIG | GPIO 5[cite: 2] |
| | ECHO | GPIO 18[cite: 2] |
| | GND | GND[cite: 2] |
| **Cảm biến nhiệt độ DS18B20** | VCC (VDD) | 3V3[cite: 2] |
| | DQ (Data) | GPIO 4[cite: 2] |
| | GND | GND[cite: 2] |
| **Relay Module 1 (Van)** | VCC | 5V[cite: 2] |
| | GND | GND[cite: 2] |
| | IN | GPIO 27[cite: 2] |
| **Relay Module 2 (Bơm)** | VCC | 5V[cite: 2] |
| | GND | GND[cite: 2] |
| | IN | GPIO 25[cite: 2] |
| **LED Chỉ thị 1 (qua điện trở)** | Chân Dương (+) | GPIO 26[cite: 2] |
| | Chân Âm (-) | GND[cite: 2] |
| **LED Chỉ thị 2 (qua điện trở)** | Chân Dương (+) | GPIO 14[cite: 2] |
| | Chân Âm (-) | GND[cite: 2] |

---

## 3. Xây dựng hệ điều hành thời gian thực FreeRTOS trên ESP32[cite: 2]

### 3.1. Kiến trúc phân bổ đa nhiệm trên 2 lõi (Dual-Core)[cite: 2]
Để đảm bảo tính thời gian thực (Real-time) và ngăn ngừa việc xử lý giao thức mạng làm gián đoạn quá trình điều khiển bể nuôi, hệ thống sử dụng hàm `xTaskCreatePinnedToCore()` của FreeRTOS trên ESP32 nhằm ghim cố định các tác vụ (Task) vào từng lõi vật lý riêng biệt[cite: 2]:

| Tên Task | Lõi thực thi | Mức ưu tiên | Nhiệm vụ thiết kế |
| :--- | :---: | :---: | :--- |
| **Task truyền data** | Core 0 | 1 | Kết nối Wi-Fi và MQTT Broker; đọc dữ liệu từ hàng đợi `data Queue` để gửi lên Broker; nhận lệnh điều khiển từ Web[cite: 2]. |
| **Task đọc data** | Core 1 | 2 | Đọc cảm biến HC-SR04 và nhiệt độ bể; hiển thị lên Serial Monitor; đẩy dữ liệu đầy đủ vào `data Queue` và đẩy mực nước vào `data Alert`[cite: 2]. |
| **Task cảnh báo** | Core 1 | 3 | Đọc dữ liệu mực nước từ `data Alert`; thực thi thuật toán điều khiển công suất Máy bơm (PWM) và Van thoát nước theo thời gian quy định[cite: 2]. |

### 3.2. Thiết kế Hàng đợi (Queue) giao tiếp giữa các Task[cite: 2]
Hệ thống sử dụng 2 hàng đợi (Queue) hoạt động theo cơ chế FIFO (First-In, First-Out) để truyền các gói dữ liệu theo khối (bản tin) và đồng bộ hóa hoạt động giữa 3 Task mà không xảy ra xung đột bộ nhớ[cite: 2]:

* **Hàng đợi `data Queue`:**[cite: 2]
  * **Mục đích:** Truyền toàn bộ thông số môi trường từ `Task đọc data` (Core 1) sang `Task truyền data` (Core 0)[cite: 2].
  * **Kiểu dữ liệu phần tử:** Một cấu trúc dữ liệu (`struct SensorData`) gồm 2 trường là `waterLevel` (kiểu `float` - mực nước tính bằng mét) và `temperature` (kiểu `float` - nhiệt độ bể tính bằng độ C)[cite: 2].
* **Hàng đợi `data Alert`:**[cite: 2]
  * **Mục đích:** Truyền riêng thông số mực nước từ `Task đọc data` (Core 1) sang `Task cảnh báo` (Core 1) để xử lý thuật toán điều khiển cơ cấu chấp hành[cite: 2].
  * **Kiểu dữ liệu phần tử:** Biến số thực (`float`) biểu diễn giá trị mực nước hiện tại (đơn vị: mét)[cite: 2].

### 3.3. Chi tiết hoạt động và thuật toán của các Task[cite: 2]

#### a) Hoạt động của "Task đọc data" (Chạy trên Core 1)[cite: 2]
* Định kỳ mỗi chu kỳ lấy mẫu, Task phát xung kích $10\ \mu\text{s}$ vào chân `TRIG` của cảm biến HC-SR04, đo thời gian phản hồi trên chân `ECHO` và tính toán ra thông số mực nước (đơn vị: m)[cite: 2].
* Tiếp theo, Task yêu cầu cảm biến DS18B20 chuyển đổi và đọc giá trị nhiệt độ nước trong bể qua bus 1-Wire[cite: 2].
* Các thông số thu thập được sẽ được xuất trực tiếp ra màn hình Serial Monitor thông qua bộ giao tiếp nối tiếp UART0 (`Serial.printf`) ở tốc độ baud `115200` để phục vụ giám sát cục bộ[cite: 2].
* Sau đó, Task đóng gói cả mực nước và nhiệt độ gửi vào hàng đợi `data Queue` bằng API `xQueueSend()`, đồng thời gửi riêng giá trị mực nước vào hàng đợi `data Alert` trước khi gọi hàm `vTaskDelay()` để chuyển sang trạng thái khóa (Blocked) chờ chu kỳ đọc tiếp theo[cite: 2].

#### b) Hoạt động của "Task cảnh báo" và Thuật toán điều khiển mực nước (Chạy trên Core 1)[cite: 2]
* Task luôn chờ bản tin từ hàng đợi `data Alert` thông qua hàm `xQueueReceive()`[cite: 2]. Khi không có dữ liệu mới trong hàng đợi, Task ở trạng thái Blocked và không tiêu tốn tài nguyên CPU[cite: 2].
* Ngay khi nhận được giá trị mực nước từ `data Alert`, nếu hệ thống đang ở chế độ tự động (Auto), Task sẽ kiểm tra các ngưỡng điều kiện và điều khiển cơ cấu chấp hành theo đúng thuật toán[cite: 2]:
  * **Ngưỡng 1 (Mực nước từ 1m đến 1.5m):** Hệ thống kích hoạt Máy bơm nước Vào hoạt động ở 50% công suất và tắt Van thoát[cite: 2]. Trạng thái bơm 50% được duy trì trong đúng 10 giây, sau đó tự động tắt máy bơm[cite: 2].
  * **Ngưỡng 2 (Mực nước từ 1.5m đến 2m):** Hệ thống kích hoạt Máy bơm nước Vào hoạt động ở 100% công suất và tắt Van thoát[cite: 2]. Trạng thái bơm 100% được duy trì trong đúng 20 giây, sau đó tự động tắt máy bơm[cite: 2].
  * **Ngưỡng 3 (Mực nước nhỏ hơn 0.5m):** Hệ thống tắt Máy bơm và bật Van thoát nước Ra[cite: 2]. Trạng thái mở van thoát được duy trì trong đúng 10 giây, sau đó tự động đóng van[cite: 2].
  * **Ngưỡng bình thường (Mực nước từ 0.5m đến dưới 1m):** Cả Máy bơm nước Vào và Van thoát nước Ra đều được giữ ở trạng thái nghỉ (OFF)[cite: 2].
* Nhờ cơ chế đa nhiệm chiếm quyền của FreeRTOS, trong suốt khoảng thời gian `Task cảnh báo` tạm dừng 10s hoặc 20s bằng `vTaskDelay()`, nhân hệ điều hành sẽ nhường CPU cho `Task đọc data` tiếp tục cập nhật thông số cảm biến và đẩy dữ liệu lên Web mà không làm đứng toàn bộ hệ thống như khi dùng hàm `delay()` thông thường trong vòng lặp Super Loop[cite: 2].

#### c) Hoạt động của "Task truyền data" (Chạy trên Core 0)[cite: 2]
* Khi khởi động trên Core 0, Task tiến hành kết nối vào mạng Wi-Fi ảo của Wokwi (`Wokwi-GUEST`) và thiết lập kết nối TCP tới máy chủ trung gian MQTT Broker[cite: 2].
* Trong vòng lặp chính, Task duy trì kết nối MQTT, kiểm tra nếu kết nối thành công thì đọc gói dữ liệu từ hàng đợi `data Queue`[cite: 2].
* Khi lấy được dữ liệu từ `data Queue`, Task đóng gói giá trị Mực nước, Nhiệt độ bể cùng trạng thái Bơm/Van thành chuỗi định dạng JSON và phát (Publish) lên Topic dữ liệu của MQTT Broker[cite: 2].
* Đồng thời, thông qua hàm ngắt nhận tin (Callback) khi đăng ký (Subscribe) Topic điều khiển, Task tiếp nhận các lệnh bật/tắt Máy bơm và Van thoát từ người dùng trên giao diện Web ở chế độ điều khiển thủ công[cite: 2].

![Lưu đồ thuật toán hệ thống RTOS](images/rtos_flowchart.png)

---

## 4. Thiết kế giao thức MQTT[cite: 2]

### Thông số kết nối MQTT[cite: 2]

| Thông số | Giá trị |
| :--- | :--- |
| **MQTT Broker** | `broker.emqx.io`[cite: 2] |
| **Giao thức ESP32** | MQTT over TCP[cite: 2] |
| **Cổng ESP32** | `1883`[cite: 2] |
| **Giao thức website** | MQTT over WebSocket Secure[cite: 2] |
| **Cổng website** | `8084`[cite: 2] |
| **Đường dẫn WebSocket** | `/mqtt`[cite: 2] |
| **URL website sử dụng** | `wss://broker.emqx.io:8084/mqtt`[cite: 2] |

### Bảng Topic dữ liệu đi và đến[cite: 2]

| Topic | Chiều truyền | Nội dung |
| :--- | :--- | :--- |
| `utc/iot_lab/de4/telemetry` | ESP32 $\rightarrow$ Broker $\rightarrow$ Website/MQTTX | Gửi dữ liệu cảm biến và trạng thái hệ thống, bao gồm: mực nước (m), nhiệt độ ($^\circ\text{C}$), trạng thái máy bơm (`ĐÓNG`, `50%`, `100%`), trạng thái van thoát (`MỞ`/`ĐÓNG`) và chế độ hoạt động (`TỰ ĐỘNG`/`THỦ CÔNG`)[cite: 2]. |
| `utc/iot_lab/de4/control` | Website/MQTTX $\rightarrow$ Broker $\rightarrow$ ESP32 | Nhận các lệnh điều khiển từ Website hoặc MQTTX để thay đổi chế độ hoạt động, điều khiển công suất máy bơm và đóng/mở van thoát[cite: 2]. |

### Các lệnh điều khiển sử dụng trên Topic `utc/iot_lab/de4/control`[cite: 2]

| Lệnh MQTT | Chức năng |
| :--- | :--- |
| `MODE_AUTO` | Chuyển hệ thống sang chế độ tự động[cite: 2]. |
| `MODE_MANUAL` | Chuyển hệ thống sang chế độ thủ công[cite: 2]. |
| `PUMP_50` | Bật máy bơm ở mức công suất 50%[cite: 2]. |
| `PUMP_ON` | Bật máy bơm ở mức công suất 100%[cite: 2]. |
| `PUMP_OFF` | Tắt máy bơm[cite: 2]. |
| `VALVE_ON` | Mở van thoát nước[cite: 2]. |
| `VALVE_OFF` | Đóng van thoát nước[cite: 2]. |

### Kiểm thử kết nối và truyền nhận dữ liệu trên MQTTX[cite: 2]

* **Giao diện kết nối:**[cite: 2]
![Giao diện kết nối MQTTX](images/mqttx_connection.png)

* **Khi nạp code vào mạch mô phỏng thì có kết quả và gửi dữ liệu kết quả cho MQTTX:**[cite: 2]
![Mô phỏng Wokwi gửi dữ liệu MQTT](images/wokwi_mqtt_telemetry.png)

* **Dữ liệu nhận được từ topic `utc/iot_lab/de4/telemetry` trên MQTTX:**[cite: 2]
![Dữ liệu nhận được trên MQTTX](images/mqttx_telemetry_received.png)

* **Điều khiển PUMP/VALVE thông qua topic `utc/iot_lab/de4/control` từ MQTTX:**[cite: 2]
  * *Khi điều khiển bật máy bơm từ MQTTX:*[cite: 2]
  ![Lệnh PUMP_ON trên MQTTX](images/mqttx_pump_on_cmd.png)
  ![Kết quả bật máy bơm trên Wokwi](images/wokwi_pump_on_result.png)

  * *Khi bật van xả từ MQTTX:*[cite: 2]
  ![Lệnh VALVE_ON trên MQTTX](images/mqttx_valve_on_cmd.png)
  ![Kết quả mở van xả trên Wokwi](images/wokwi_valve_on_result.png)

---

## 5. Thiết kế giao diện website để hiển thị dữ liệu/điều khiển[cite: 2]

Website được xây dựng bằng HTML, CSS và JavaScript nhằm giám sát, hiển thị dữ liệu và điều khiển hệ thống quản lý bể nuôi thủy, hải sản thông minh thông qua giao thức MQTT[cite: 2].

### Các chức năng chính[cite: 2]
* Hiển thị thông số mực nước bể nuôi (m) và nhiệt độ nước bể ($^\circ\text{C}$) dạng số thông qua các thẻ HTML class[cite: 2].
* Hiển thị cảnh báo vùng ngưỡng mực nước hiện tại (4 ngưỡng) và đánh giá trạng thái môi trường nhiệt độ[cite: 2].
* Hiển thị trạng thái hoạt động, công suất xung PWM của Máy bơm và trạng thái Đóng/Mở của Van thoát[cite: 2].
* Sử dụng biểu đồ theo dõi sự thay đổi mực nước, nhiệt độ theo thời gian thực và mô phỏng trực quan mặt cắt bể nuôi trung tâm[cite: 2].
* Cung cấp các nút điều khiển Máy bơm (Bật 50%, Bật 100%, Đóng bơm) và Van thoát đáy (Mở van, Đóng van)[cite: 2].
* Cho phép chuyển đổi linh hoạt giữa chế độ vận hành Tự động (`AUTO`) và Thủ công (`MANUAL`)[cite: 2].

### Nguyên lý hoạt động[cite: 2]
* Website sử dụng thư viện `MQTT.js` để kết nối với MQTT Broker EMQX thông qua giao thức WebSocket[cite: 2].
* Đăng ký (Subscribe) nhận dữ liệu từ ESP32 trên Wokwi gửi lên để cập nhật thông số cảm biến và trạng thái thiết bị lên giao diện[cite: 2].
* Khi người dùng nhấn các nút chuyển đổi chế độ hoặc điều khiển thủ công, Website gửi (Publish) lệnh điều khiển tới Topic `utc/iot_lab/de4/control`[cite: 2].
* ESP32 nhận lệnh từ MQTT Broker, xử lý và điều khiển đóng/ngắt hoặc băm xung PWM cho Máy bơm và Van thoát tương ứng[cite: 2].

**Kết quả:** Website giám sát chính xác dữ liệu mực nước và nhiệt độ bể nuôi theo thời gian thực, hiển thị trực quan trạng thái cảnh báo và điều khiển từ xa Máy bơm, Van thoát ổn định[cite: 2].

![Giao diện Website Dashboard - Phần điều khiển](images/web_dashboard_1.png)
![Giao diện Website Dashboard - Mặt cắt bể nuôi](images/web_dashboard_2.png)

---

## 6. Quá trình tạo kết nối, cập nhật dữ liệu và điều khiển[cite: 2]

* **Thiết lập kết nối Wi-Fi và MQTT:** ESP32 kết nối Internet qua mạng Wi-Fi `Wokwi-GUEST` và thiết lập giao tiếp với máy chủ MQTT Broker tại địa chỉ `broker.emqx.io` (cổng `1883`)[cite: 2]. Sau khi kết nối thành công, thiết bị đăng ký lắng nghe dữ liệu từ Topic điều khiển để sẵn sàng nhận và xử lý lệnh từ xa[cite: 2].
* **Cập nhật và truyền dữ liệu cảm biến:** Định kỳ 2 giây/lần, hệ thống thu thập thông số mực nước từ cảm biến HC-SR04 và nhiệt độ từ cảm biến DS18B20[cite: 2]. Các dữ liệu này cùng với trạng thái hoạt động của thiết bị được đóng gói thành định dạng JSON và gửi lên Broker qua Topic `utc/iot_lab/de4/telemetry` để hiển thị trên Website và MQTTX[cite: 2].
* **Nhận lệnh và điều khiển cơ cấu chấp hành:** Ở chế độ tự động, hệ thống tự căn cứ vào ngưỡng mực nước để điều khiển máy bơm và van thoát theo kịch bản lập trình sẵn[cite: 2]. Khi nhận được lệnh từ Website hoặc MQTTX qua Topic `utc/iot_lab/de4/control`, ESP32 sẽ phân tích lệnh để chuyển đổi chế độ hoạt động (Tự động/Thủ công), điều chỉnh công suất máy bơm (bật 50%, 100% hoặc tắt) và đóng/mở van thoát nước[cite: 2].
* **Đồng bộ trạng thái sau điều khiển:** Ngay sau khi nhận và thực thi lệnh điều khiển, ESP32 kiểm tra kết nối MQTT rồi tổng hợp lại thông tin cảm biến cùng trạng thái thiết bị mới nhất để gửi ngay lên Topic Telemetry[cite: 2]. Nhờ đó, giao diện Website và MQTTX luôn cập nhật chính xác trạng thái hiển thị theo thời gian thực[cite: 2].

---

## 7. Kết quả đạt được sau khi mô phỏng[cite: 2]

### TH1: Mực nước 1m - 1.5m bật 50% công suất máy bơm, trong 10s[cite: 2]
![TH1 - Mô phỏng Wokwi](images/th1_wokwi.png)
![TH1 - Giao diện Website](images/th1_web.png)

### TH2: Mực nước 1.5m - 2m bật 100% công suất máy bơm, trong 20s[cite: 2]
![TH2 - Mô phỏng Wokwi](images/th2_wokwi.png)
![TH2 - Giao diện Website](images/th2_web.png)

### TH3: Mực nước < 0.5m bật Van thoát, trong 10s[cite: 2]
![TH3 - Mô phỏng Wokwi](images/th3_wokwi.png)
![TH3 - Giao diện Website](images/th3_web.png)

### TH4: Khi chuyển sang chế độ thủ công, yêu cầu đóng máy bơm với mức nước 1.5m - 2m và mở van thoát (chấp hành ngược lại với chế độ Tự động ở TH2)[cite: 2]
![TH4 - Giao diện Website](images/th4_web.png)
![TH4 - Mô phỏng Wokwi](images/th4_wokwi.png)
