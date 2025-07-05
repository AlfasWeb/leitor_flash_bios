#include "flash.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==== Display ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// =================
// ==== Pins ====
#define ENCODER_PIN_A 32
#define ENCODER_PIN_B 33
#define BUTTON_MENU   13
#define BUTTON_SET    14
#define BUTTON_DOUBLE 15
#define BUTTON_HALF   16
#define BUTTON_ENC    17
#define LED_PIN       2       // Pino do LED de alta potência para o estroboscópio/lanterna
#define SENSOR_IR_PIN 4       // Pino do sensor infravermelho para medição de RPM

/*
GPIO	Observações
0	    -Influencia o boot (deve estar em HIGH para iniciar normalmente)
1	    -Saída UART0 TX — usado para upload/debug, evite usar se possível
3	    -Entrada UART0 RX — usado para upload/debug, evite usar
12	  -Afeta o boot — deve estar em nível adequado (LOW recomendado)
21-22 -SDA, SCL
34–39	-Apenas input (sem função de output)
6–11	-NÃO USAR — estão conectados internamente ao flash SPI
5,23,18,19,14 - SPI
*/

// ==== Menu ====
enum class Mode { HOME, FREQUENCY, RPM, LANTERN, VIBROMETER, TEST, ABOUT, CONFIG, NUM_MODES};
Mode currentMode = Mode::HOME;
Mode selectedMode = Mode::HOME;
bool inMenu = true;
bool inSubmenu = false;
bool inEncoder = false;

// ==== Variáveis do Vibrometro ====
enum class VibroState {VIBRO_HOME, VIBRO_IDLE, VIBRO_CALIB, VIBRO_CONFIG, VIBRO_MEASURE, VIBRO_RESULT};
VibroState vibroState = VibroState::VIBRO_HOME;

// Lista de mensagens
String lines[] = {
  "Agradecimento:",
  "Evandro Padilha",
  "Renato",
  "Vitor Santarosa",
  "Alex Penteado",
  "Bruno",
  "Epaminondas",
  "Fernando",
  "Fabio Camarinha",
  "Gabriela"
};
const int totalLines = sizeof(lines) / sizeof(lines[0]);
int topLineIndex = 0;  // Índice da linha superior visível
int lineShowMsg = 5;
// ==============

// ==== Temporizador ====
struct TimerMicros {
  unsigned long start;
  unsigned long duration;
  void startTimer(unsigned long d) {
    duration = d;
    start = micros();
  }
  bool isExpired() {
    return (micros() - start) >= duration;
  }
  bool isRunning() {
    return (micros() - start) < duration;
  }
};
TimerMicros msgTimer; //Pode criar quantos TimerMicros for necessário
TimerMicros fpmTest;

// ==== Vetores da Flash ====
std::vector<Config> imgLogo1;
std::vector<Config> imgLogo2;
std::vector<Config> imgLogo3;
std::vector<Config> dadosConfig;
std::vector<Config> displayConfig;
int numIdioma = 1;
String idioma = "";
bool saveData=false;
// ==============

bool checkButtonDebounce(uint8_t pin, bool &lastState, unsigned long &lastDebounceTime, unsigned long debounceDelayMicros = 200000) {
  bool currentState = digitalRead(pin);
  unsigned long now = micros();
  if (currentState == LOW && lastState == HIGH && (now - lastDebounceTime > debounceDelayMicros)) {
    lastDebounceTime = now;
    lastState = currentState;
    return true; // Botão pressionado com debounce válido
  }
  lastState = currentState;
  return false;
}

void exibirImagemDaFlash(uint32_t enderecoInicial, int largura, int altura, int offsetX, int offsetY) {
  int bytesPorLinha = largura / 8;
  uint8_t linhaBuffer[bytesPorLinha];

  for (int y = 0; y < altura; y++) {
    // Lê a linha da imagem diretamente da flash
    uint32_t enderecoLinha = enderecoInicial + (y * bytesPorLinha);
    readData(enderecoLinha, linhaBuffer, bytesPorLinha);

    // Desenha os pixels dessa linha
    for (int byteIndex = 0; byteIndex < bytesPorLinha; byteIndex++) {
      uint8_t b = linhaBuffer[byteIndex];
      for (int bit = 0; bit < 8; bit++) {
        int pixelX = offsetX + (byteIndex * 8) + (7 - bit); // MSB primeiro
        int pixelY = offsetY + y;

        if (pixelX >= 0 && pixelX < 128 && pixelY >= 0 && pixelY < 64) {
          bool isOn = b & (1 << bit);
          display.drawPixel(pixelX, pixelY, isOn ? WHITE : BLACK);
        }
      }
    }
  }

  display.display();  // Atualiza o display ao final
}

