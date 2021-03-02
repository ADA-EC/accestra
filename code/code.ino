#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#define SS_SD           4  // Slave Select do cartão SD
#define SS_RFID         10 // Slave Select do RFID
#define RST_PIN         9  // Configurable, see typical pin layout above
#define led_verde       5  // Led que indica porta positivo
#define led_vermelho    8  // Led que indica porta negativo
#define porta           6  // Envia o sinal de abertura para a porta
#define but_in          7  // Botão interno que abre a porta
#define buzzerPin       2  // Pino do buzzer que emite alertas sonoros

// --- Objetos do RFID --- //
MFRC522 mfrc522(SS_RFID, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;

// --- Objetos do SD --- //
File arquivo;

// Struct dos membros
typedef struct memb
{
  byte codigo[4];
}Membro;

//UID que será lido na loop
byte UID_loop[4];

//UID do cartão de cadastro
byte UID_cadastro[4] = {146, 129, 82, 28};

//UID do cartão de descadastro
byte UID_descadastro[4] = {0, 0, 0, 0};

//0 para fechado, 1 para aberto
int estado_porta = 0; 

//Emite alerta sonoro de positivo
void positivo()
{
  tone(buzzerPin, 440, 400);
  delay(200);
  noTone(buzzerPin);
  tone(buzzerPin, 523, 400);
  delay(400);
  noTone(buzzerPin);
}

//Emite alerta sonoro de negativo
void negativo()
{
  tone(buzzerPin, 440, 400);
  delay(200);
  noTone(buzzerPin);
  tone(buzzerPin, 349, 400);
  delay(400);
  noTone(buzzerPin);
}

//Ajusta os leds  para o estado atual da porta
void ajusta_led() 
{
  //Se estiver fechado
  if (!estado_porta)
  {
    digitalWrite(led_verde, LOW);
    digitalWrite(led_vermelho, HIGH);
  }
  else  //Se estiver aberto
  {
    digitalWrite(led_verde, HIGH);
    digitalWrite(led_vermelho, LOW);
  }
}

//Função que abre a porta - o programa não tem controle sobre o fechamento
void abre_porta()
{
  //Ativar a trava elétrica
  digitalWrite(porta, HIGH); 
  
  //O melhor tempo de delay deve ser testado
  delay(300);                
  digitalWrite(porta, LOW); 

  //Alterar os Leds
  estado_porta = 1;
  ajusta_led();
  positivo();
  delay(1000);

  //Alterar os Leds
  estado_porta = 0;
  ajusta_led();
}

//Função que lê um cartão RF e cadastra no SD
void cadastra_membro()
{
  //Importante para não recadastrar o próprio cartão admin
  delay(1000);
  
  int estado_led = HIGH;
  int i;

  MFRC522::MIFARE_Key key;
  for (byte j = 0; j < 6; j++) key.keyByte[j] = 0xFF;

  //Enquanto ainda não aproximar o cartão
  while ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial())
  {
    digitalWrite(led_verde, estado_led);
    digitalWrite(led_vermelho, estado_led);

    //Blinkar o LED
    estado_led = !estado_led; 

    //Som para indicar estado de cadastro
    tone(buzzerPin, 349, 400);
    delay(100);
    tone(buzzerPin, 349, 400);
    noTone(buzzerPin);
    delay(300);
  }
  
  //***************** Bloco de cadastro de membros ***********//
  byte UID_lido[4];
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  //Verifica se o cartão é do tipo válido
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) 
  {
    negativo();
    ajusta_led();
    return;
  }

  //Armazena o UID lido
  for (byte j = 0; j < 4; j++) {
    UID_lido[j] = mfrc522.uid.uidByte[j];
  }

  //Desativa o RFID e ativa o SD
  digitalWrite(SS_RFID, HIGH);
  digitalWrite(SS_SD, LOW);
  delay(3);
  
  //------------------ Inserir o novo membro no cartão SD --------/
  if (!SD.begin(SS_SD))
  {
    erro();
  }

  //Abre o arquivo de membros
  arquivo = SD.open("membros.bin", FILE_WRITE); 
  
  if (arquivo) 
  {
    //Escreve os 4 bytes do UID lido
    arquivo.write(UID_lido, 4);
    arquivo.close();
  } 
  else 
  {
    erro();
  }

  //Desativa o SD e ativa o RFID
  digitalWrite(SS_RFID, LOW);
  digitalWrite(SS_SD, HIGH);
  delay(3);
  
  ///***************** Fim do bloco de cadastro de membros *******/
  ajusta_led();
  delay(1500);
}

//Função que verifica se o UID lido corresponde a algum cadastrado
boolean detecta_membro(byte *UID_lido)
{
  int i;
  Membro mem;
 
  //Abre o arquivo de membros
  arquivo = SD.open("membros.bin", FILE_READ);

  if(!arquivo)
    erro();

  for(i = 0; i < arquivo.size() ; i += 4)
  {
    //Lê arquivo os bytes do cartão SD
    arquivo.read(mem.codigo, 4);

    //Compara o UID lido com o membro cadastrado
    if(memcmp(UID_lido, mem.codigo, 4) == 0)
    {
      return true;
    }
  }   

  return false; 
}

