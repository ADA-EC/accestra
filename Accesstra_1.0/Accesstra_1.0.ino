//Código inicialmente desenvolvido em:
//  - https://github.com/Marcos-Pietrucci/Acesstra-ADA/tree/master/codigo


#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         9  // Configurable, see typical pin layout above
#define SS_PIN          10 // Configurable, see typical pin layout above
#define led_verde       5  // Led que indica porta aberta,  pisca quando estiver cadastrando um novo cartão
#define led_vermelho    8  // Led que indica porta fechada, pisca quando estiver cadastrando um novo cartão
#define porta           6  // Envia o sinal de abertura para a porta
#define but_in          7  // Botão interno abre ou fecha!
#define buzzerPin       2  // Pino do buzzer que emite alertas sonoros

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;

// Struct de códigos válidos
typedef struct mem
{
  byte codigo[4];
}Membro;

//Vetor de membros
Membro *membros;
int num_membros;

// UID do cartão mestre
byte UID_mestre[4];

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

//Abre a porta - o programa não tem controle sobre o fechamento
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

void cadastra_membro()
{
  delay(500); //Importante para não recadastrar o próprio cartão admin
  int estado_led = HIGH;

  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

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
  
  ///***************** Bloco de cadastro de membros usando RFID ***********/
  
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
  for (byte i = 0; i < 4; i++) {
    UID_lido[i] = mfrc522.uid.uidByte[i];
  }

  //Realoca o vetor de membros
  num_membros++;
  membros = (Membro *) realloc(membros, num_membros*sizeof(Membro));

  //Cadastra o novo membro
  (membros[num_membros - 1]).codigo[0] = UID_lido[0];
  (membros[num_membros - 1]).codigo[1] = UID_lido[1];
  (membros[num_membros - 1]).codigo[2] = UID_lido[2];
  (membros[num_membros - 1]).codigo[3] = UID_lido[3];
  
  
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

void setup()
{
  pinMode(porta, OUTPUT);         // Sinal eletrico para a abertura da trava 
  pinMode(led_verde, OUTPUT);    // Led Verde conectado como output no pino já definido
  pinMode(led_vermelho, OUTPUT); // Led Vermelho conectado como output no pino já definido

  Serial.begin(9600);            // Inicialize a comunicação serial com o PC
  SPI.begin();                   // Init SPI bus
  mfrc522.PCD_Init();            // Init MFRC522 card
  ajusta_led();

  //Cadastra o cartão mestre (em decimal)
  UID_mestre[0] = 146;
  UID_mestre[1] = 129;
  UID_mestre[2] = 82;
  UID_mestre[3] = 28;
  
} 

void loop()
{
  byte UID_lido[4];
  
  if (digitalRead(but_in)) //Apertaram o botão interno, movimentar o sistema
  {
    Serial.println("APERTO");
    while (digitalRead(but_in) == HIGH) {
      ;
    }
    delay(10);
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
