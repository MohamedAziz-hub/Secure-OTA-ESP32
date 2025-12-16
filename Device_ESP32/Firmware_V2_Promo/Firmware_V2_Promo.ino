/*
 * ============================================
 * ESP32 SECURE OTA - VERSION MASTER (UNIVERSELLE)
 * ============================================
 * Architecture : ESP32
 * Sécurité : mbedTLS (Hardware Accelerated)
 * Fonctionnalités : HTTPS, SHA-256, NTP, Atomic Update
 * ============================================
 */

#include <WiFi.h>              // Spécifique ESP32
#include <HTTPClient.h>        // Spécifique ESP32
#include <WiFiClientSecure.h>  // Spécifique ESP32
#include <Update.h>            // Spécifique ESP32
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include "mbedtls/sha256.h"    // Moteur Hachage ESP32

// =====================================================
// 1. CONFIGURATION RÉSEAU
// =====================================================
const char* ssid = "OPPO A17";
const char* password = "sirzizou123";
const char* serverIP = "10.203.181.186"; 
const int serverPort = 3000;
const char* apiKey = "OTA_KEY_2025";

// =====================================================
// 2. ZONE DE PERSONNALISATION (MODIFIER ICI)
// =====================================================

const char* line1_text = "BLACK FRIDAY!!"; 
const char* line2_text = "PRIX 50 DT";  
int flashSpeed = 50; // Stroboscope

// =====================================================
// 3. CŒUR DU SYSTÈME (ESP32 ENGINE)
// =====================================================

#define LED_PIN 2 // LED Builtin sur ESP32 est souvent sur D2
LiquidCrystal_I2C lcd(0x27, 16, 2);
String storedHash = ""; 
unsigned long lastCheck = 0;
const unsigned long checkInterval = 30000; 

// Certificat (Même format que ESP8266)
const char* CA_CERT_PEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIID7zCCAtegAwIBAgIUX7i7tuzpXoUllwYBbSrZcyoP8GMwDQYJKoZIhvcNAQEL
BQAwgYYxCzAJBgNVBAYTAnRuMRAwDgYDVQQIDAdiaXplcnRlMRAwDgYDVQQHDAdi
aXplcnRlMRAwDgYDVQQKDAdiaXplcnRlMRAwDgYDVQQLDAdiaXplcnRlMRcwFQYD
VQQDDA4xMC4yMDMuMTgxLjE4NjEWMBQGCSqGSIb3DQEJARYHYml6ZXJ0ZTAeFw0y
NTEyMTYwNzU0MjJaFw0yNjEyMTYwNzU0MjJaMIGGMQswCQYDVQQGEwJ0bjEQMA4G
A1UECAwHYml6ZXJ0ZTEQMA4GA1UEBwwHYml6ZXJ0ZTEQMA4GA1UECgwHYml6ZXJ0
ZTEQMA4GA1UECwwHYml6ZXJ0ZTEXMBUGA1UEAwwOMTAuMjAzLjE4MS4xODYxFjAU
BgkqhkiG9w0BCQEWB2JpemVydGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQCk4/pys+z6h+G8eglrAXiCeKGWjUdHVsiQVHQZj9e1CUgx+UniYVDPOtt7
LgTEakFMtOE/U04P7PrenodQZDtjI079+d7tvoAPaq/m+AlB11vjrBRRzEjlROOq
6X8UL7f8EavNbCLe4bSXQjBTT88TdKO3+GSlSJnNvior8TTdvuMoNdI4gDGXLmrr
BQTYn/HwgupKcRQE4KZzGKxAKxASgPZKRmYQyYFKYYbLnMDB4zjA/2zD1crYOGVu
Pi/zrYkfKlRVNgD2oD01+F6vRVOHFmlR9FxfEC7J8Pmkwu+qxtm7nruqGTRzGOpp
FYYGekrfKDmY87e4I+Sh/FD1GF/3AgMBAAGjUzBRMB0GA1UdDgQWBBR4iXeIYKEe
yxusiWDewsd+RghFxDAfBgNVHSMEGDAWgBR4iXeIYKEeyxusiWDewsd+RghFxDAP
BgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBORPFJ4y1KiFwHZa6Q
zWMq5Z2YgjT02hJpQunjlCdKdlKx+P+QW1IF69zeusBmx2Wi+Mld81aHbyf4As7s
uj4nCbOsQ0BHUix8yVCiUOEFSuZ3ThS46shVg+asYgMjG3rMJNlDg9llDaNkfLzE
QS4RAcuvTZWw0YDnNBtiGMW1uzV83seVCeIrltF533kgR5BmWeD7kOvr2V4fsJtD
YF8UxvRhTtY2MV4ZXBxklm0oqyB8NshvGcLyPaKcN3HMzDfF/dXBKCLqKv/AF0r1
8MMaxAhPOJUO5PZ96XF4Bx8GIRJEz4jl2qe+XRLq17GHHoBkq7sPpkPmn3wGcdR/
MpV9
-----END CERTIFICATE-----
)EOF";

// --- Helpers Mémoire ---
String readHash() {
    EEPROM.begin(512);
    String h = "";
    for (int i = 0; i < 64; i++) {
        char c = EEPROM.read(i);
        if (c == '\0' || c == 255) break;
        h += c;
    }
    EEPROM.end(); // Sur ESP32 end() n'est pas toujours nécessaire mais ne fait pas de mal
    return h;
}

