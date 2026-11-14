Link mô phỏng Wokwi:


https://wokwi.com/projects/480542136022290433


Link báo cáo chi tiết:


https://1drv.ms/w/c/e5ff9a7b0093a807/IQCkX35Cda36RYKKsfI3A095AfkcnL6dbx8WyKfvuVZBgLg?e=h153Pg


Link MQTT Broker:


https://mqttx.app/web-client#/recent_connections/a6d055d7-3c46-41b2-b0f0-837b71de7732


<div align="center">
  <h1>THIẾT KẾ MẠCH TỰ ĐỘNG GIÁM SÁT THÔNG SỐ MÔI TRƯỜNG CHO BỂ NUÔI THỦY, HẢI SẢN</h1>
</div>

* **Họ và tên:** Vũ Hùng Trường
* **Mã sinh viên:** 231402381

---

## 1. Mục tiêu bài thực hành và Các cảm biến chính sử dụng

### 1.1. Mục tiêu bài thực hành
Xây dựng và mô phỏng hoàn chỉnh hệ thống nhúng IoT tự động giám sát thông số môi trường (mực nước, nhiệt độ) và điều khiển cơ cấu chấp hành cho bể nuôi thủy, hải sản theo thời gian thực. Hệ thống ứng dụng vi điều khiển lõi kép ESP32 kết hợp hệ điều hành thời gian thực FreeRTOS và giao thức truyền thông MQTT nhằm đảm bảo tính ổn định, độ trễ thấp, xử lý đa nhiệm mượt mà và hỗ trợ người dùng giám sát, điều khiển từ xa qua giao diện Web/App.

### 1.2. Các cảm biến chính sử dụng.

#### a) Cảm biến siêu âm HC-SR04

<p align="center">
<img width="331" height="245" alt="image" src="https://github.com/user-attachments/assets/c3b81e72-db5e-40c9-b696-71edf2c716e7" />


* HC-SR04 là cảm biến đo khoảng cách bằng sóng siêu âm. Trong mô hình, cảm biến được giả định đặt phía trên bể và hướng xuống mặt nước, cho phép đo mà không cần tiếp xúc trực tiếp với nước.
* ESP32 phát một xung kích hoạt đến chân `TRIG`. Cảm biến phát sóng siêu âm và đưa ra xung tại chân `ECHO` có độ rộng tương ứng với thời gian sóng truyền đến bề mặt rồi phản xạ trở lại.
* Trong Wokwi, khoảng cách mô phỏng của HC-SR04 có thể điều chỉnh từ 2 đến 400 cm. Đơn vị khoảng cách sử dụng trong bài là centimet (cm).
* Khoảng cách được tính gần đúng:

$$d = \frac{t}{58}$$

Trong đó:
* $d$: Khoảng cách từ cảm biến đến mặt nước, đơn vị cm.
* $t$: Độ rộng xung `ECHO`, đơn vị micro giây ($\mu\text{s}$).

* Chiều cao mực nước được xác định theo công thức:

$$h = H - d$$

Trong đó $H$ là khoảng cách từ vị trí đặt cảm biến đến đáy bể.

#### b) Cảm biến nhiệt độ DS18B20

<p align="center">
<img width="272" height="325" alt="image" src="https://github.com/user-attachments/assets/ff85d7e1-faa9-47f2-99ee-f0ecb44c9bb2" />


* DS18B20 là cảm biến nhiệt độ số sử dụng giao tiếp 1-Wire, cho phép truyền dữ liệu qua một đường tín hiệu.
* Cảm biến có dải đo từ $-55^\circ\text{C}$ đến $+125^\circ\text{C}$, độ chính xác $\pm 0,5^\circ\text{C}$. Đơn vị nhiệt độ sử dụng trong bài là độ C ($^\circ\text{C}$).

---

## 2. Sơ đồ kết nối mạch trên Wokwi

[Sơ đồ kết nối mạch trên Wokwi]

<p align="center">
<img width="980" height="625" alt="image" src="https://github.com/user-attachments/assets/7e1f4da3-fbd4-48be-865a-cd7c090674d9" />


