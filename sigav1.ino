

/*
PROJECT: Sistema Integral de Gestion Ambiental (SIGA) v1.4
AUTHOR: @rafasollero  
DESCRIPCTION: Autonomus controller for controlled place (Invernadero/Lab).
6 sensors, 4 actuators, Watchdog & security logic.
*/

// libraries
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal.h>

// pin definition
#define PIN_SENSOR_LUZ A0      // LDR
#define PIN_SENSOR_HUM_SUELO A1 // Higrometro capacitivo
#define PIN_SENSOR_GAS A2      // Sensor MQ-2 (Gas)
#define PIN_SENSOR_TEMP A3     // LM35 
#define PIN_PIR_MOVIMIENTO 2   // Sensor PIR HC-SR501
#define PIN_BTN_EMERGENCIA 3   // Interrupcion externa

#define PIN_LCD_RS 7
#define PIN_LCD_E  8
#define PIN_LCD_D4 11
#define PIN_LCD_D5 19 
#define PIN_LCD_D6 13 
#define PIN_LCD_D7 18 

#define PIN_RELE_VENTILADOR 4  // Control de ventilacion
#define PIN_RELE_BOMBA 5       // Control de riego
#define PIN_RELE_LUCES 6       // Iluminacion artificial
#define PIN_SERVO_VENTANA 9    // Servo para apertura de techo
#define PIN_BUZZER 10          // Alarma sonora piezoelectrica
#define PIN_LED_ESTADO 13      // Indicador de heartbeat
#define PIN_LED_ALERTA 12      // Indicador rojo de fallo

//variables
Servo servomotorVentana;
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

// Calibration
const int UMBRAL_LUZ_MIN = 300;
const int UMBRAL_LUZ_MAX = 800;
const int UMBRAL_HUMEDAD_MIN = 400;
const int UMBRAL_GAS_PELIGRO = 150;
const int TEMP_MAXIMA = 30;
const int TEMP_MINIMA = 18;

// systemstate
bool sistemaActivo = true;
bool modoEmergencia = false;
bool ventiladorEncendido = false;
bool bombaEncendida = false;
bool lucesEncendidas = false;
int anguloVentana = 0;
unsigned long ultimoTiempoLectura = 0;
const long INTERVALO_LECTURA = 2000; // 2 s

// suavizado de señal
const int NUM_LECTURAS = 10;
int lecturasLuz[NUM_LECTURAS];
int lecturasTemp[NUM_LECTURAS];
int indiceLectura = 0;
long totalLuz = 0;
long totalTemp = 0;

// SETUP
void setup() {
  // Inicial serial
  Serial.begin(115200);
  Serial.println(F("[INIT] Iniciando Sistema SIGA v1.4..."));

  // INPUT PIN COFIGURATION
  pinMode(PIN_SENSOR_LUZ, INPUT);
  pinMode(PIN_SENSOR_HUM_SUELO, INPUT);
  pinMode(PIN_SENSOR_GAS, INPUT);
  pinMode(PIN_SENSOR_TEMP, INPUT);
  pinMode(PIN_PIR_MOVIMIENTO, INPUT);
  pinMode(PIN_BTN_EMERGENCIA, INPUT_PULLUP);

  //OUTPUT PIN CONFIGURATION
  pinMode(PIN_RELE_VENTILADOR, OUTPUT);
  pinMode(PIN_RELE_BOMBA, OUTPUT);
  pinMode(PIN_RELE_LUCES, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_ESTADO, OUTPUT);
  pinMode(PIN_LED_ALERTA, OUTPUT);

  // TURNING OFF ALL DEVICES FOR SECURITY
  digitalWrite(PIN_RELE_VENTILADOR, HIGH); // inverse common logic for servo
  digitalWrite(PIN_RELE_BOMBA, HIGH);
  digitalWrite(PIN_RELE_LUCES, HIGH);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ALERTA, LOW);

  //Initialize servo and lcd
  servomotorVentana.attach(PIN_SERVO_VENTANA);
  moverVentana(0); // Close window
  lcd.begin(16,2);


  //Security interrup
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_EMERGENCIA), rutinaEmergencia, FALLING);

  // Inicializar arrays de promedios en 0
  for (int i = 0; i < NUM_LECTURAS; i++) {
    lecturasLuz[i] = 0;
    lecturasTemp[i] = 0;
  }

  Serial.println(F("[INIT] Hardware configurado correctamente."));
  lcd.setCursor(0,0);
  lcd.print("SISTEMA LISTO");
  delay(2000);
  lcd.clear();
}

