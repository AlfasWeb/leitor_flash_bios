#include "display.h"
#include "flash.h"
#include "config.h"

std::vector<Config> imgLogo1;
std::vector<Config> imgLogo2;
std::vector<Config> imgLogo3;
std::vector<Config> dadosConfig;
std::vector<Config> displayConfig;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  iniciarSPIFlash();
  identificarJEDEC();

  carregarArquivo(displayConfig, 0x00680, 0x001AD1);
  carregarArquivo(dadosConfig, 0x001AD2, 0x001B6B);

  for (auto& c : dadosConfig) {
    Serial.println(c.chave + "=" + c.valor);
  }

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
}

void loop() {
  // nada aqui
}