**Bảng kết nối chân:**

| Linh kiện | Tên chân linh kiện | Chân trên ESP32 |
| :--- | :--- | :--- |
| **Cảm biến khoảng cách HC-SR04** | VCC | 5V |
| | TRIG | GPIO 5 |
| | ECHO | GPIO 18 |
| | GND | GND |
| **Cảm biến nhiệt độ DS18B20** | VCC (VDD) | 3V3 |
| | DQ (Data) | GPIO 4 |
| | GND | GND |
| **Relay Module 1 (Van)** | VCC | 5V |
| | GND | GND |
| | IN | GPIO 27 |
| **Relay Module 2 (Bơm)** | VCC | 5V |
| | GND | GND |
| | IN | GPIO 25 |
| **LED Chỉ thị 1 (qua điện trở)** | Chân Dương (+) | GPIO 26 |
| | Chân Âm (-) | GND |
| **LED Chỉ thị 2 (qua điện trở)** | Chân Dương (+) | GPIO 14 |
| | Chân Âm (-) | GND |

---

## 3. Xây dựng hệ điều hành thời gian thực FreeRTOS trên ESP32

### 3.1. Kiến trúc phân bổ đa nhiệm trên 2 lõi (Dual-Core)
Để đảm bảo tính thời gian thực (Real-time) và ngăn ngừa việc xử lý giao thức mạng làm gián đoạn quá trình điều khiển bể nuôi, hệ thống sử dụng hàm `xTaskCreatePinnedToCore()` của FreeRTOS trên ESP32 nhằm ghim cố định các tác vụ (Task) vào từng lõi vật lý riêng biệt:

| Tên Task | Lõi thực thi | Mức ưu tiên | Nhiệm vụ thiết kế |
| :--- | :---: | :---: | :--- |
| **Task truyền data** | Core 0 | 1 | Kết nối Wi-Fi và MQTT Broker; đọc dữ liệu từ hàng đợi `data Queue` để gửi lên Broker; nhận lệnh điều khiển từ Web |
| **Task đọc data** | Core 1 | 2 | Đọc cảm biến HC-SR04 và nhiệt độ bể; hiển thị lên Serial Monitor; đẩy dữ liệu đầy đủ vào `data Queue` và đẩy mực nước vào `data Alert` |
| **Task cảnh báo** | Core 1 | 3 | Đọc dữ liệu mực nước từ `data Alert`; thực thi thuật toán điều khiển công suất Máy bơm (PWM) và Van thoát nước theo thời gian quy định |

### 3.2. Thiết kế Hàng đợi (Queue) giao tiếp giữa các Task
Hệ thống sử dụng 2 hàng đợi (Queue) hoạt động theo cơ chế FIFO (First-In, First-Out) để truyền các gói dữ liệu theo khối (bản tin) và đồng bộ hóa hoạt động giữa 3 Task mà không xảy ra xung đột bộ nhớ

* **Hàng đợi `data Queue`:**
  * **Mục đích:** Truyền toàn bộ thông số môi trường từ `Task đọc data` (Core 1) sang `Task truyền data` (Core 0)
  * **Kiểu dữ liệu phần tử:** Một cấu trúc dữ liệu (`struct SensorData`) gồm 2 trường là `waterLevel` (kiểu `float` - mực nước tính bằng mét) và `temperature` (kiểu `float` - nhiệt độ bể tính bằng độ C)
* **Hàng đợi `data Alert`:**
  * **Mục đích:** Truyền riêng thông số mực nước từ `Task đọc data` (Core 1) sang `Task cảnh báo` (Core 1) để xử lý thuật toán điều khiển cơ cấu chấp hành
  * **Kiểu dữ liệu phần tử:** Biến số thực (`float`) biểu diễn giá trị mực nước hiện tại (đơn vị: mét)

### 3.3. Chi tiết hoạt động và thuật toán của các Task