//Função que lê um cartão e o descadastra caso encontre-o
void deleta_membro()
{
  delay(1000); //Importante para não recadastrar o próprio cartão admin
  int estado_led = HIGH;
  int i;

  MFRC522::MIFARE_Key key;
  for (byte j = 0; j < 6; j++) key.keyByte[j] = 0xFF;

  //Enquanto ainda não aproximar o cartão
  while ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial())
  {
    digitalWrite(led_verde, estado_led);
    digitalWrite(led_vermelho, estado_led);
    
    //Blinkar o LED
    estado_led = !estado_led; 

    //Alerta sonoro de descadastro
    tone(buzzerPin, 349, 400);
    delay(100);
    tone(buzzerPin, 523, 400);
    noTone(buzzerPin);
    delay(300);
  }
  
  //***************** Bloco de cadastro de membros ***********//
  
  byte UID_lido[4];
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  //Verifica se o cartão é do tipo válido
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) 
  {
    negativo();
    ajusta_led();
    return;
  }

  //Armazena o UID lido
  for (byte j = 0; j < 4; j++) {
    UID_lido[j] = mfrc522.uid.uidByte[j];
  }

  //Ativa o SD e desativa o cartão RFID
  digitalWrite(SS_RFID, HIGH);
  digitalWrite(SS_SD, LOW);

  //---- Percorrer o cartão SD e deletar o membro lógicamente ---- //
  byte vetorNulo[4] = {0,0,0,0};
  int flag = 0;
  Membro mem;
 
  //Abre o arquivo de membros
  arquivo = SD.open("membros.bin", O_WRITE | O_READ);

  if(!arquivo)
    erro();

  for(i = 0; i < arquivo.size() ; i += 4)
  {
    //Lê arquivo os bytes do cartão SD
    arquivo.read(mem.codigo, 4);

    //Compara o UID lido com o membro cadastrado
    if(memcmp(UID_lido, mem.codigo, sizeof(UID_lido)) == 0)
    { 
      //Posiciona o ponteiro para a posição necessária
      arquivo.seek(arquivo.position() - 4);

      //Escreve valor invalido no registro
      arquivo.write(vetorNulo, 4);

      //Indica que houve o descadastro
      flag = 1;
      
      break;
    }
  }   
  
  arquivo.close();

  //Se não houve descadastro, emitir alerta negativo
  if(!flag)
  {
    negativo();
  }
  
  //Ativa o RFID e desativa o cartão SD
  digitalWrite(SS_RFID, LOW);
  digitalWrite(SS_SD, HIGH);

  ///***************** Fim do bloco de descadastro de membros *******/
  ajusta_led();
  delay(1500);
}

//Função que emite um alerta sonoro de erro
void erro()
{
    while(1)
    {
      //Substituir por função sonora específica de erro
      negativo();
      delay(500);
    }  
}

void setup()
{
  // ----------- Preparar os pinos ----------- //
  pinMode(porta, OUTPUT);         
  pinMode(led_verde, OUTPUT);    
  pinMode(led_vermelho, OUTPUT);  

  pinMode(SS_RFID, OUTPUT);
  pinMode(SS_SD, OUTPUT);
  
  Serial.begin(9600);           
  SPI.begin();                   
  ajusta_led();

  //Desativa o RFID
  digitalWrite(SS_RFID, HIGH);
  
  // ------------- Cartão SD --------------- //
  //Verifica se o cartão está funcionando
  if (!SD.begin(SS_SD)) 
  {
    erro();
  }

  //Ativa o RFID e desativa o cartão SD
  digitalWrite(SS_RFID, LOW);
  digitalWrite(SS_SD, HIGH);

  //Inicia o MFRC522
  mfrc522.PCD_Init();
} 

void loop()
{
  //Verifica se apertaram o botão interno
  if (digitalRead(but_in)) 
  {
    while (digitalRead(but_in) == HIGH) {
      ;
    }
    positivo();
  }

  ///***************** Bloco de detecção de cartões que serão aproximados **************/
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  // Verifica se o cartão é do tipo válido
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) 
  {
    negativo();
    return;
  }

  // Armazena o UID lido
  for (byte i = 0; i < 4; i++)
  {
    UID_loop[i] = mfrc522.uid.uidByte[i];
  }
  
  ///******************** Processar o UID lido *****************************/

  //Verificar se o UID lido é o mestre
  if(memcmp(UID_loop, UID_cadastro, sizeof(UID_loop)) == 0)
  {
    //UID lido é o mestre, iniciar cadastro
    cadastra_membro();
  }
  //Verificar se o UID lido é o de descadastro
  else if(memcmp(UID_loop, UID_descadastro, sizeof(UID_loop)) == 0)
  {
    //UID lido é o de descadastro
    deleta_membro();
  }
  //Verificar se o UID lido é de algum membro
  else if(detecta_membro(UID_loop))
  {
    //Positivo: Abrir a porta
    positivo();
  }
  else
  {
    //Negativo: Fechar a porta
    negativo();
  }
  
}
