//Código inicialmente desenvolvido em:
//  - https://github.com/Marcos-Pietrucci/Acesstra-ADA/tree/master/codigo


#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define RST_PIN         9  // Configurable, see typical pin layout above
#define SS_PIN          10 // Configurable, see typical pin layout above
#define led_verde       4  // Led que indica porta aberta,  pisca quando estiver cadastrando um novo cartão
#define led_vermelho    2  // Led que indica porta fechada, pisca quando estiver cadastrando um novo cartão
#define porta           6  // Envia o sinal de abertura para a porta
#define but_in          7  // Botão interno abre ou fecha!
#define but_out         8  // Botão externo vai apenas travar!!
#define buzzerPin       3  // Pino do buzzer que emite alertas sonoros
#define NUMERO_MESTRE   42 // Número exclusivo do cartão mestre, cuja única finalidade é cadastrar novos membros
#define NUMERO_MEMBRO   22 // Número que os cartões de membros "visitantes" terão - Esses não contarão com uma música personalizada

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

char *nome = calloc(16, sizeof(char));
char *nusp_char = calloc(16, sizeof(char));

int estado_porta = 1; //0 para fechado, 1 para aberto.

void open_Door()//Emite alerta sonoro de abertura da porta
{
  tone(buzzerPin, 440, 400);
  delay(200);
  noTone(buzzerPin);
  tone(buzzerPin, 523, 400);
  delay(400);
  noTone(buzzerPin);
}

void close_Door()//Emite alerta sonoro de travamento da porta
{
  tone(buzzerPin, 440, 400);
  delay(200);
  noTone(buzzerPin);
  tone(buzzerPin, 349, 400);
  delay(400);
  noTone(buzzerPin);
}

void ajusta_led() //Ajusta os leds  para o estado atual da porta
{
  if (!estado_porta) //Se estiver fechado
  {
    digitalWrite(led_verde, LOW);
    digitalWrite(led_vermelho, HIGH);
  }
  else
  {
    digitalWrite(led_verde, HIGH);
    digitalWrite(led_vermelho, LOW);
  }

}

void ativar_porta()
{
  //Ativa o servo e destrava/trava a porta
  //Emite o alerta sonoro
  if (!estado_porta)     //Está trancada
  {
    digitalWrite(porta, HIGH); //Abre a porta
    estado_porta = 1;
    ajusta_led();
    open_Door();
    delay(1000); //Delay de 1s, para esperar, teoricamente, a fechadura manual da porta
    estado_porta = 0;
    ajusta_led();
  }
}

void reseta_rfid()
{ //Sem esses comandos, após um erro de leitura ou aproximação do cartão -
  // - o módulo travava, e era necessário resetar o arduino para voltar à normalidade
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

int cadastra_membro()
{
  delay(500); //Importante para não escrever no próprio cartão de admin
  int estado_led = HIGH;

  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  while ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial())
  {
    //Enquanto ainda não aproximar o cartão
    digitalWrite(led_verde, estado_led);
    digitalWrite(led_vermelho, estado_led);
    estado_led = !estado_led; //Blinkar o LED
    tone(buzzerPin, 349, 400);
    delay(100);
    tone(buzzerPin, 349, 400);
    noTone(buzzerPin);
    delay(400);
  }

  //Saiu do while, significa que encontrou o cartão
  //Iniciar cadastro do membro

  ///***************** Bloco de cadastro de membros usando RFID ***********/

  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  byte buffer[34];
  byte block;
  MFRC522::StatusCode status;
  byte len;

  //Aqui está setando o número permitido dos membros
  buffer[0] = '2';
  buffer[1] = '2';

  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return 0;
  }

  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return 0;
  }

  block = 2;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return 0;
  }

  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return 0;
  }

  // Primeiro nome
  strcpy(buffer, "membro");
  block = 4;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return 0;
  }

  // Write block
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return 0;
  }

  block = 5;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return 0;
  }

  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return 0;
  }

  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD


  ///***************** Fim do bloco de cadastro de membros *******/

  ajusta_led();
  return 1; //Deu tudo certo!!
}

void detecta_membro(long nusp_lido)
{

  if (nusp_lido == NUMERO_MESTRE)
  {
    while (!cadastra_membro()) //Enquanto ele não conseguir cadastrar o membro, ficar tentando!
    {
      ;
    }

    return;
  }
  else if (nusp_lido == NUMERO_MEMBRO)
  {
    ativar_porta();
  }
  else
    close_Door();
}


void setup()
{
  pinMode(porta,OUTPUT);         // Sinal eletrico para a abertura da trava 
  pinMode(led_verde, OUTPUT);    // Led Verde conectado como output no pino já definido
  pinMode(led_vermelho, OUTPUT); // Led Vermelho conectado como output no pino já definido

  Serial.begin(9600);            // Inicialize a comunicação serial com o PC
  SPI.begin();                   // Init SPI bus
  mfrc522.PCD_Init();            // Init MFRC522 card

  ativar_porta();                // Certificar-se de que a porta inicia fechada
}

//*****************************************************************************************//
void loop()
{

  if (digitalRead(but_in)) //Apertaram o botão interno, movimentar o sistema
  {
    while (digitalRead(but_in) == HIGH) {
      ;
    }
    delay(10);
    ativar_porta();
  }

  if (digitalRead(but_out) && estado_porta == 1) //Se o botão externo foi pressionado e a porta estava aberta, fecha a porta
  {
    while (digitalRead(but_out) == HIGH) {
      ;
    }
    delay(10);
    ativar_porta();
  }


  ///***************** Bloco de detecção de cartões que serão aproximados **************/

  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  byte block;
  byte len;
  MFRC522::StatusCode status;

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte buffer1[18];

  block = 4;
  len = 18;

  //------------------------------------------- Acessar o primeiro nome ----------- //
  //Na realidade o primeiro nome será inútil

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer1, &len);
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return;
  }

  for (uint8_t i = 0; i < 16; i++)
  {
    if (buffer1[i] != 32 && buffer1[i] != 0)
    {
      nome[i] = buffer1[i]; //Copia o nome
    }
  }

  //---------------------------------------- ACESSAR O "SOBRENOME" QUE SERIA O ID -----------//

  byte buffer2[18];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK)
  {
    reseta_rfid();
    return;
  }

  for (uint8_t i = 0; i < 16; i++)
  {
    if (buffer2[i] != 0)
      nusp_char[i] = buffer2[i];
  }

  long nusp_l = atol(nusp_char);

  delay(300);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  ///******************** Fim do bloco de detecção de cartões *****************************/

  detecta_membro(nusp_l);

  //Zerando as variáveis globais
  for (int j = 0; j < 16; j++)
  {
    nusp_char[j] = 32; //Colocando nulo em todas as posições dos vetores
    nome[j] = 32;
  }

}