#### a) Hoạt động của "Task đọc data" (Chạy trên Core 1)
* Định kỳ mỗi chu kỳ lấy mẫu, Task phát xung kích $10\ \mu\text{s}$ vào chân `TRIG` của cảm biến HC-SR04, đo thời gian phản hồi trên chân `ECHO` và tính toán ra thông số mực nước (đơn vị: m)
* Tiếp theo, Task yêu cầu cảm biến DS18B20 chuyển đổi và đọc giá trị nhiệt độ nước trong bể qua bus 1-Wire
* Các thông số thu thập được sẽ được xuất trực tiếp ra màn hình Serial Monitor thông qua bộ giao tiếp nối tiếp UART0 (`Serial.printf`) ở tốc độ baud `115200` để phục vụ giám sát cục bộ
* Sau đó, Task đóng gói cả mực nước và nhiệt độ gửi vào hàng đợi `data Queue` bằng API `xQueueSend()`, đồng thời gửi riêng giá trị mực nước vào hàng đợi `data Alert` trước khi gọi hàm `vTaskDelay()` để chuyển sang trạng thái khóa (Blocked) chờ chu kỳ đọc tiếp theo

#### b) Hoạt động của "Task cảnh báo" và Thuật toán điều khiển mực nước (Chạy trên Core 1)
* Task luôn chờ bản tin từ hàng đợi `data Alert` thông qua hàm `xQueueReceive()`. Khi không có dữ liệu mới trong hàng đợi, Task ở trạng thái Blocked và không tiêu tốn tài nguyên CPU
* Ngay khi nhận được giá trị mực nước từ `data Alert`, nếu hệ thống đang ở chế độ tự động (Auto), Task sẽ kiểm tra các ngưỡng điều kiện và điều khiển cơ cấu chấp hành theo đúng thuật toán
  * **Ngưỡng 1 (Mực nước từ 1m đến 1.5m):** Hệ thống kích hoạt Máy bơm nước Vào hoạt động ở 50% công suất và tắt Van thoát. Trạng thái bơm 50% được duy trì trong đúng 10 giây, sau đó tự động tắt máy bơm
  * **Ngưỡng 2 (Mực nước từ 1.5m đến 2m):** Hệ thống kích hoạt Máy bơm nước Vào hoạt động ở 100% công suất và tắt Van thoát. Trạng thái bơm 100% được duy trì trong đúng 20 giây, sau đó tự động tắt máy bơm
  * **Ngưỡng 3 (Mực nước nhỏ hơn 0.5m):** Hệ thống tắt Máy bơm và bật Van thoát nước Ra. Trạng thái mở van thoát được duy trì trong đúng 10 giây, sau đó tự động đóng van
  * **Ngưỡng bình thường (Mực nước từ 0.5m đến dưới 1m):** Cả Máy bơm nước Vào và Van thoát nước Ra đều được giữ ở trạng thái nghỉ (OFF)
* Nhờ cơ chế đa nhiệm chiếm quyền của FreeRTOS, trong suốt khoảng thời gian `Task cảnh báo` tạm dừng 10s hoặc 20s bằng `vTaskDelay()`, nhân hệ điều hành sẽ nhường CPU cho `Task đọc data` tiếp tục cập nhật thông số cảm biến và đẩy dữ liệu lên Web mà không làm đứng toàn bộ hệ thống như khi dùng hàm `delay()` thông thường trong vòng lặp Super Loop

#### c) Hoạt động của "Task truyền data" (Chạy trên Core 0)
* Khi khởi động trên Core 0, Task tiến hành kết nối vào mạng Wi-Fi ảo của Wokwi (`Wokwi-GUEST`) và thiết lập kết nối TCP tới máy chủ trung gian MQTT Broker
* Trong vòng lặp chính, Task duy trì kết nối MQTT, kiểm tra nếu kết nối thành công thì đọc gói dữ liệu từ hàng đợi `data Queue`
* Khi lấy được dữ liệu từ `data Queue`, Task đóng gói giá trị Mực nước, Nhiệt độ bể cùng trạng thái Bơm/Van thành chuỗi định dạng JSON và phát (Publish) lên Topic dữ liệu của MQTT Broker
* Đồng thời, thông qua hàm ngắt nhận tin (Callback) khi đăng ký (Subscribe) Topic điều khiển, Task tiếp nhận các lệnh bật/tắt Máy bơm và Van thoát từ người dùng trên giao diện Web ở chế độ điều khiển thủ công