// ==== Setup dos botões ====
void setupButtons(){
  pinMode(BUTTON_MENU, INPUT_PULLUP);
  pinMode(BUTTON_SET, INPUT_PULLUP);
  pinMode(BUTTON_DOUBLE, INPUT_PULLUP);
  pinMode(BUTTON_HALF, INPUT_PULLUP);
  pinMode(BUTTON_ENC, INPUT_PULLUP);
}

void setup() {
  // Inicializa o pino do LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // Inicializa o pino do sensor IR
  pinMode(SENSOR_IR_PIN, INPUT);
  setupButtons();

  // Inicia a comunicação I2C com os pinos corretos
  Wire.begin(21, 22);  // SDA = 21, SCL = 22
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao iniciar OLED"));
    while (true);
  }
  display.clearDisplay();

  Serial.begin(115200);
  while (!Serial);
  iniciarSPIFlash();
  identificarJEDEC();

  exibirImagemDaFlash(0x00000, 128, 64, 0, 0);
  delay(2000);
  display.clearDisplay();
  
  exibirImagemDaFlash(0x0041F, 128, 32, 0, 20);
  delay(2000);
  display.clearDisplay();
  
  exibirImagemDaFlash(0x0063E, 32, 32, 90, 15);

  //carregarArquivo(displayConfig, 0x00680, 0x001AD1);
  carregarArquivo(dadosConfig, 0x001B92, 0x001BD4);
  numIdioma = getValor(dadosConfig, "IDIOMA").toInt();
  idioma = getSiglaIdioma(numIdioma);
  delay(100);
  carregarArquivoParcial(displayConfig, 0x006DD, 0x001B60, idioma);
  
  delay(2000);
  display.clearDisplay();
  /*
  setValor(dadosConfig, "IDIOMA", "1");
  setValor(dadosConfig, "TIMEMEASURE", "30");
  setValor(dadosConfig, "TIMECALIB", "10");
  setValor(dadosConfig, "FPM", "3000");
  setValor(dadosConfig, "INDICEARQUIVO", "0");
  setValor(dadosConfig, "NEW", "0");
  

  if(salvarArquivo(dadosConfig, 0x001AD2, 0x001B6B)){
    //Serial.println("Listando dadosConfig:");
    for (auto& c : dadosConfig) {
      Serial.println(c.chave + "=" + c.valor);
    }
  }
  */
}

void loop() {
  handleInput();
  if (inMenu) {
    drawMenu();
    if(saveData){
      if(salvarArquivo(dadosConfig, 0x001AD2, 0x001B6B)){
        saveData=false;
      }
    }
  } else {
    drawScreen(currentMode);
  }
  
  if (!inMenu) {
    switch (selectedMode) {
      case Mode::FREQUENCY:
      //strobo
        break;
      case Mode::RPM:
      //rpm
        break;
      case Mode::LANTERN:
        //lantern
      case Mode::TEST: 
        //test
       break;
    }
  }
  // nada aqui
}