// Loop
void loop() {
  // verify whether if we are on emergency state for security reasons
  if (modoEmergencia) {
    parpadeoAlerta();
    return; // Close loop if any issue is reported
  }

  unsigned long tiempoActual = millis();

  // multitasking
  if (tiempoActual - ultimoTiempoLectura >= INTERVALO_LECTURA) {
    ultimoTiempoLectura = tiempoActual;

    // 1. data reader
    int rawLuz = analogRead(PIN_SENSOR_LUZ);
    int rawHumedad = analogRead(PIN_SENSOR_HUM_SUELO);
    int rawGas = analogRead(PIN_SENSOR_GAS);
    int rawTemp = analogRead(PIN_SENSOR_TEMP);
    bool movimiento = digitalRead(PIN_PIR_MOVIMIENTO);

    // 2. compile medium values
    int luzPromedio = suavizarLecturaLuz(rawLuz);
    int tempPromedio = suavizarLecturaTemp(rawTemp);

    // 3. ambiental control logic
    controlarTemperatura(tempPromedio);
    controlarIluminacion(luzPromedio);
    controlarRiego(rawHumedad);
    verificarCalidadAire(rawGas);
    verificarSeguridad(movimiento);

    // 4. interfaz update
    actualizarLCD(tempPromedio, rawHumedad, luzPromedio);
    reportarSerial(tempPromedio, rawHumedad, luzPromedio, rawGas);
  }

  // heartbeat
  digitalWrite(PIN_LED_ESTADO, (millis() / 500) % 2);
}

// aux functions

void controlarTemperatura(int tempVal) {
  // lecture value to celsius degrees (Aprox for LM35)
  float voltaje = tempVal * (5.0 / 1023.0);
  float temperaturaC = voltaje * 100.0;

  if (temperaturaC > TEMP_MAXIMA) {
    if (!ventiladorEncendido) {
      digitalWrite(PIN_RELE_VENTILADOR, LOW); // turn on relay
      ventiladorEncendido = true;
      moverVentana(90); // open window to the maximun range
      Serial.println(F("[ACCION] Temp Alta: Ventilador ON / Ventana ABIERTA"));
    }
  } else if (temperaturaC < TEMP_MINIMA) {
    if (ventiladorEncendido) {
      digitalWrite(PIN_RELE_VENTILADOR, HIGH); // turn off relay
      ventiladorEncendido = false;
      moverVentana(0); // close down window
      Serial.println(F("[ACCION] Temp Baja: Ventilador OFF / Ventana CERRADA"));
    }
  }
}

void controlarIluminacion(int luzVal) {
  
  if (luzVal < UMBRAL_LUZ_MIN && !lucesEncendidas) {
    digitalWrite(PIN_RELE_LUCES, LOW); // turn on
    lucesEncendidas = true;
    Serial.println(F("[ACCION] Oscuridad detectada: Luces ON"));
  } else if (luzVal > UMBRAL_LUZ_MAX && lucesEncendidas) {
    digitalWrite(PIN_RELE_LUCES, HIGH); // turn off
    lucesEncendidas = false;
    Serial.println(F("[ACCION] Luz suficiente: Luces OFF"));
  }
}