[Lưu đồ thuật toán hệ thống RTOS]


<p align="center">
<img width="868" height="764" alt="image" src="https://github.com/user-attachments/assets/40da968f-6b02-4c7a-a0c6-c01649ec0a50" />


---

## 4. Thiết kế giao thức MQTT

### Thông số kết nối MQTT

| Thông số | Giá trị |
| :--- | :--- |
| **MQTT Broker** | `broker.emqx.io` |
| **Giao thức ESP32** | MQTT over TCP |
| **Cổng ESP32** | `1883` |
| **Giao thức website** | MQTT over WebSocket Secure |
| **Cổng website** | `8084` |
| **Đường dẫn WebSocket** | `/mqtt` |
| **URL website sử dụng** | `wss://broker.emqx.io:8084/mqtt` |

### Bảng Topic dữ liệu đi và đến

| Topic | Chiều truyền | Nội dung |
| :--- | :--- | :--- |
| `utc/iot_lab/de4/telemetry` | ESP32 $\rightarrow$ Broker $\rightarrow$ Website/MQTTX | Gửi dữ liệu cảm biến và trạng thái hệ thống, bao gồm: mực nước (m), nhiệt độ ($^\circ\text{C}$), trạng thái máy bơm (`ĐÓNG`, `50%`, `100%`), trạng thái van thoát (`MỞ`/`ĐÓNG`) và chế độ hoạt động (`TỰ ĐỘNG`/`THỦ CÔNG`) |
| `utc/iot_lab/de4/control` | Website/MQTTX $\rightarrow$ Broker $\rightarrow$ ESP32 | Nhận các lệnh điều khiển từ Website hoặc MQTTX để thay đổi chế độ hoạt động, điều khiển công suất máy bơm và đóng/mở van thoát |

### Các lệnh điều khiển sử dụng trên Topic `utc/iot_lab/de4/control`

| Lệnh MQTT | Chức năng |
| :--- | :--- |
| `MODE_AUTO` | Chuyển hệ thống sang chế độ tự động |
| `MODE_MANUAL` | Chuyển hệ thống sang chế độ thủ công |
| `PUMP_50` | Bật máy bơm ở mức công suất 50% |
| `PUMP_ON` | Bật máy bơm ở mức công suất 100% |
| `PUMP_OFF` | Tắt máy bơm |
| `VALVE_ON` | Mở van thoát nước |
| `VALVE_OFF` | Đóng van thoát nước |

### Kiểm thử kết nối và truyền nhận dữ liệu trên MQTTX

* **Giao diện kết nối:**
[Giao diện kết nối MQTTX]


<p align="center">
<img width="980" height="501" alt="image" src="https://github.com/user-attachments/assets/f87dc0fb-387f-4cf1-9243-e0a47e7fb9d6" />


* **Khi nạp code vào mạch mô phỏng thì có kết quả và gửi dữ liệu kết quả cho MQTTX:**
[Mô phỏng Wokwi gửi dữ liệu MQTT]


<p align="center">
<img width="980" height="482" alt="image" src="https://github.com/user-attachments/assets/edcf1990-230f-4fe0-ad2a-fa550fe588fd" />


* **Dữ liệu nhận được từ topic `utc/iot_lab/de4/telemetry` trên MQTTX:**
[Dữ liệu nhận được trên MQTTX]


<p align="center">
<img width="980" height="419" alt="image" src="https://github.com/user-attachments/assets/ecb7fa8d-d87b-42ec-b20d-38391da91327" />


* **Điều khiển PUMP/VALVE thông qua topic `utc/iot_lab/de4/control` từ MQTTX:**
  * *Khi điều khiển bật máy bơm từ MQTTX:*
  [Lệnh PUMP_ON trên MQTTX]


