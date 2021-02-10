//Código inicialmente desenvolvido em:
//  - https://github.com/Marcos-Pietrucci/Acesstra-ADA/tree/master/codigo


#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#define SS_SD           4  // Slave Select do cartão SD
#define RST_PIN         9  // Configurable, see typical pin layout above
#define SS_RFID         10 // Slave Select do RFID
#define led_verde       5  // Led que indica porta aberta,  pisca quando estiver cadastrando um novo cartão
#define led_vermelho    8  // Led que indica porta fechada, pisca quando estiver cadastrando um novo cartão
#define porta           6  // Envia o sinal de abertura para a porta
#define but_in          7  // Botão interno abre ou fecha!
#define buzzerPin       2  // Pino do buzzer que emite alertas sonoros

// --- Objetos do RFID --- //
MFRC522 mfrc522(SS_RFID, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;


// --- Objetos do SD --- //
File arquivo;

// Struct de códigos válidos
typedef struct mem
{
  byte codigo[4];
}Membro;

//Vetor de membros
Membro *membros;
int num_membros;

//UID do cartão mestre
byte UID_mestre[4] = {146, 129, 82, 28};

//0 para fechado, 1 para aberto
int estado_porta = 0; 

//Emite alerta sonoro de positivo
void open_Door()
{
  tone(buzzerPin, 440, 400);
  delay(200);
  noTone(buzzerPin);
  tone(buzzerPin, 523, 400);
  delay(400);
  noTone(buzzerPin);
}

//Emite alerta sonoro de negativo
void close_Door()
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
  digitalWrite(porta, HIGH); //Abre a porta
  delay(300);                //O melhor tempo de delay deve ser testado
  digitalWrite(porta, LOW); 

  //Alterar os Leds
  estado_porta = 1;
  ajusta_led();
  open_Door();
  delay(1000);

  //Alterar os Leds
  estado_porta = 0;
  ajusta_led();
}

//Função que lê um cartão RF e cadastra no SD e na SRAM
void cadastra_membro()
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
    estado_led = !estado_led; //Blinkar o LED
    tone(buzzerPin, 349, 400);
    delay(100);
    tone(buzzerPin, 349, 400);
    noTone(buzzerPin);
    delay(400);
  }
  
  //***************** Bloco de cadastro de membros ***********//
  
  byte UID_lido[4];
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  //Verifica se o cartão é do tipo válido
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) 
  {
    close_Door();
    ajusta_led();
    return;
  }

  //Armazena o UID lido
  for (byte j = 0; j < 4; j++) {
    UID_lido[j] = mfrc522.uid.uidByte[j];
  }

  //Realoca o vetor de membros
  num_membros++;
  membros = (Membro *) realloc(membros, num_membros*sizeof(Membro));

  //Cadastra o novo membro
  for(i = 0; i < 4; i++)
    (membros[num_membros - 1]).codigo[i] = UID_lido[i];

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
  return;
}

//Função que verifica se o UID lido corresponde a algum cadastrado
boolean detecta_membro(byte *UID_lido)
{
  int i;
  for(i = 0; i < num_membros; i++)
  {
    if(memcmp(UID_lido, (membros[i]).codigo, sizeof(UID_lido)) == 0)
      return true;
  }

  return false; 
}

//Função que emite um alerta sonoro de erro
void erro()
{
    while(1)
    {
      close_Door();
      delay(500);
    }  
}

//Função que lê os registros do cartão SD
void le_registros()
{
  byte aux;
  int i;
 
  //Abre o arquivo de membros
  arquivo = SD.open("membros.bin", FILE_READ);
  for(i = 0; i < arquivo.size() ; i+=4)
  {
    //Aloca um novo membro
    num_membros++;
    membros = (Membro *) realloc(membros, num_membros*sizeof(Membro));

    //Lê os bytes do cartão SD
    arquivo.read(membros[i/4].codigo, 4);  
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

  //Lê os registros armazenados
  le_registros();

  //Ativa o RFID e desativa o cartão SD
  digitalWrite(SS_RFID, LOW);
  digitalWrite(SS_SD, HIGH);

  //Inicia o MFRC522
  mfrc522.PCD_Init();
  
} 


void loop()
{
  //Armazena o UID que será lido
  byte UID_lido[4];

  //Verifica se apertaram o botão interno
  if (digitalRead(but_in)) 
  {
    while (digitalRead(but_in) == HIGH) {
      ;
    }
    abre_porta();
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
    close_Door();
    return;
  }

  // Armazena o UID lido
  for (byte i = 0; i < 4; i++)
  {
    UID_lido[i] = mfrc522.uid.uidByte[i];
  }
  
  ///******************** Processar o UID lido *****************************/

  //Verificar se o UID lido é o mestre
  if(memcmp(UID_lido, UID_mestre, sizeof(UID_lido)) == 0)
  {
    //UID lido é o mestre, iniciar cadastro
    cadastra_membro();
    return;  
  }
  //Verificar se o UID lido é de algum membro
  else if(detecta_membro(UID_lido))
  {
    //Positivo: Abrir a porta
    abre_porta();
  }
  else
  {
    //Negativo: Fechar a porta
    close_Door();
  }
  
}
