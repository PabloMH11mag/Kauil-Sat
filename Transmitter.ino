#include <Wire.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <Adafruit_MLX90640.h>
#include <Adafruit_ADS1X15.h>
#include <DFRobot_BMI160.h>

// ========= NRF24 =========
RF24 radio(4, 5);
const byte direccion[6] = "00001";

// ========= SENSORES =========
Adafruit_MLX90640 mlx;
float frame[32 * 24];

Adafruit_ADS1115 ads;

DFRobot_BMI160 bmi160;
int16_t accelGyro[6];

// ========= FRAME ID =========
unsigned long frameID = 0;

// ========= TIEMPO INICIO =========
unsigned long tiempoInicioFrame = 0;

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

float bateria = 90;

void setup() {

  Serial.begin(921600);
  Wire.begin(21, 22);

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(direccion);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();

  mlx.begin();
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_4_HZ);

  ads.begin(0x48);
  bmi160.I2cInit(0x69);

  Serial.println("TRANSMISOR LISTO");
}

void loop() {

  // ========= INICIO DE FRAME (AQUÍ SE REINICIA TODO) =========
  tiempoInicioFrame = millis();

  mlx.getFrame(frame);

  int indice = 0;

  for (int numero = 0; numero < 154; numero++) {

    paquete.tipo = 0;
    paquete.numeroPaquete = numero;
    paquete.frameID = frameID;

    for (int i = 0; i < 5; i++) {

      if (indice < 768) paquete.datos[i] = frame[indice];
      else paquete.datos[i] = 0;

      indice++;
    }

    radio.write(&paquete, sizeof(paquete));
    delay(2);
  }

  // ========= IMU =========
  bmi160.getAccelGyroData(accelGyro);

  imu1.tipo = 1;
  imu1.ax = accelGyro[0] / 16384.0;
  imu1.ay = accelGyro[1] / 16384.0;
  imu1.az = accelGyro[2] / 16384.0;
  radio.write(&imu1, sizeof(imu1));

  imu2.tipo = 2;
  imu2.gx = accelGyro[3] / 131.0;
  imu2.gy = accelGyro[4] / 131.0;
  imu2.gz = accelGyro[5] / 131.0;
  radio.write(&imu2, sizeof(imu2));

  // ========= TEMP =========
  int16_t adc0 = ads.readADC_SingleEnded(0);
  float voltaje = adc0 * 0.1875 / 1000.0;

  lm35.tipo = 3;
  lm35.temperatura = voltaje * 100.0;

  radio.write(&lm35, sizeof(lm35));

  // ========= BATERÍA =========
  bateriaPacket.tipo = 4;
  bateriaPacket.porcentaje = bateria;

  radio.write(&bateriaPacket, sizeof(bateriaPacket));

  Serial.println("FRAME ENVIADO");

  frameID++;   // 🔥 IMPORTANTE: nuevo frame

  delay(100);  // pausa ENTRE frames
}