<p align="center">
 <img width="856" height="202" alt="image" src="https://github.com/user-attachments/assets/f9cd2626-9c41-4ef5-a3d7-c52601e7e481" />


  [Kết quả bật máy bơm trên Wokwi]


  <p align="center">
  <img width="912" height="878" alt="image" src="https://github.com/user-attachments/assets/86572dde-f58e-4562-aa9d-fccee02992fa" />


  * *Khi bật van xả từ MQTTX:*
  [Lệnh VALVE_ON trên MQTTX]


<p align="center">
 <img width="866" height="169" alt="image" src="https://github.com/user-attachments/assets/9f5bc0b3-c596-4010-974f-5d4ed70c58f9" />

  [Kết quả mở van xả trên Wokwi]


  <p align="center">
  <img width="912" height="907" alt="image" src="https://github.com/user-attachments/assets/40009091-c265-4bda-aeff-872f42ca74a8" />


---

## 5. Thiết kế giao diện website để hiển thị dữ liệu/điều khiển

Website được xây dựng bằng HTML, CSS và JavaScript nhằm giám sát, hiển thị dữ liệu và điều khiển hệ thống quản lý bể nuôi thủy, hải sản thông minh thông qua giao thức MQTT

### Các chức năng chính
* Hiển thị thông số mực nước bể nuôi (m) và nhiệt độ nước bể ($^\circ\text{C}$) dạng số thông qua các thẻ HTML class
* Hiển thị cảnh báo vùng ngưỡng mực nước hiện tại (4 ngưỡng) và đánh giá trạng thái môi trường nhiệt độ
* Hiển thị trạng thái hoạt động, công suất xung PWM của Máy bơm và trạng thái Đóng/Mở của Van thoát
* Sử dụng biểu đồ theo dõi sự thay đổi mực nước, nhiệt độ theo thời gian thực và mô phỏng trực quan mặt cắt bể nuôi trung tâm
* Cung cấp các nút điều khiển Máy bơm (Bật 50%, Bật 100%, Đóng bơm) và Van thoát đáy (Mở van, Đóng van)
* Cho phép chuyển đổi linh hoạt giữa chế độ vận hành Tự động (`AUTO`) và Thủ công (`MANUAL`)

### Nguyên lý hoạt động
* Website sử dụng thư viện `MQTT.js` để kết nối với MQTT Broker EMQX thông qua giao thức WebSocket
* Đăng ký (Subscribe) nhận dữ liệu từ ESP32 trên Wokwi gửi lên để cập nhật thông số cảm biến và trạng thái thiết bị lên giao diện
* Khi người dùng nhấn các nút chuyển đổi chế độ hoặc điều khiển thủ công, Website gửi (Publish) lệnh điều khiển tới Topic `utc/iot_lab/de4/control`
* ESP32 nhận lệnh từ MQTT Broker, xử lý và điều khiển đóng/ngắt hoặc băm xung PWM cho Máy bơm và Van thoát tương ứng

**Kết quả:** Website giám sát chính xác dữ liệu mực nước và nhiệt độ bể nuôi theo thời gian thực, hiển thị trực quan trạng thái cảnh báo và điều khiển từ xa Máy bơm, Van thoát ổn định

[Giao diện Website Dashboard - Phần điều khiển]


<p align="center">
 <img width="980" height="531" alt="image" src="https://github.com/user-attachments/assets/f3ce6919-2c3c-40be-86eb-45c610f837f9" />

[Giao diện Website Dashboard - Mặt cắt bể nuôi]


<p align="center">
<img width="980" height="533" alt="image" src="https://github.com/user-attachments/assets/b6a51bf1-afe0-4181-b4a9-a8a181a33cb9" />


---

## 6. Quá trình tạo kết nối, cập nhật dữ liệu và điều khiển