void handleInput() {
  static bool lastMenuState = HIGH;
  static bool lastSetState  = HIGH;
  static unsigned long lastDebounceTimeMenu = 0;
  static unsigned long lastDebounceTimeSet  = 0;

  if (checkButtonDebounce(BUTTON_MENU, lastMenuState, lastDebounceTimeMenu)) {
    if (inSubmenu) {
      // Se estiver no submenu, apenas sai dele
      inSubmenu = false;
    } else if (!inMenu) {
      // Se estiver fora do menu, entrar no menu com o modo atual
      currentMode = selectedMode;
      inMenu = true;
    } else {
      currentMode = static_cast<Mode>((static_cast<int>(currentMode) + 1) % static_cast<int>(Mode::NUM_MODES));
    }
  }

  if (checkButtonDebounce(BUTTON_SET, lastSetState, lastDebounceTimeSet)) {
    if (inMenu) {
      selectedMode = currentMode;
      inMenu = false;
    } else {
      inSubmenu = !inSubmenu;
    }
  }

  if (checkButtonDebounce(BUTTON_DOUBLE, lastSetState, lastDebounceTimeSet)) {
    if (inMenu) {
      currentMode = static_cast<Mode>((static_cast<int>(currentMode) + 1) % static_cast<int>(Mode::NUM_MODES));
    } else if(selectedMode == Mode::HOME){
      if (topLineIndex < totalLines - lineShowMsg) { // lineShowMsg = linhas cabem na tela
        topLineIndex++;
      }
    } else {
      //nada
    }
  }

  if (checkButtonDebounce(BUTTON_HALF, lastSetState, lastDebounceTimeSet)) {
    if (inMenu) {
      currentMode = static_cast<Mode>((static_cast<int>(currentMode) - 1) % static_cast<int>(Mode::NUM_MODES));
    }
  }

  if (checkButtonDebounce(BUTTON_ENC, lastMenuState, lastDebounceTimeMenu)) {
    inEncoder = !inEncoder;
    if (inEncoder) {
      //FAZ
    } else {
      //NÃO FAZ
    }
  }
}

void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(getValor(displayConfig, "SELECTMODE"+idioma));
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(getModeName(currentMode));
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println(getValor(displayConfig, "MENUPROX", idioma));
  display.display();
  //Serial.println(getValor(displayConfig, "MENUPROX", idioma));
}

