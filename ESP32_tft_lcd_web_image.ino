#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <JPEGDecoder.h>
#include <Arduino_GFX_Library.h>

// ==== Cấu hình chân TFT (tùy board) ====
#define TFT_CS     7
#define TFT_DC     8
#define TFT_RST    9
#define TFT_MOSI   10
#define TFT_SCK    6

Arduino_DataBus *bus = new Arduino_SWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 240, 280);

// ==== Cấu hình WiFi ====
const char* ssid = "My Nhung";
const char* password = "0932472990";
IPAddress local_IP(192, 168, 1, 123);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// ==== Web server ====
WebServer server(80);
File uploadFile;

// ==== Danh sách ảnh hiển thị ====
const char* images[] = {"/test1.jpg", "/test2.jpg", "/test3.jpg", "/test4.jpg", "/test5.jpg", "/test6.jpg"};
int currentImage = 0;
unsigned long lastChange = 0;
const unsigned long interval = 5000; // 5 giây
bool autoRotate = true; // Trạng thái tự động chuyển ảnh

// ==== Setup ====
void setup() {
  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(0, 0);
  gfx->println("Khoi dong...");

  if (!LittleFS.begin(true)) {
    gfx->println("LittleFS loi!");
    return;
  }

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  gfx->println("Dang ket noi WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    gfx->print(".");
  }
  gfx->println("");
  gfx->print("WiFi OK: ");
  gfx->println(WiFi.localIP());

  // Giao diện HTML upload với CSS đẹp và các chức năng mới
  server.on("/", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Image Display Manager</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            box-shadow: 0 15px 35px rgba(0, 0, 0, 0.1);
            padding: 30px;
            backdrop-filter: blur(10px);
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .header h1 {
            color: #333;
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
        }
        
        .header p {
            color: #666;
            font-size: 1.1em;
        }
        
        .section {
            background: white;
            border-radius: 15px;
            padding: 25px;
            margin-bottom: 20px;
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.08);
            border: 1px solid #e0e0e0;
        }
        
        .section h2 {
            color: #444;
            margin-bottom: 20px;
            font-size: 1.5em;
            border-bottom: 2px solid #667eea;
            padding-bottom: 10px;
        }
        
        .upload-form {
            display: grid;
            gap: 20px;
        }
        
        .form-group {
            display: flex;
            flex-direction: column;
            gap: 8px;
        }
        
        .form-group label {
            font-weight: 600;
            color: #555;
            font-size: 1.1em;
        }
        
        input[type="file"], select {
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 10px;
            font-size: 1em;
            transition: all 0.3s ease;
            background: #fafafa;
        }
        
        input[type="file"]:focus, select:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 10px rgba(102, 126, 234, 0.3);
        }
        
        .btn {
            padding: 12px 25px;
            border: none;
            border-radius: 10px;
            font-size: 1.1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .btn-primary {
            background: linear-gradient(45deg, #667eea, #764ba2);
            color: white;
            box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6);
        }
        
        .btn-secondary {
            background: linear-gradient(45deg, #11998e, #38ef7d);
            color: white;
            box-shadow: 0 4px 15px rgba(17, 153, 142, 0.4);
        }
        
        .btn-secondary:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(17, 153, 142, 0.6);
        }
        
        .btn-tertiary {
            background: linear-gradient(45deg, #ff6b6b, #feca57);
            color: white;
            box-shadow: 0 4px 15px rgba(255, 107, 107, 0.4);
        }
        
        .btn-tertiary:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(255, 107, 107, 0.6);
        }
        
        .checkbox-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }
        
        .checkbox-item {
            display: flex;
            align-items: center;
            padding: 15px;
            background: #f8f9ff;
            border-radius: 10px;
            border: 2px solid transparent;
            transition: all 0.3s ease;
        }
        
        .checkbox-item:hover {
            background: #e8f0ff;
            border-color: #667eea;
        }
        
        .checkbox-item input[type="checkbox"] {
            width: 20px;
            height: 20px;
            margin-right: 12px;
            accent-color: #667eea;
        }
        
        .checkbox-item label {
            font-weight: 500;
            color: #555;
            cursor: pointer;
            flex: 1;
        }
        
        .status-dot {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-left: 10px;
        }
        
        .status-exists {
            background: #4caf50;
        }
        
        .status-missing {
            background: #f44336;
        }
        
        .button-group {
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
            justify-content: center;
        }
        
        .files-list {
            background: #f8f9ff;
            border-radius: 10px;
            padding: 20px;
        }
        
        .files-list ul {
            list-style: none;
        }
        
        .files-list li {
            padding: 10px;
            background: white;
            margin-bottom: 8px;
            border-radius: 8px;
            border-left: 4px solid #4caf50;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        
        @media (max-width: 600px) {
            .container {
                padding: 20px;
                margin: 10px;
            }
            
            .header h1 {
                font-size: 2em;
            }
            
            .button-group {
                flex-direction: column;
            }
            
            .checkbox-grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🖼️ ESP32 Image Display</h1>
            <p>Quản lý và hiển thị ảnh JPEG trên màn hình TFT</p>
        </div>
        
        <div class="section">
            <h2>📤 Tải ảnh lên</h2>
            <form method='POST' action='/upload' enctype='multipart/form-data' class="upload-form">
                <div class="form-group">
                    <label>Chọn file JPG:</label>
                    <input type='file' name='upload' accept='.jpg,.jpeg' required>
                </div>
                <div class="form-group">
                    <label>Lưu thành:</label>
                    <select name='filename' required>
                        <option value='test1.jpg'>test1.jpg</option>
                        <option value='test2.jpg'>test2.jpg</option>
                        <option value='test3.jpg'>test3.jpg</option>
                        <option value='test4.jpg'>test4.jpg</option>
                        <option value='test5.jpg'>test5.jpg</option>
                        <option value='test6.jpg'>test6.jpg</option>
                    </select>
                </div>
                <input type='submit' value='📤 Tải lên' class="btn btn-primary">
            </form>
        </div>
        
        <div class="section">
            <h2>🖼️ Chọn ảnh hiển thị</h2>
            <form method='POST' action='/display-selected'>
                <div class="checkbox-grid">
    )rawliteral";
    
    // Tạo checkbox cho từng ảnh
    for (int i = 0; i < 6; i++) {
      String imageName = String(images[i]).substring(1); // Bỏ dấu "/"
      bool exists = LittleFS.exists(images[i]);
      String statusClass = exists ? "status-exists" : "status-missing";
      
      html += "<div class='checkbox-item'>";
      html += "<input type='checkbox' name='selected' value='" + imageName + "' id='img" + String(i) + "'>";
      html += "<label for='img" + String(i) + "'>" + imageName + "</label>";
      html += "<div class='status-dot " + statusClass + "'></div>";
      html += "</div>";
    }
    
    html += R"rawliteral(
                </div>
                <div class="button-group">
                    <input type='submit' value='🎯 Hiển thị ảnh đã chọn' class="btn btn-secondary">
                </div>
            </form>
        </div>
        
        <div class="section">
            <h2>⚡ Điều khiển nhanh</h2>
            <div class="button-group">
                <button onclick="displayAllImages()" class="btn btn-tertiary">🔄 Hiển thị tất cả ảnh</button>
                <button onclick="toggleAutoRotate()" class="btn btn-secondary">⏯️ Bật/Tắt tự động chuyển</button>
            </div>
        </div>
        
        <div class="section">
            <h2>📁 Danh sách file hiện có</h2>
            <div class="files-list">
                <ul>
    )rawliteral";
    
    // Hiển thị danh sách file có sẵn
    bool hasFiles = false;
    for (int i = 0; i < 6; i++) {
      if (LittleFS.exists(images[i])) {
        html += "<li>✅ " + String(images[i]) + "</li>";
        hasFiles = true;
      }
    }
    if (!hasFiles) {
      html += "<li>❌ Chưa có file nào</li>";
    }
    
    html += R"rawliteral(
                </ul>
            </div>
        </div>
    </div>
    
    <script>
        function displayAllImages() {
            fetch('/display-all', {method: 'POST'})
                .then(response => response.text())
                .then(data => alert('Đã bật hiển thị tất cả ảnh!'));
        }
        
        function toggleAutoRotate() {
            fetch('/toggle-auto', {method: 'POST'})
                .then(response => response.text())
                .then(data => alert(data));
        }
    </script>
