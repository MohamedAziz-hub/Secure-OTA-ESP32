# 🛡️ Secure OTA Firmware Update System (ESP32)

Ce projet implémente une solution complète de mise à jour à distance (Over-The-Air) pour systèmes embarqués critiques (ESP32). L'architecture met l'accent sur la **sécurité en profondeur** (Defense in Depth) et la **résilience**, garantissant qu'aucun appareil ne se retrouve hors service ("bricked") suite à une mise à jour corrompue ou malveillante.

## 🚀 Fonctionnalités Clés

Le système repose sur trois piliers de sécurité fondamentaux :

1.  **Authentification Serveur (SSL/TLS)** : Le client ESP32 authentifie le serveur via un certificat X.509 (`.pem`) embarqué. Toute tentative d'attaque *Man-in-the-Middle* est immédiatement rejetée avant l'établissement de la connexion.
2.  **Intégrité des Données (SHA-256)** : Chaque binaire est signé numériquement. L'ESP32 calcule l'empreinte SHA-256 à la volée (via accélération matérielle mbedTLS) pendant le téléchargement et la compare à la signature autorisée.
3.  **Mise à Jour Atomique (Rollback)** : Utilisation du partitionnement mémoire (A/B Partitioning). Le nouveau firmware est écrit dans une partition inactive. Il n'est activé (Boot Switch) **que** si toutes les vérifications de sécurité sont validées. Dans le cas contraire, le système maintient la version précédente active.

## 🏗️ Architecture du Système

Le projet se compose de deux entités distinctes :

*   **Serveur de Provisioning (Python/Flask)** :
    *   Héberge les binaires de firmware cryptés.
    *   Délivre les métadonnées de sécurité (Version, URL, Signature SHA-256) via une API JSON sécurisée.
    *   Gère l'authentification par clé API.
*   **Client IoT (ESP32 / C++)** :
    *   Gestionnaire de connexion WiFi et synchronisation NTP (nécessaire pour la validation SSL).
    *   Moteur de mise à jour OTA sécurisé avec gestion des erreurs critiques.

## 📦 Cycle de Vie des Firmwares (Démonstration)

Le projet démontre l'évolution d'un produit à travers trois versions distinctes, prouvant la flexibilité du système OTA :

*   **Firmware V1 (Base)** : Version initiale déployée en usine.
*   **Firmware V2 (Promo)** : Mise à jour déployant une nouvelle interface LCD et une nouvelle fréquence de LED.
*   **Firmware V3 (Liquidation)** : Mise à jour finale modifiant la logique métier (comportement Stroboscope).

L'appareil passe de la V1 à la V3 de manière autonome et sécurisée.

## 🛡️ Scénarios de Sécurité & Gestion d'Erreurs

Le point fort de ce projet est sa capacité à gérer les attaques ou les défaillances réseau.

### Scénario 1 : Attaque parusurpation d'identité (Faux Serveur)
Si l'ESP32 est redirigé vers un serveur malveillant (DNS Spoofing) possédant une adresse IP identique mais ne possédant pas la clé privée légitime :
*   **Réaction :** L'ESP32 détecte l'invalidité du certificat lors du handshake SSL.
*   **Résultat :** La connexion est coupée immédiatement (`Connection Refused`). Aucune donnée n'est téléchargée. **L'appareil reste sur sa version stable.**

### Scénario 2 : Injection de Malware ou Corruption (Hash Mismatch)
Si un attaquant parvient à injecter un binaire modifié pendant le transit, ou si le fichier est corrompu :
*   **Réaction :** L'ESP32 télécharge le fichier en zone tampon, calcule son empreinte SHA-256 et constate la divergence avec la signature officielle.
*   **Résultat :** La commande `Update.end(false)` est déclenchée. La partition de mise à jour est purgée. L'écran affiche **"ALERTE SÉCURITÉ : HASH INVALID"**. **L'appareil refuse l'installation et conserve la version précédente.**

## 🛠️ Installation et Configuration

### Prérequis
*   ESP32 DevKit V1
*   Python 3.x (Flask)
*   OpenSSL (pour la génération des certificats)

### Structure du projet
Secure-OTA-ESP32/
├── Device_ESP32/ # Codes sources C++ (V1, V2, V3)
└── Server/ # Serveur Flask et Binaires
├── firmwares/ # Stockage des versions (.bin)
├── cert.pem # Certificat public
└── ota_server.py # Script serveur

---
*Ce projet a été réalisé dans le cadre d'une démonstration technique sur la cybersécurité des objets connectés (IoT).*
