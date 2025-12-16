from flask import Flask, jsonify, request, send_file
import os
import hashlib
import ssl

app = Flask(__name__)

API_KEY = "OTA_KEY_2025"
FOLDER = "firmwares"
SERVER_IP = "10.203.181.186"  # ⚠️ Votre IP
PORT = 3000

# Mapping des fichiers
FIRMWARES = {
    "v1": "v1.0.bin",
    "v2": "v2.0.bin"
}

# 🎮 CHANGEZ CECI POUR TESTER LA MAJ OU LE ROLLBACK
LATEST_VERSION = "v2"


# LATEST_VERSION = "v1"

def get_sha256(filepath):
    if not os.path.exists(filepath): return None
    h = hashlib.sha256()
    with open(filepath, 'rb') as f:
        # Lecture par bloc pour gérer les gros fichiers
        for chunk in iter(lambda: f.read(4096), b''): h.update(chunk)
    return h.hexdigest()


@app.route('/firmware/latest')
def latest():
    # Sécurité API Key
    if request.headers.get('x-api-key') != API_KEY:
        return jsonify(error="Auth failed"), 401

    fname = FIRMWARES.get(LATEST_VERSION)
    if not fname: return jsonify(error="Config error"), 500

    fpath = os.path.join(FOLDER, fname)
    file_hash = get_sha256(fpath)

    if not file_hash:
        return jsonify(error="File missing"), 404

    # On envoie le SHA256 que l'ESP utilisera pour valider le téléchargement
    return jsonify({
        "version": LATEST_VERSION,
        "filename": fname,
     #  "sha256": file_hash
       "sha256": "ceci_est_un_faux_hash_pirate_123456789"
    })


@app.route('/firmware/download/<fname>')
def download(fname):
    # Protection simple contre Path Traversal
    if ".." in fname or "/" in fname: return "Error", 400

    fpath = os.path.join(FOLDER, fname)
    if os.path.exists(fpath):
        return send_file(fpath, as_attachment=True)
    return "Not Found", 404


if __name__ == '__main__':
    # Configuration SSL (HTTPS)
    # C'est ce qui permet à l'ESP de vérifier le certificat
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain('cert.pem', 'key.pem')

    print(f"🔒 Serveur OTA Sécurisé (HTTPS) sur {SERVER_IP}:{PORT}")
    app.run(host='0.0.0.0', port=PORT, ssl_context=context)