* **Thiết lập kết nối Wi-Fi và MQTT:** ESP32 kết nối Internet qua mạng Wi-Fi `Wokwi-GUEST` và thiết lập giao tiếp với máy chủ MQTT Broker tại địa chỉ `broker.emqx.io` (cổng `1883`). Sau khi kết nối thành công, thiết bị đăng ký lắng nghe dữ liệu từ Topic điều khiển để sẵn sàng nhận và xử lý lệnh từ xa
* **Cập nhật và truyền dữ liệu cảm biến:** Định kỳ 2 giây/lần, hệ thống thu thập thông số mực nước từ cảm biến HC-SR04 và nhiệt độ từ cảm biến DS18B20 Các dữ liệu này cùng với trạng thái hoạt động của thiết bị được đóng gói thành định dạng JSON và gửi lên Broker qua Topic `utc/iot_lab/de4/telemetry` để hiển thị trên Website và MQTTX
* **Nhận lệnh và điều khiển cơ cấu chấp hành:** Ở chế độ tự động, hệ thống tự căn cứ vào ngưỡng mực nước để điều khiển máy bơm và van thoát theo kịch bản lập trình sẵn. Khi nhận được lệnh từ Website hoặc MQTTX qua Topic `utc/iot_lab/de4/control`, ESP32 sẽ phân tích lệnh để chuyển đổi chế độ hoạt động (Tự động/Thủ công), điều chỉnh công suất máy bơm (bật 50%, 100% hoặc tắt) và đóng/mở van thoát nước
* **Đồng bộ trạng thái sau điều khiển:** Ngay sau khi nhận và thực thi lệnh điều khiển, ESP32 kiểm tra kết nối MQTT rồi tổng hợp lại thông tin cảm biến cùng trạng thái thiết bị mới nhất để gửi ngay lên Topic Telemetry. Nhờ đó, giao diện Website và MQTTX luôn cập nhật chính xác trạng thái hiển thị theo thời gian thực.

---

## 7. Kết quả đạt được sau khi mô phỏng

### TH1: Mực nước 1m - 1.5m bật 50% công suất máy bơm, trong 10s
[TH1 - Mô phỏng Wokwi]


<p align="center">
 <img width="980" height="427" alt="image" src="https://github.com/user-attachments/assets/3ac48c9e-3c51-4cac-b09f-7ffd3f15b181" />

[TH1 - Giao diện Website]


<p align="center">
<img width="976" height="486" alt="image" src="https://github.com/user-attachments/assets/872a78b1-f856-4883-a53e-0268c293df18" />


### TH2: Mực nước 1.5m - 2m bật 100% công suất máy bơm, trong 20s
[TH2 - Mô phỏng Wokwi]


<p align="center">
 <img width="980" height="503" alt="image" src="https://github.com/user-attachments/assets/9cbe6b9c-577b-41d8-8ce7-f1cd0620650f" />

[TH2 - Giao diện Website]


<p align="center">
<img width="980" height="526" alt="image" src="https://github.com/user-attachments/assets/df0c60ec-6d68-4b4c-ba93-42e17a5618e0" />


### TH3: Mực nước < 0.5m bật Van thoát, trong 10s
[TH3 - Mô phỏng Wokwi]


<p align="center">
 <img width="980" height="502" alt="image" src="https://github.com/user-attachments/assets/1bd8f161-509c-41ac-bf8a-37048e53a6ae" />

[TH3 - Giao diện Website]


<p align="center">
<img width="980" height="525" alt="image" src="https://github.com/user-attachments/assets/24b3b9f7-472d-4ec9-b212-d92f95930e71" />


### TH4: Khi chuyển sang chế độ thủ công, yêu cầu đóng máy bơm với mức nước 1.5m - 2m và mở van thoát (chấp hành ngược lại với chế độ Tự động ở TH2)
[TH4 - Giao diện Website]


<p align="center">
 <img width="980" height="531" alt="image" src="https://github.com/user-attachments/assets/5f39c558-6391-40f9-938c-09bf45ad4162" />

[TH4 - Mô phỏng Wokwi]


<p align="center">
<img width="980" height="498" alt="image" src="https://github.com/user-attachments/assets/a567304b-7127-4ec6-8709-0b037d849965" />

