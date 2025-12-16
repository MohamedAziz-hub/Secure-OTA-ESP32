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

const char* line1_text = "NEW SAISON"; 
const char* line2_text = "PRIX 100 DT";  
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
    Serial.println("\n[ESP32] Demarrage Update...");       // Log le début de la procédure de mise à jour
    showStatus("CONNEXION SSL...", "MAJ EN COURS");        // Affiche l'état initial sur l'écran LCD
    
    WiFiClientSecure client;                               // Crée une instance de client WiFi sécurisé (SSL/TLS)
    client.setCACert(CA_CERT_PEM);                         // Charge le certificat racine pour authentifier le serveur

    if (!client.connect(serverIP, serverPort)) {           // Tente d'établir le tunnel chiffré avec le serveur
        Serial.println("[ERREUR] Connexion SSL Refusee");  // Log si le serveur rejette la connexion (mauvais certif)
        showStatus("ERREUR SSL", "CERT REJETE!");          // Affiche l'erreur critique de sécurité sur le LCD
        delay(3000);                                       // Pause de 3s pour permettre la lecture du message
        showStatus(line1_text, line2_text);                // Rétablit l'affichage normal de la version actuelle
        return;                                            // Annule tout et sort de la fonction immédiatement
    }

    String url = "/firmware/download/" + filename;         // Construit l'URL du fichier binaire à télécharger
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +  // Envoie la commande HTTP GET au serveur
                 "Host: " + serverIP + "\r\n" +            // Spécifie l'hôte (obligatoire pour HTTP/1.1)
                 "Connection: close\r\n\r\n");             // Demande la fermeture de connexion après réponse

    unsigned long timeout = millis();                      // Enregistre le temps actuel pour gérer le timeout
    while (client.available() == 0) {                      // Boucle d'attente tant qu'aucune donnée n'est reçue
        if (millis() - timeout > 5000) {                   // Vérifie si l'attente dépasse 5 secondes
            showStatus("TIMEOUT", "ABANDON MAJ");          // Affiche une erreur de délai dépassé
            client.stop();                                 // Force la fermeture de la connexion réseau
            return;                                        // Quitte la fonction
        }
    }

    long contentLength = -1;                               // Variable pour stocker la taille du fichier (en octets)
    while (client.available()) {                           // Boucle de lecture des en-têtes HTTP
        String line = client.readStringUntil('\n');        // Lit une ligne complète de l'en-tête
        line.trim();                                       // Nettoie les espaces et retours à la ligne
        if (line.startsWith("Content-Length:")) contentLength = line.substring(15).toInt(); // Extrait la taille du fichier
        if (line.length() == 0) break;                     // Une ligne vide indique la fin des en-têtes et le début du fichier
    }

    if (!Update.begin(contentLength)) {                    // Prépare la partition OTA secondaire avec la taille requise
        showStatus("MEMOIRE PLEINE", "ABANDON");           // Affiche une erreur si l'espace flash est insuffisant
        return;                                            // Quitte la fonction
    }

    showStatus("TELECHARGEMENT", "ECRITURE FLASH");        // Informe l'utilisateur que l'écriture en mémoire commence
    
    // --- SHA-256 Engine (mbedTLS) pour ESP32 ---
    mbedtls_sha256_context ctx;                            // Déclare la structure pour le calcul cryptographique
    mbedtls_sha256_init(&ctx);                             // Initialise le moteur de hachage SHA-256
    mbedtls_sha256_starts(&ctx, 0);                        // Démarre le calcul (0 indique le mode SHA-256)
    // ------------------------------------------

    size_t written = 0;                                    // Initialise le compteur d'octets écrits à 0
    uint8_t buff[1024];                                    // Crée un tampon de 1 Ko pour les paquets de données
    
    while (client.available() > 0 || written < contentLength) { // Boucle tant qu'il reste des données à lire
        int len = client.read(buff, sizeof(buff));         // Lit un paquet de données du réseau dans le tampon
        if (len > 0) {                                     // Si des données ont été reçues
            Update.write(buff, len);                       // Écrit les données dans la partition OTA isolée
            mbedtls_sha256_update(&ctx, buff, len);        // Ajoute ces données au calcul du hash en cours
            written += len;                                // Met à jour le total des octets écrits
        }
    }

    // Récupération Hash
    uint8_t resHash[32];                                   // Tableau pour stocker le résultat final du hash (32 octets)
    mbedtls_sha256_finish(&ctx, resHash);                  // Termine le calcul mathématique du SHA-256
    mbedtls_sha256_free(&ctx);                             // Libère la mémoire utilisée par le moteur crypto
    
    String calculatedHash = "";                            // Chaîne pour stocker la version lisible (Hex) du hash
    for(int i=0; i<32; i++) {                              // Parcours chaque octet du hash binaire
        if(resHash[i] < 16) calculatedHash += "0";         // Ajoute un '0' initial si le nombre est < 16 (formatage)
        calculatedHash += String(resHash[i], HEX);         // Convertit l'octet en hexadécimal et l'ajoute à la chaîne
    }
    Serial.println("[ESP32] Hash calcule : " + calculatedHash); // Affiche le hash calculé dans la console pour débogage

    if (calculatedHash.equalsIgnoreCase(expectedSha256)) { // Compare le hash calculé avec celui envoyé par le serveur (JSON)
        showStatus("SECURITE OK", "SHA256 VALIDE");        // Affiche succès : l'intégrité du fichier est confirmée
        delay(1000);                                       // Petite pause visuelle
        
        if (Update.end(true)) {                            // Finalise la mise à jour et active la nouvelle partition (Boot Flag)
            saveHash(expectedSha256);                      // Sauvegarde la nouvelle empreinte en mémoire permanente
            showStatus("MAJ REUSSIE", "REDEMARRAGE...");   // Informe l'utilisateur du succès total
            delay(1000);                                   // Pause de sécurité avant redémarrage
            ESP.restart();                                 // Redémarre l'ESP32 sur le nouveau firmware
        }
    } else {                                               // Si les empreintes ne correspondent pas (Fichier corrompu/Piraté)
        Update.end(false);                                 // Annule tout, vide la partition OTA (Rollback implicite)
        showStatus("ALERTE SECURITE", "HASH INVALID!");    // Alerte visuelle : Tentative d'injection détectée
        delay(2000);                                       // Temps de lecture
        showStatus("REJET MAJ", "RETOUR VERSION...");      // Confirme le rejet de la mise à jour
        delay(2000);                                       // Temps de lecture
        showStatus(line1_text, line2_text);                // Retour à l'affichage du firmware actuel (non modifié)
    }
}

