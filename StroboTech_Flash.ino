#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "flash.h"
#include "config.h"

std::vector<Config> imgLogo1;
std::vector<Config> imgLogo2;
std::vector<Config> imgLogo3;
std::vector<Config> dadosConfig;
std::vector<Config> displayConfig;

bool saveData=false;

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

  display.begin();

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

  for (auto& c : displayConfig) {
    Serial.println(c.chave + "=" + c.valor);
  }
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
  if(saveData){
    if(salvarArquivo(dadosConfig, 0x001B92, 0x001BD4)){
      saveData=false;
    }
  }
  // nada aqui
}
