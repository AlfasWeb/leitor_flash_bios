#include "flash.h"
#include "config.h"

std::vector<Config> imgLogo1;
std::vector<Config> imgLogo2;
std::vector<Config> imgLogo3;
std::vector<Config> dadosConfig;
std::vector<Config> displayConfig;

bool saveData=false;

void exibirImagem(const std::vector<Config>& imagem) {
  for (const auto& pixel : imagem) {
    int sep = pixel.chave.indexOf(',');
    if (sep == -1) continue;

    int x = pixel.chave.substring(0, sep).toInt();
    int y = pixel.chave.substring(sep + 1).toInt();

    int cor = pixel.valor.toInt();  // Deve ser "1" ou "0"

    display.drawPixel(x, y, cor ? WHITE : BLACK);
  }

  display.display();  // Atualiza o display com todos os pixels
}


void setup() {
  Serial.begin(115200);
  while (!Serial);

  iniciarSPIFlash();
  identificarJEDEC();

  carregarImagem(imgLogo1, 0x000000, 0x0003FF);  // endereço da imagem 1
  exibirImagem(imgLogo1, 128, 64, 0, 0);
  display.clearDisplay();
  delay(2000);
  carregarImagem(imgLogo2, 0x00041F, 0x00061E);  // endereço da imagem 1
  exibirImagem(imgLogo2, 128, 32, 90, 15);
  display.clearDisplay();
  delay(2000);
  carregarImagem(imgLogo3, 0x00063E, 0x0006BD);  // endereço da imagem 1
  exibirImagem(imgLogo3, 32, 32, 0, 20);

  //carregarArquivo(displayConfig, 0x00680, 0x001AD1);
  carregarArquivo(dadosConfig, 0x001B92, 0x001BD4);
  numIdioma = getValor(dadosConfig, "IDIOMA").toInt();
  idioma = getSiglaIdioma(numIdioma);
  delay(100);
  carregarArquivoParcial(displayConfig, 0x006DD, 0x001B60, idioma);
  display.clearDisplay();
  delay(2000);
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