// ==== Tela de cada Modo ====
void drawScreen(Mode mode) {
  // Limita a taxa de atualização para evitar flickering e consumo de CPU
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 50) return;  // Atualiza a cada 50ms (20 FPS)
  lastUpdate = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(getModeName(mode));
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
  display.setCursor(0, 14);

  if (currentMode == Mode::RPM) {
    display.print(getValor(displayConfig, "RPM_"+idioma+" "));
    //display.print((int)rpmValue);
    display.setCursor(0, 26);
    display.print(msgTimer.isRunning() ? getValor(displayConfig, "RECRPM", idioma) : getValor(displayConfig, "SETRECRPM", idioma));
    display.setCursor(0, 56);
    display.println(getValor(displayConfig, "MENUPROX", idioma));

  } else if (currentMode == Mode::ABOUT) {
    display.println("Grupo AlfaS: ");
    display.println(getValor(displayConfig, "ABOUTP1", idioma));
    display.println(getValor(displayConfig, "ABOUTP2", idioma));
    display.println(getValor(displayConfig, "ABOUTP3", idioma));
    display.println(getValor(displayConfig, "ABOUTP4", idioma));
    //display.drawBitmap(90, 15, bufferImagem, 32, 32, SSD1306_WHITE);
    display.setCursor(0, 56);
    display.println(getValor(displayConfig, "ABOUTSITE", idioma));
  } else if (currentMode == Mode::TEST) {
    //display.print(fpmTest.isRunning() ? getValor(displayConfig, "TESTING", idioma) : getValor(displayConfig, "TESTER", idioma));
    //display.setCursor(0, 56);
    //display.println(fpmTest.isRunning() ? getValor(displayConfig, "TESTWAIT", idioma) : getValor(displayConfig, "TESTSET", idioma));
  } else if (currentMode == Mode::LANTERN) {
    display.print(getValor(displayConfig, "LANTERN", idioma));
    display.setCursor(0, 56);
    display.println(getValor(displayConfig, "MENULANT", idioma));
  } else if (currentMode == Mode::CONFIG) {
    display.println(getValor(displayConfig, "SELECTLANG", idioma));
    display.println(getNomeIdioma(numIdioma));
  } else if (currentMode == Mode::FREQUENCY) {
    display.setTextSize(2);
    display.setCursor(0, 14);
    //display.print((int)fpm);
    display.setTextSize(1);
    display.setCursor(70, 22);
    display.print(getValor(displayConfig, "FPM", idioma));
    display.setCursor(0, 34);
    display.print(getValor(displayConfig, "HZ", idioma)+"  ");
    //display.print(fpm / 60.0, 2);
    display.setCursor(0, 46);
    display.print(getValor(displayConfig, "PHASE", idioma)+" ");
    //display.print(STB_phaseDegrees);
    display.print((char)247);
    if(inSubmenu){
      display.setCursor(80, 46);
      display.println("<");
      display.setCursor(0, 56);
      display.println(getValor(displayConfig, "PHASEEDIT", idioma));
    }
  } else if (currentMode == Mode::VIBROMETER){
    /*if (adxlAvailable){
      switch (vibroState) {
        case VibroState::VIBRO_HOME:
          display.println(getValor(displayConfig, "CALIBRATE", idioma));
          break;
        case VibroState::VIBRO_CALIB:
         if(!isCalibrating){
          display.print(getValor(displayConfig, "SELECTTIME", idioma));
          display.print(timeCalib);
          display.println("s");
          display.println("");
          display.println(getValor(displayConfig, "SETCALIB", idioma));
         }else{
          display.println(getValor(displayConfig, "CALIBRATING", idioma));
          display.println("");
          display.print(getValor(displayConfig, "RESTTIME", idioma));
          display.print(secondsLeftCalib);
          display.println("s");
         }
          break;
        case VibroState::VIBRO_IDLE:
          display.print(getValor(displayConfig, "SELECTTIME", idioma));
          //display.print(timeMeasure);
          display.println("s");
          display.println("");
          display.println(getValor(displayConfig, "MSGIDLE1", idioma));
          display.println(getValor(displayConfig, "MSGIDLE2", idioma));
        break;
        case VibroState::VIBRO_CONFIG:
          display.println(getValor(displayConfig, "MEASURE", idioma));
          display.println("");
          display.print(getValor(displayConfig, "RESTTIME", idioma));
          display.print(secondsLeft);
          display.println("s");
          break;
        case VibroState::VIBRO_MEASURE:
          display.print("PA: "); display.print(aPeak, 3); display.println("m/s2");
          display.print("RMS A: "); display.print(aRMS, 3); display.println("m/s2");
          display.print("DP: "); display.print(stdDev, 3); display.println("m/s2");
          display.print("V RMS: "); display.print(vRMS, 3); display.println("m/s");
          display.print("FD: "); display.print(freqDominant, 2); display.println("Hz");
          break;
        case VibroState::VIBRO_RESULT:
          display.println("PA: Pico de Acel.");
          display.println("RMS A: RMS de Acel.");
          display.println("DP: Desvio Padrao");
          display.println("V RMS: Vel. de RMS");
          display.println("FD: Freq. Dominante");
          break;
      }
    }else{
      display.println(getValor(displayConfig, "NOTSENSOR", idioma));
    }*/
  }
  display.display();
}

// ==== Retorna nome do modo atual ====
String getModeName(Mode mode) {
  switch (mode) {
    case Mode::HOME: return "StroboTech";
    case Mode::FREQUENCY: return getValor(displayConfig, "TITLEFREQ", idioma);
    case Mode::RPM: return getValor(displayConfig, "TITLERPM", idioma);
    case Mode::LANTERN: return getValor(displayConfig, "TITLELANT", idioma);
    case Mode::VIBROMETER: return getValor(displayConfig, "TITLEVIBRO", idioma);
    case Mode::TEST: return getValor(displayConfig, "TITLETEST", idioma);
    case Mode::ABOUT: return getValor(displayConfig, "TITLEABOUT", idioma);
    case Mode::CONFIG: return getValor(displayConfig, "TITLECONFIG", idioma);
  }
}

String getNomeIdioma(int lang) {
  switch (lang) {
    case 1: return getValor(displayConfig, "LANGPT", idioma);
    case 2: return getValor(displayConfig, "LANGEN", idioma);
    case 3: return getValor(displayConfig, "LANGES", idioma);
    case 4: return getValor(displayConfig, "LANGFR", idioma);
  }
}
String getSiglaIdioma(int lng) {
  switch (lng) {
    case 1: return "PT";
    case 2: return "EN";
    case 3: return "ES";
    case 4: return "FR";
  }
}