</body>
</html>
    )rawliteral";
    
    server.send(200, "text/html", html);
  });

  // Xử lý upload ảnh (giữ nguyên)
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "Tai len hoan tat. <a href='/'>Quay lai</a>");
  }, []() {
    HTTPUpload& upload = server.upload();
    static String filename;
    if (upload.status == UPLOAD_FILE_START) {
      filename = "/" + server.arg("filename");
      if (LittleFS.exists(filename)) LittleFS.remove(filename);
      uploadFile = LittleFS.open(filename, "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) uploadFile.close();
    }
  });

  // Xử lý hiển thị ảnh đã chọn
  server.on("/display-selected", HTTP_POST, []() {
    String selectedImages = "";
    int count = 0;
    
    // Thu thập các ảnh được chọn
    for (int i = 0; i < server.args(); i++) {
      if (server.argName(i) == "selected") {
        String imgPath = "/" + server.arg(i);
        if (LittleFS.exists(imgPath)) {
          selectedImages += imgPath + " ";
          count++;
        }
      }
    }
    
    if (count > 0) {
      autoRotate = false; // Tắt tự động chuyển
      // Hiển thị ảnh đầu tiên được chọn
      String firstImage = selectedImages.substring(0, selectedImages.indexOf(' '));
      gfx->fillScreen(BLACK);
      drawJPEG(firstImage.c_str(), 0, 0);
      
      server.send(200, "text/html", 
        "<h2>Da hien thi anh da chon!</h2><p>So luong: " + String(count) + "</p><a href='/'>Quay lai</a>");
    } else {
      server.send(200, "text/html", 
        "<h2>Khong co anh nao duoc chon hoac ton tai!</h2><a href='/'>Quay lai</a>");
    }
  });

  // Xử lý hiển thị tất cả ảnh
  server.on("/display-all", HTTP_POST, []() {
    autoRotate = true; // Bật lại tự động chuyển
    currentImage = 0;
    lastChange = millis();
    gfx->fillScreen(BLACK);
    drawJPEG(images[currentImage], 0, 0);
    server.send(200, "text/plain", "Da bat hien thi tat ca anh!");
  });

  // Xử lý bật/tắt tự động chuyển ảnh
  server.on("/toggle-auto", HTTP_POST, []() {
    autoRotate = !autoRotate;
    if (autoRotate) {
      lastChange = millis(); // Reset timer
      server.send(200, "text/plain", "Da BAT tu dong chuyen anh!");
    } else {
      server.send(200, "text/plain", "Da TAT tu dong chuyen anh!");
    }
  });

  server.begin();

  // Kiểm tra file mặc định có tồn tại
  for (int i = 0; i < 6; i++) {
    if (!LittleFS.exists(images[i])) {
      gfx->printf("Thieu file: %s\n", images[i]);
    }
  }

  gfx->println("Dang hien anh...");
  delay(1000);

  // Hiển thị ảnh đầu tiên ngay khi khởi động
  drawJPEG(images[currentImage], 0, 0);
  lastChange = millis();
}