// ============================================
// FONCTION 2 : VERIFICATION PERIODIQUE
// ============================================
// ============================================
// FONCTION 2 : VERIFICATION PERIODIQUE
// ============================================
void checkUpdate() {
    WiFiClientSecure client;                                       // Crée le client WiFi pour la connexion chiffrée (SSL/TLS)
    client.setCACert(CA_CERT_PEM);                                 // Charge le certificat racine pour authentifier le serveur
    client.setTimeout(10000);                                      // Définit un timeout de 10s pour éviter les faux échecs (latence)
    
    HTTPClient http;                                               // Crée l'objet pour gérer le protocole HTTP
    String url = "https://" + String(serverIP) + ":" + String(serverPort) + "/firmware/latest"; // Construit l'URL de l'API de vérification
    
    Serial.println("\n--- [ESP32] Verification Update ---");       // Affiche un log de début de vérification

    if (http.begin(client, url)) {                                 // Initie la connexion sécurisée vers l'URL cible
        http.addHeader("x-api-key", apiKey);                       // Ajoute la clé API dans l'en-tête pour l'authentification
        int httpCode = http.GET();                                 // Envoie la requête GET et récupère le code de réponse
        
        if (httpCode == 200) {                                     // Si le serveur répond "200 OK" (Succès)
            // --- CAS SUCCES ---
            String payload = http.getString();                     // Récupère le corps de la réponse (le JSON) sous forme de texte
            DynamicJsonDocument doc(1024);                         // Alloue un buffer mémoire pour traiter le JSON
            DeserializationError error = deserializeJson(doc, payload); // Convertit le texte en objet JSON exploitable

            if (!error) {                                          // Si le JSON est valide (pas d'erreur de syntaxe)
                String serverHash = doc["sha256"].as<String>();    // Extrait l'empreinte SHA-256 de la nouvelle version
                String fname = doc["filename"].as<String>();       // Extrait le nom du fichier binaire à télécharger
                
                if (fname == "null" || fname.length() == 0) {      // Vérifie que le nom de fichier n'est pas vide ou invalide
                     http.end(); return;                           // Si invalide, on ferme la connexion et on arrête tout
                }
                
                if (serverHash != storedHash) {                    // Compare l'empreinte reçue avec celle stockée en mémoire
                    Serial.println("=> Changement detecte !");     // Log qu'une mise à jour est disponible
                    http.end();                                    // Ferme la connexion JSON pour libérer la RAM avant le téléchargement
                    performSecureUpdate(fname, serverHash);        // Lance la fonction critique de mise à jour et vérification
                } else {
                    Serial.println("=> Pas de changement.");       // Log que le firmware est déjà à jour
                }
            }
        } else {
            // --- CAS ERREUR ---
            Serial.printf("[ERREUR] Code HTTP: %d\n", httpCode);   // Affiche le code d'erreur (ex: 404, 500, ou négatif)
            
            if (httpCode < 0) {                                    // Si le code est négatif (ex: -1), c'est une erreur de connexion/SSL
                Serial.println("Probable erreur SSL (Certificat rejete)"); // Log spécifique pour le débogage
                
                // AJOUT POUR LA DEMO : AFFICHER L'ERREUR SUR LCD
                showStatus("ERREUR RESEAU", "OU SSL REJETE !");    // Affiche l'alerte de sécurité sur l'écran LCD
                delay(3000);                                       // Laisse le message affiché 3 secondes pour lecture
                showStatus(line1_text, line2_text);                // Rétablit l'affichage normal (Nom de la version actuelle)
            }
        }
        http.end();                                                // Ferme proprement la connexion HTTP pour éviter les fuites mémoire
    } else {
        Serial.println("Impossible de lancer la connexion");       // Log si le client HTTP n'a pas pu être initialisé
    }
}

