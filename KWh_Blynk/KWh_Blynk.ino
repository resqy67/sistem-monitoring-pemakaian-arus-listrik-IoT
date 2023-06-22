#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <BlynkSimpleEsp8266.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RESET_BUTTON_PIN D6
unsigned long buttonPressStartTime = 0;
unsigned int buttonClickCount = 0;
bool isButtonPressed = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);
PZEM004Tv30 pzem(D3, D4);
char auth[] = "iCnh8lqWRbl81St2F5sNi3Wkd_n3Mvob";

void setup() {
  // Inisialisasi serial
  Serial.begin(9600);
  
  // Membuat objek WiFiManager
  WiFiManager wifiManager;

  // Reset konfigurasi WiFiManager sebelumnya
  // wifiManager.resetSettings();
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting ...");
  delay(500);

  // Mencoba menghubungkan ke jaringan WiFi yang tersimpan
  // Jika tidak ada jaringan yang tersimpan, maka WiFiManager akan membuka portal konfigurasi
  int attempt = 0;
  while (!wifiManager.autoConnect("AP-ESP8266")) {
    Serial.println("Gagal terhubung ke jaringan WiFi.");

    // Mencoba hingga 3 kali
    if (attempt >= 3) {
      Serial.println("Mencoba akses titik akses sementara.");
      startAccessPointMode();
      break;
    }

    delay(3000);
    attempt++;
  }
  
  // Jika berhasil terhubung ke jaringan WiFi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Terhubung ke jaringan WiFi!");
    Serial.print("Alamat IP: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.print("Connected to WIFI!");
    lcd.setCursor(0, 1);
    lcd.print("IP: " + WiFi.localIP().toString());

    // Menghubungkan ke server Blynk
    Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());
  }
  
}

void loop() {
  // Menjalankan Blynk
  Blynk.run();

  // Periksa tombol untuk reset WiFiManager atau reset energi
  readResetButton();

  // Memeriksa tombol untuk menghitung jumlah klik
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    if (!isButtonPressed) {
      buttonPressStartTime = millis();
      isButtonPressed = true;
    }
  } else {
    isButtonPressed = false;
  }

  // Reset WiFiManager jika tombol diklik 5 kali
  if (buttonClickCount >= 5) {
    Serial.println("Tombol diklik 5 kali. Me-reset WiFiManager.");
    lcd.clear();
    lcd.print("Resetting WiFi...");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(1000);
    ESP.restart();
  }

  float voltage = pzem.voltage();
  if (!isnan(voltage)) {
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    lcd.setCursor(0, 0);
    lcd.print("V:");
    lcd.print(voltage);
    Blynk.virtualWrite(V0, voltage);
  } else {
    Serial.println("Error reading voltage");
  }

  float current = pzem.current();
  if (!isnan(current)) {
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");
    Blynk.virtualWrite(V1, current);
  } else {
    Serial.println("Error reading current");
  }

  float power = pzem.power();
  if (!isnan(power)) {
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");

    lcd.setCursor(8, 0);
    lcd.print(" W:");
    lcd.print(power);
    Blynk.virtualWrite(V2, power);
  } else {
    Serial.println("Error reading power");
  }

  float energy = pzem.energy();
  if (!isnan(energy)) {
    Serial.print("Energy: ");
    Serial.print(energy, 3);
    Serial.println("kWh");
    lcd.setCursor(0, 1);
    lcd.print("kWh:");
    lcd.print(energy);
    Blynk.virtualWrite(V3, energy);
  } else {
    Serial.println("Error reading energy");
  }

  float frequency = pzem.frequency();
  if (!isnan(frequency)) {
    Serial.print("Frequency: ");
    Serial.print(frequency, 1);
    Serial.println("Hz");

    lcd.setCursor(8, 1);
    lcd.print(" f:");
    lcd.print(frequency);
    Blynk.virtualWrite(V4, frequency);
  } else {
    Serial.println("Error reading frequency");
  }

  float pf = pzem.pf();
  if (!isnan(pf)) {
    Serial.print("PF: ");
    Serial.println(pf);
    Blynk.virtualWrite(V5, pf);
  } else {
    Serial.println("Error reading power factor");
  }

  delay(2000);
  lcd.clear();
}

void readResetButton() {
  int resetButtonReading = digitalRead(RESET_BUTTON_PIN);

  // Menahan tombol lebih dari 5 detik untuk mereset energi
  if (resetButtonReading == LOW) {
    buttonPressStartTime = millis();
    while (digitalRead(RESET_BUTTON_PIN) == LOW && millis() - buttonPressStartTime < 5000) {
      // Menunggu tombol dilepaskan atau waktu 5 detik tercapai
    }

    if (millis() - buttonPressStartTime >= 5000) {
      Serial.println("Tombol ditekan selama 5 detik. Me-reset Energy.");
      resetEnergy();
    }
  }

  // Menghitung jumlah klik tombol
  if (resetButtonReading == LOW && millis() - buttonPressStartTime >= 100) {
    buttonClickCount++;
    Serial.println(buttonClickCount);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Clicks: " + String(buttonClickCount));
  }
}

void resetEnergy() {
  pzem.resetEnergy();
  Serial.println("Energy reset");
  lcd.setCursor(0, 0);
  lcd.print("Energy reset");
  delay(2000);
  lcd.clear();
}

void startAccessPointMode() {
  Serial.println("Memulai mode akses titik akses sementara.");
  lcd.clear();
  lcd.print("Starting Ap Mode!");

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);

  // Mengatur konfigurasi untuk mode akses titik akses
  if (!wifiManager.startConfigPortal("AP-ESP8266")) {
    Serial.println("Gagal memulai mode akses titik akses.");
    lcd.clear();
    lcd.print("Failed to start AP Mode");
    lcd.setCursor(0, 1);
    lcd.print("Restarting ESP8266!!");
    ESP.restart();
    delay(5000);
  }

  lcd.clear();
  lcd.print("Connected to WIFi!");
  lcd.setCursor(0, 1);
  lcd.print("IP: " + WiFi.localIP().toString());
  delay(5000);
  Serial.println("Terhubung ke jaringan WiFi!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());

  // Menghubungkan ke server Blynk
  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());
}

void configModeCallback(WiFiManager *wifiManager) {
  lcd.clear();
  lcd.print("Config Portal");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.softAPIP());
}
