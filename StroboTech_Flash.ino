#include "flash.h"
#include "config.h"
#include <vector>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

std::vector<Config> dadosConfig;
std::vector<Config> displayConfig;

// Exemplo de callback
void processarPixel(int x, int y, bool isOn) {
  display.drawPixel(x, y, isOn ? WHITE : BLACK);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Inicia a comunicação I2C com os pinos corretos
  Wire.begin(21, 22);  // SDA = 21, SCL = 22
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao iniciar OLED"));
    while (true);
  }
  display.clearDisplay();

  iniciarSPIFlash();
  identificarJEDEC();

  exibirImagemDaFlash(0x00000, 64, 64, 0, 0, processarPixel);
  display.display();
  
  //carregarArquivo(displayConfig, 0x00680, 0x001AD1);
  carregarArquivo(dadosConfig, 0x002000, 0x0020FF);
  Serial.println("Listando dadosConfig:");
  for (auto& c : dadosConfig) {
    Serial.println(c.chave + "=" + c.valor);
  }
  //Carregar dados parcialmente, para não sobrecarregar a memoria
  carregarArquivoParcial(displayConfig, 0x006DD, 0x001FFF, "PT");
  Serial.println("Listando displayConfig:");
  for (auto& c : displayConfig) {
    Serial.println(c.chave + "=" + c.valor);
  }
  //Para imprimir um valor através de GET
  Serial.println(getValor(dadosConfig, "TIMEMEASURE"));
  //Pegar um valor e converter em inteiro
  int valor = getValor(dadosConfig, "TIMEMEASURE").toInt();


  //Salvar um valor em arquivo
  setValor(dadosConfig, "TIMEMEASURE", "30");
  //para salvar na flash deve executar o salvar arquivo, ou ficará salvo apenas na memoria ram
  if(salvarArquivo(dadosConfig, 0x002000, 0x0020FF)){
    Serial.println("Valor foi salvo");
  }

  delay(2000);
  display.clearDisplay();
}

void loop() {
  // nada aqui
}