// ============================================
// SETUP
// ============================================
// ============================================
// SETUP : INITIALISATION MATÉRIEL & SÉCURITÉ
// ============================================
void setup() {
    Serial.begin(115200);                                  // Initialise la communication série pour le débogage (Vitesse 115200 bauds)
    Serial.println("\n\n--- BOOT ESP32 SECURE ---");       // Affiche un message de démarrage dans la console

    pinMode(LED_PIN, OUTPUT);                              // Configure la broche de la LED en mode sortie pour le clignotement
    
    Wire.begin(21, 22);                                    // Initialise le bus I2C sur les broches spécifiques ESP32 (SDA=21, SCL=22)
    lcd.init();                                            // Initialise l'écran LCD connecté via I2C
    lcd.backlight();                                       // Active le rétroéclairage de l'écran pour l'affichage
    
    showStatus(line1_text, line2_text);                    // Affiche le nom de la version actuelle (ex: "NEW SAISON") sur l'LCD
    
    WiFi.mode(WIFI_STA);                                   // Configure le WiFi en mode Station (Client) pour se connecter à une box
    WiFi.begin(ssid, password);                            // Lance la tentative de connexion avec les identifiants fournis
    
    Serial.print("Connexion Wifi");                        // Log de début de connexion
    while (WiFi.status() != WL_CONNECTED) {                // Boucle bloquante tant que le WiFi n'est pas connecté
        delay(500);                                        // Pause de 500ms entre chaque vérification
        Serial.print(".");                                 // Affiche un point pour montrer l'activité
    }
    Serial.println("\n[WIFI] Connecte !");                 // Confirme la connexion réseau réussie
    
    // --- SYNCHRO HEURE (Natif ESP32) ---
    // C'est CRITIQUE : Sans l'heure exacte, la validation du certificat SSL échouera (Date invalide)
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov");  // Configure le serveur NTP pour récupérer l'heure UTC+1 (3600s offset)
    Serial.print("Attente NTP");                           // Log de début de synchro
    struct tm timeinfo;                                    // Structure pour stocker les informations de temps
    while(!getLocalTime(&timeinfo)){                       // Boucle bloquante tant que l'heure n'est pas récupérée du serveur NTP
        Serial.print(".");                                 // Affiche un point d'attente
        delay(500);                                        // Pause pour ne pas saturer le réseau
    }
    Serial.println("\n[SYSTEM] Heure OK !");               // Confirme que l'heure est synchronisée (SSL est maintenant possible)
    // -----------------------------------

    storedHash = readHash();                               // Lit l'empreinte SHA-256 de la version actuelle stockée en EEPROM
    Serial.println("Hash en memoire : " + storedHash);     // Affiche le hash courant pour vérification
}

// ============================================
// LOOP : TÂCHES DE FOND (NON BLOQUANTES)
// ============================================
void loop() {
    unsigned long currentMillis = millis();                // Récupère le temps écoulé depuis le démarrage (pour les tâches sans delay)
    
    // --- Animation LED (Preuve visuelle de la version) ---
    // Utilise l'opérateur modulo (%) pour créer un cycle de clignotement basé sur 'flashSpeed'
    if (currentMillis % (flashSpeed * 2) < flashSpeed) {   // Si on est dans la première moitié du cycle
        digitalWrite(LED_PIN, HIGH);                       // Allume la LED (État Haut)
    } else {                                               // Si on est dans la seconde moitié
        digitalWrite(LED_PIN, LOW);                        // Éteint la LED (État Bas)
    }

    // --- Timer de Vérification OTA (Toutes les 30s) ---
    if (currentMillis - lastCheck >= checkInterval) {      // Vérifie si 30 secondes (checkInterval) se sont écoulées
        lastCheck = currentMillis;                         // Réinitialise le compteur de temps immédiatement
        if (WiFi.status() == WL_CONNECTED) {               // Vérifie si le WiFi est toujours actif avant de lancer la requête
            checkUpdate();                                 // Lance la procédure de vérification de mise à jour sécurisée
        }
    }
}