void saveHash(const String& h) {
    EEPROM.begin(512);
    for (int i = 0; i < 64; i++) {
        EEPROM.write(i, (i < h.length()) ? h.charAt(i) : '\0');
    }
    EEPROM.commit();
    EEPROM.end();
}

void showStatus(String l1, String l2) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(l1);
    lcd.setCursor(0, 1); lcd.print(l2);
}

// ============================================
// FONCTION 1 : TELECHARGEMENT SECURISE (ESP32)
// ============================================
void performSecureUpdate(String filename, String expectedSha256) {
    Serial.println("\n[ESP32] Demarrage Update...");
    showStatus("CONNEXION SSL...", "MAJ EN COURS");
    
    WiFiClientSecure client;
    client.setCACert(CA_CERT_PEM); // <--- Beaucoup plus simple sur ESP32 !

    if (!client.connect(serverIP, serverPort)) {
        Serial.println("[ERREUR] Connexion SSL Refusee");
        showStatus("ERREUR SSL", "CERT REJETE!"); 
        delay(3000);
        showStatus(line1_text, line2_text); 
        return;
    }

    String url = "/firmware/download/" + filename;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + serverIP + "\r\n" +
                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            showStatus("TIMEOUT", "ABANDON MAJ");
            client.stop();
            return;
        }
    }

    long contentLength = -1;
    while (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.startsWith("Content-Length:")) contentLength = line.substring(15).toInt();
        if (line.length() == 0) break;
    }

    if (!Update.begin(contentLength)) {
        showStatus("MEMOIRE PLEINE", "ABANDON");
        return;
    }

    showStatus("TELECHARGEMENT", "ECRITURE FLASH");
    
    // --- SHA-256 Engine (mbedTLS) pour ESP32 ---
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // 0 = SHA-256
    // ------------------------------------------

    size_t written = 0;
    uint8_t buff[1024]; // ESP32 peut gérer des buffers plus gros (vitesse++)
    
    while (client.available() > 0 || written < contentLength) {
        int len = client.read(buff, sizeof(buff));
        if (len > 0) {
            Update.write(buff, len);         
            mbedtls_sha256_update(&ctx, buff, len); // Hash à la volée
            written += len;
        }
    }

    // Récupération Hash
    uint8_t resHash[32];
    mbedtls_sha256_finish(&ctx, resHash);
    mbedtls_sha256_free(&ctx);
    
    String calculatedHash = "";
    for(int i=0; i<32; i++) {
        if(resHash[i] < 16) calculatedHash += "0";
        calculatedHash += String(resHash[i], HEX);
    }
    Serial.println("[ESP32] Hash calcule : " + calculatedHash);

    if (calculatedHash.equalsIgnoreCase(expectedSha256)) {
        showStatus("SECURITE OK", "SHA256 VALIDE");
        delay(1000);
        
        if (Update.end(true)) { 
            saveHash(expectedSha256);
            showStatus("MAJ REUSSIE", "REDEMARRAGE...");
            delay(1000);
            ESP.restart(); 
        }
    } else {
        Update.end(false); 
        showStatus("ALERTE SECURITE", "HASH INVALID!");
        delay(2000);
        showStatus("REJET MAJ", "RETOUR VERSION...");
        delay(2000);
        showStatus(line1_text, line2_text);
    }
}

// ============================================
// FONCTION 2 : VERIFICATION PERIODIQUE
// ============================================
void checkUpdate() {
    WiFiClientSecure client;
    client.setCACert(CA_CERT_PEM);
    
    HTTPClient http;
    String url = "https://" + String(serverIP) + ":" + String(serverPort) + "/firmware/latest";
    
    Serial.println("\n--- [ESP32] Verification Update ---");

    if (http.begin(client, url)) {
        http.addHeader("x-api-key", apiKey);
        int httpCode = http.GET();
        
        if (httpCode == 200) {
            String payload = http.getString();
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                String serverHash = doc["sha256"].as<String>();
                String fname = doc["filename"].as<String>();
                
                if (fname == "null" || fname.length() == 0) {
                     http.end(); return;
                }
                
                if (serverHash != storedHash) {
                    Serial.println("=> Changement detecte !");
                    http.end(); // Fermeture propre
                    performSecureUpdate(fname, serverHash);
                } else {
                    Serial.println("=> Pas de changement.");
                }
            }
        }
        http.end();
    }
}

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n--- BOOT ESP32 SECURE ---");

    pinMode(LED_PIN, OUTPUT);
    
    Wire.begin(21, 22); // PIN SDA=21, SCL=22 pour ESP32
    lcd.init();
    lcd.backlight();
    
    showStatus(line1_text, line2_text); 
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("Connexion Wifi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[WIFI] Connecte !");
    
    // --- SYNCHRO HEURE (Natif ESP32) ---
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Attente NTP");
    struct tm timeinfo;
    while(!getLocalTime(&timeinfo)){
        Serial.print(".");
        delay(500);
    }
    Serial.println("\n[SYSTEM] Heure OK !");
    // -----------------------------------

    storedHash = readHash();
    Serial.println("Hash en memoire : " + storedHash);
}

// ============================================
// LOOP
// ============================================
void loop() {
    unsigned long currentMillis = millis();
    
    if (currentMillis % (flashSpeed * 2) < flashSpeed) {
        digitalWrite(LED_PIN, HIGH); // Sur ESP32, HIGH allume souvent la LED
    } else {
        digitalWrite(LED_PIN, LOW);
    }

    if (currentMillis - lastCheck >= checkInterval) {
        lastCheck = currentMillis;
        if (WiFi.status() == WL_CONNECTED) {
            checkUpdate();
        }
    }
}