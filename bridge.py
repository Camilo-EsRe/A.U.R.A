import serial
import requests
import time

PUERTO_COM = 'COM11'
BAUD_RATE = 115200
N8N_URL = 'https://auraincietex.app.n8n.cloud/webhook-test/aura-inspect'

print("=== PUENTE A.U.R.A. INICIADO (CICLO DE 35 SEGUNDOS) ===")
print(f"Escuchando puerto {PUERTO_COM} y reenviando a {N8N_URL}\n")

try:
    ser = serial.Serial(PUERTO_COM, BAUD_RATE, timeout=1)
    time.sleep(2)
except Exception as e:
    print(f"❌ Error al abrir el puerto {PUERTO_COM}: {e}")
    exit()

while True:
    try:
        linea = ser.readline().decode('utf-8', errors='ignore').strip()
        
        if linea.startswith("START:"):
            tamano_bytes = int(linea.split(":")[1])
            print(f"\n[USB] Recibiendo foto de {tamano_bytes} bytes desde ESP32...")
            
            # Asegurar lectura completa de la imagen
            ser.timeout = 10
            buffer_imagen = ser.read(tamano_bytes)
            ser.timeout = 1
            
            with open("captura_actual.jpg", "wb") as f:
                f.write(buffer_imagen)

            print("[HTTP] Enviando foto a n8n...")
            respuesta = requests.post(
                N8N_URL,
                data=buffer_imagen,
                headers={'Content-Type': 'image/jpeg'},
                timeout=15
            )
            
            raw_veredicto = respuesta.text.strip().replace('"', '').replace("'", "")
            print(f"[N8N] Veredicto recibido: '{raw_veredicto}'")
            
            if "DANADO" in raw_veredicto.upper() or "DAÑADO" in raw_veredicto.upper():
                veredicto_enviar = "DANADO"
            elif "PASA" in raw_veredicto.upper():
                veredicto_enviar = "PASA"
            else:
                veredicto_enviar = "VACIO"

            ser.write(f"{veredicto_enviar}\n".encode('ascii'))
            print(f"[USB] Veredicto enviado al ESP32: {veredicto_enviar}")
            print("-" * 45)

            # Sincronizar reloj: 35s base + 2s extra si encendió LED/Buzzer
            segundos_espera = 37 if veredicto_enviar in ("PASA", "DANADO") else 35
            
            for seg in range(segundos_espera, 0, -1):
                print(f"\r⏳ Siguiente captura en: {seg:02d} segundos...   ", end="", flush=True)
                time.sleep(1)
            
            print("\r📸 Tomando fotografía...                      ")

    except KeyboardInterrupt:
        print("\n🛑 Puente A.U.R.A. detenido por el usuario.")
        ser.close()
        break
    except Exception as e:
        print(f"\n❌ Error durante la comunicación: {e}")
        time.sleep(1)