// ==== Loop ====
void loop() {
  server.handleClient();

  // Đổi ảnh định kỳ 5 giây (chỉ khi autoRotate = true)
  if (autoRotate && millis() - lastChange > interval) {
    lastChange = millis();
    currentImage = (currentImage + 1) % 6;
    gfx->fillScreen(BLACK);
    drawJPEG(images[currentImage], 0, 0);
  }
}

// ==== Giải mã JPEG và hiển thị (giữ nguyên) ====
void drawJPEG(const char *filename, int xpos, int ypos) {
  File jpegFile = LittleFS.open(filename, "r");
  if (!jpegFile || jpegFile.isDirectory()) {
    gfx->println("Mo file JPG that bai");
    return;
  }

  if (!JpegDec.decodeFsFile(jpegFile)) {
    gfx->println("Giai ma JPG that bai");
    return;
  }

  renderJPEG(xpos, ypos);
}

void renderJPEG(int xpos, int ypos) {
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint16_t img_w = JpegDec.width;
  uint16_t img_h = JpegDec.height;

  int crop_x = (img_w - gfx->width()) / 2;
  int crop_y = (img_h - gfx->height()) / 2;
  crop_x = crop_x < 0 ? 0 : crop_x;
  crop_y = crop_y < 0 ? 0 : crop_y;

  while (JpegDec.read()) {
    pImg = JpegDec.pImage;
    int mcu_x = JpegDec.MCUx * mcu_w;
    int mcu_y = JpegDec.MCUy * mcu_h;

    if (mcu_x + mcu_w > crop_x && mcu_x < crop_x + gfx->width() &&
        mcu_y + mcu_h > crop_y && mcu_y < crop_y + gfx->height()) {

      int draw_x = mcu_x - crop_x;
      int draw_y = mcu_y - crop_y;

      if (draw_x >= 0 && draw_y >= 0 &&
          draw_x + mcu_w <= gfx->width() &&
          draw_y + mcu_h <= gfx->height()) {
        gfx->draw16bitRGBBitmap(draw_x, draw_y, pImg, mcu_w, mcu_h);
      }
    }
  }
}
