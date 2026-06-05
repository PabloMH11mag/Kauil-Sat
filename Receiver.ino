#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ========= NRF =========
RF24 radio(4, 5);
const byte direccion[6] = "00001";

// ========= DATOS =========
float frameRecibido[768];

float ax, ay, az;
float gx, gy, gz;
float temperaturaLM35;
float bateria;

// ========= LATENCIA =========
unsigned long tiempoInicioFrame = 0;
unsigned long latencia = 0;

unsigned long frameID_actual = 0;

// ========= PAQUETES =========
struct PaqueteImagen {
  byte tipo;
  byte numeroPaquete;
  float datos[5];
  unsigned long frameID;
};

struct IMU1 { byte tipo; float ax, ay, az; };
struct IMU2 { byte tipo; float gx, gy, gz; };
struct LM35Packet { byte tipo; float temperatura; };
struct BatteryPacket { byte tipo; float porcentaje; };

// ========= OBJETOS =========
PaqueteImagen paquete;
IMU1 imu1;
IMU2 imu2;
LM35Packet lm35;
BatteryPacket bateriaPacket;

void setup() {

  Serial.begin(921600);

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, direccion);
  radio.startListening();

  Serial.println("RECEPTOR LISTO");
}

void loop() {

  if (radio.available()) {

    byte buffer[32];
    radio.read(&buffer, sizeof(buffer));

    byte tipo = buffer[0];

    // ========= FRAME =========
    if (tipo == 0) {

      memcpy(&paquete, &buffer, sizeof(paquete));

      // 🔥 INICIO DE FRAME REAL
      if (paquete.numeroPaquete == 0) {
        tiempoInicioFrame = millis();
        frameID_actual = paquete.frameID;
      }

      int inicio = paquete.numeroPaquete * 5;

      for (int i = 0; i < 5; i++) {
        if (inicio + i < 768)
          frameRecibido[inicio + i] = paquete.datos[i];
      }

      // 🔥 FINAL DE FRAME
      if (paquete.numeroPaquete == 153 &&
          paquete.frameID == frameID_actual) {

        latencia = millis() - tiempoInicioFrame;

        enviarJSON();
      }
    }

    // ========= IMU =========
    if (tipo == 1) {
      memcpy(&imu1, &buffer, sizeof(imu1));
      ax = imu1.ax; ay = imu1.ay; az = imu1.az;
    }

    if (tipo == 2) {
      memcpy(&imu2, &buffer, sizeof(imu2));
      gx = imu2.gx; gy = imu2.gy; gz = imu2.gz;
    }

    // ========= TEMP =========
    if (tipo == 3) {
      memcpy(&lm35, &buffer, sizeof(lm35));
      temperaturaLM35 = lm35.temperatura;
    }

    // ========= BATERÍA =========
    if (tipo == 4) {
      memcpy(&bateriaPacket, &buffer, sizeof(bateriaPacket));
      bateria = bateriaPacket.porcentaje;
    }
  }
}

// ========= JSON =========
void enviarJSON() {

  Serial.print("{\"cam\":[");

  for (int i = 0; i < 768; i++) {
    Serial.print(frameRecibido[i], 1);
    if (i < 767) Serial.print(",");
  }

  Serial.print("],\"imu\":{");

  Serial.print("\"ax\":"); Serial.print(ax, 2); Serial.print(",");
  Serial.print("\"ay\":"); Serial.print(ay, 2); Serial.print(",");
  Serial.print("\"az\":"); Serial.print(az, 2); Serial.print(",");
  Serial.print("\"gx\":"); Serial.print(gx, 2); Serial.print(",");
  Serial.print("\"gy\":"); Serial.print(gy, 2); Serial.print(",");
  Serial.print("\"gz\":"); Serial.print(gz, 2);

  Serial.print("},\"tempCam\":");
  Serial.print(temperaturaLM35, 2);

  Serial.print(",\"bateria\":");
  Serial.print(bateria, 1);

  Serial.print(",\"latencia\":");
  Serial.print(latencia);

  Serial.println("}");
}