from flask import Flask, request, jsonify
import cv2
import numpy as np
import requests as req
import tensorflow as tf
import json
import base64
import time

app = Flask(__name__)

# ip-ul unde porneste server-ul de streaming web
ESP32_IP          = "192.168.1.135"
ESP32_STREAM_PORT = 81
ESP32_API_PORT    = 82

# incarca modelul custom
print("Incarc modelul...")
model = tf.keras.models.load_model('model_fructe.keras')

# pentru a sti maparea label - indice
with open('class_indices.json', 'r') as f:
    class_indices = json.load(f)
classes = {v: k for k, v in class_indices.items()}
print(f"Clase: {classes}")
print("Model incarcat")

def clasificare_imagine(img):
    img_resized = cv2.resize(img, (96, 96))
    img_norm    = img_resized.astype(np.float32) / 255.0
    # modelul asteapta mereu un batch, chiar daca este o singura imagine
    img_input   = np.expand_dims(img_norm, axis=0)
    predictions = model.predict(img_input, verbose=0)
    # gasim clasa cu probabilitate maxima
    class_idx   = np.argmax(predictions[0])
    confidence  = round(float(predictions[0][class_idx]) * 100, 1)
    label       = classes[class_idx]

    # Daca confidence e prea mic → empty
    if confidence < 70.0:
        label = 'empty'

    return label, confidence

HTML = '''
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Clasificare Fructe</title>
    <style>
        body { 
            font-family: Arial; 
            text-align: center; 
            background: #1a1a2e;
            color: white;
            padding: 20px;
        }
        h1 { color: #e94560; }
        img#stream { 
            width: 400px; 
            border: 3px solid #e94560;
            border-radius: 10px;
        }
        button {
            margin-top: 20px;
            padding: 15px 40px;
            font-size: 20px;
            background: #e94560;
            color: white;
            border: none;
            border-radius: 10px;
            cursor: pointer;
        }
        button:hover { background: #c73652; }
        #rezultat {
            margin-top: 20px;
            font-size: 28px;
            font-weight: bold;
            padding: 20px;
            background: #16213e;
            border-radius: 10px;
            min-height: 70px;
        }
        #snapshot {
            width: 400px;
            border: 3px solid #1D9E75;
            border-radius: 10px;
            margin-top: 10px;
            display: none;
        }
    </style>
</head>
<body>
    <h1>ESP32-CAM Clasificare Fructe</h1>
    
    <h3>Stream live:</h3>
    <img id="stream" /><br>
    
    <button onclick="clasificare()"> Clasifica</button>
    
    <h3>Ultima captura:</h3>
    <img id="snapshot" />
    
    <div id="rezultat">Apasa butonul pentru clasificare</div>

    <script>
        const ESP32_STREAM = "http://192.168.1.135:81";

        function updateStream() {
            document.getElementById('stream').src = 
                ESP32_STREAM + '/jpeg?' + Date.now();
            setTimeout(updateStream, 300);
        }
        updateStream();

        async function clasificare() {
            document.getElementById('rezultat').innerHTML = 
                'Se clasifica...';
            
            try {
                const response = await fetch('/clasificare', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({})
                });
                
                const data = await response.json();
                
                if (data.snapshot) {
                    const img = document.getElementById('snapshot');
                    img.src = 'data:image/jpeg;base64,' + data.snapshot;
                    img.style.display = 'block';
                }
                
                if (data.detectii && data.detectii.length > 0) {
                    const d = data.detectii[0];
                    let emoji = '';
                    let mesaj = '';

                    if (d.label === 'apple') {
                        mesaj = 'Mar detectat';
                    } else if (d.label === 'lemon') {
                        mesaj = 'Lamaie detectata';
                    } else {
                        mesaj = 'Nimic detectat';
                    }

                    document.getElementById('rezultat').innerHTML = 
                        emoji + ' ' + mesaj + ' — ' + d.confidence + '%';
                } else {
                    document.getElementById('rezultat').innerHTML = 
                        'Nimic detectat';
                }
                
            } catch(e) {
                document.getElementById('rezultat').innerHTML = 
                    'Eroare: ' + e.message;
            }
        }
    </script>
</body>
</html>
'''

# inregistreaza o ruta in flask
# cand cineva acceseaza /, se ruleaza fct index, care afiseaza pagina
@app.route('/')
def index():
    return HTML

# cand browser-ul apasa butonul, se apeleaza aceasta functie
@app.route('/clasificare', methods=['POST'])
def clasificare():
    try:
        print(f"Accesez http://{ESP32_IP}:{ESP32_API_PORT}/jpeg")
        response = req.get(
            f'http://{ESP32_IP}:{ESP32_API_PORT}/jpeg',
            timeout=5
        )

        raw = response.content
        print(f"Primit: {len(raw)} bytes")

        # converteste RGB565
        # camera rhyx trimite in ordine inversa bytes
        if len(raw) == 153600:      # QVGA 320x240
            raw_swapped = bytes([raw[i+1] if i%2==0 else raw[i-1] 
                                for i in range(len(raw))])
            img = np.frombuffer(raw_swapped, dtype=np.uint16).reshape((240, 320))
            img = cv2.cvtColor(
                img.view(np.uint8).reshape(240, 320, 2),
                cv2.COLOR_BGR5652BGR
            )

        print(f"Imagine OK: {img.shape}")

        # salveaza local
        cv2.imwrite('ultima_captura.jpg', img)

        # Converteste la base64
        _, buffer = cv2.imencode('.jpg', img)
        snapshot_b64 = base64.b64encode(buffer).decode('utf-8')

        # Clasificare
        label, confidence = clasificare_imagine(img)
        print(f"Detectat: {label} ({confidence}%)")
        detectii = [{'label': label, 'confidence': confidence}]
        if label == 'apple':
            req.get(
			    f'http://{ESP32_IP}:{ESP32_API_PORT}/servo?grade=45',
			    timeout=3
			)
            time.sleep(1)
            req.get(
                f'http://{ESP32_IP}:{ESP32_API_PORT}/servo?grade=90',
                timeout=3
			)
            print("Poarta ridicata!")
            time.sleep(3)
            req.get(
			    f'http://{ESP32_IP}:{ESP32_API_PORT}/servo?grade=0',
			    timeout=3
			)
            print("Poarta coborata!")

        # trimite la OLED
        try:
            req.get(
                f'http://{ESP32_IP}:{ESP32_API_PORT}'
                f'/rezultat?label={label}&conf={confidence}',
                timeout=3
            )
            print(f"Trimis la OLED: {label} ({confidence}%)")
        except Exception as e:
            print(f"Nu s-a putut trimite la OLED: {e}")

        return jsonify({
            'detectii': detectii,
            'snapshot': snapshot_b64
        })

    except Exception as e:
        print(f"Eroare: {e}")
        return jsonify({'eroare': str(e), 'detectii': []})

if __name__ == '__main__':
    print(f"Server pornit la http://localhost:5000")
    print(f"ESP32 stream: http://{ESP32_IP}:{ESP32_STREAM_PORT}")
    print(f"ESP32 API:    http://{ESP32_IP}:{ESP32_API_PORT}")
    app.run(host='0.0.0.0', port=5000, debug=True)