void controlarRiego(int humVal) {
  // Sensor capacitivo: valor alto = seco, bajo = humedo 
  if (humVal > UMBRAL_HUMEDAD_MIN && !bombaEncendida) {
    digitalWrite(PIN_RELE_BOMBA, LOW); // turn on bomba
    bombaEncendida = true;
    Serial.println(F("[ACCION] Suelo seco: Riego INICIADO"));
  } else if (humVal < (UMBRAL_HUMEDAD_MIN - 100) && bombaEncendida) {
    digitalWrite(PIN_RELE_BOMBA, HIGH); // turn off bomba
    bombaEncendida = false;
    Serial.println(F("[ACCION] Humedad optima: Riego DETENIDO"));
  }
}

void verificarCalidadAire(int gasVal) {
  if (gasVal > UMBRAL_GAS_PELIGRO) {
    tone(PIN_BUZZER, 1000, 200);
    digitalWrite(PIN_LED_ALERTA, HIGH);
    Serial.println(F("[ALERTA] GAS DETECTADO! VENTILANDO..."));
    // force autonomous ventilation
    digitalWrite(PIN_RELE_VENTILADOR, LOW);
    moverVentana(180);
  } else {
    digitalWrite(PIN_LED_ALERTA, LOW);
  }
}

void verificarSeguridad(bool mov) {
  static unsigned long ultimoMovimiento = 0;
  if (mov) {
    ultimoMovimiento = millis();
    Serial.println(F("[SEGURIDAD] Movimiento detectado en el area"));
  }
}

void actualizarLCD(int t, int h, int l) {
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(t); lcd.print(" H:"); lcd.print(h);
  lcd.setCursor(0, 1);
  lcd.print("L:"); lcd.print(l);
  if (bombaEncendida) lcd.print(" RIEGO ON");
  else lcd.print(" OK      ");
}

void moverVentana(int angulo) {
  angulo = constrain(angulo, 0, 180);
  servomotorVentana.write(angulo);
  anguloVentana = angulo;
}


int suavizarLecturaLuz(int nuevaLectura) {
  totalLuz = totalLuz - lecturasLuz[indiceLectura];
  lecturasLuz[indiceLectura] = nuevaLectura;
  totalLuz = totalLuz + lecturasLuz[indiceLectura];

  return totalLuz / NUM_LECTURAS;
}

int suavizarLecturaTemp(int nuevaLectura) {
  totalTemp = totalTemp - lecturasTemp[indiceLectura];
  lecturasTemp[indiceLectura] = nuevaLectura;
  totalTemp = totalTemp + lecturasTemp[indiceLectura];

  indiceLectura = indiceLectura + 1;
  if (indiceLectura >= NUM_LECTURAS) {
    indiceLectura = 0;
  }
  return totalTemp / NUM_LECTURAS;
}

void rutinaEmergencia() {
  // Interruption
  modoEmergencia = !modoEmergencia;
}

void parpadeoAlerta() {
  
  digitalWrite(PIN_RELE_VENTILADOR, HIGH); // turn off all the stuff
  digitalWrite(PIN_RELE_BOMBA, HIGH);
  digitalWrite(PIN_RELE_LUCES, HIGH);

  digitalWrite(PIN_LED_ALERTA, HIGH);
  tone(PIN_BUZZER, 2000);
  delay(500);
  digitalWrite(PIN_LED_ALERTA, LOW);
  noTone(PIN_BUZZER);
  delay(500);
  Serial.println(F("!!! EMERGENCIA ACTIVADA MANUALMENTE !!!"));
}

void reportarSerial(int t, int h, int l, int g) {
    Serial.print("DATA >> Temp: ");
    Serial.print(t);
    Serial.print(" | HumSuelo: ");
    Serial.print(h);
    Serial.print(" | Luz: ");
    Serial.print(l);
    Serial.print(" | Gas: ");
    Serial.println(g);
}