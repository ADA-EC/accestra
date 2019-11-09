//Créditos: 
//  - Marcos-Pietrucci 
//    https://github.com/Marcos-Pietrucci/Acesstra-ADA/tree/master/codigo

//*****************************************************************************************//

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define RST_PIN         9  // Configurable, see typical pin layout above
#define SS_PIN          10 // Configurable, see typical pin layout above
#define led_verde       3  // Led que indica porta aberta,  pisca quando estiver cadastrando um novo cartão
#define led_vermelho    2  // Led que indica porta fechada, pisca quando estiver cadastrando um novo cartão
#define motor           6  // Servo Motor que abre e fecha
#define but_in          4  // Botão interno abre ou fecha!
#define but_out         5  // Botão externo vai apenas travar!!
#define NUMERO_MESTRE   42 // Número exclusivo do cartão mestre, cuja única finalidade é cadastrar novos membros
#define TEMP_MEMBRO     22 // Número que os cartões de membros "visitantes" terão - Esses não contarão com uma música personalizada

//Configurações do servo motor
Servo servo;      // Criar o objeto servo
int aberto = 110; // Definir a posição aberto (ângulo em graus) do servo (varia de 0 até 180)
int fechado = 0;  // Definir a posição fechado (ângulo em graus) do servo (varia de 0 até 180)

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

struct membro{
  char nome[16];
  long nusp; //int em arduinos com ATmega possui apenas 2 bytes, o que é insuficiente
};

char *nome = calloc(16, sizeof(char)); 
char *nusp_char = calloc(16,sizeof(char));

int estado_porta = 1; //0 para fechado, 1 para aberto.

int indc = -1; //Registra o índice dos cadastros
struct membro cadastro[5];

void ajusta_led() //Ajusta os leds  para o estado atual da porta
{
  if(!estado_porta) //Se estiver fechado
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

void ativar_servo()
{ 
  //Adicionar reações do buzzer também
  //Ativa o servo e destrava/trava a porta
  if(!estado_porta)      //Está trancada
  { servo.write(aberto); //Abre a porta
    estado_porta = 1;
    ajusta_led();
  }
  else
  { servo.write(fechado); //Fecha a porta
    estado_porta = 0;
    ajusta_led();
  }
}

void reseta_rfid()
{
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

int cadastra_membro_temporario()
{
  Serial.println("Ola sr(a) admin, vamos cadastrar um membro temporario");
  delay(1500);
  int estado_led = HIGH;
  
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  while( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial())
  { //Enquanto ainda não aproximar o cartão    
    digitalWrite(led_verde, estado_led);
    digitalWrite(led_vermelho, estado_led);
    estado_led = !estado_led;
    delay(250);
  }
  //Saiu do while, significa que encontrou o cartão
  digitalWrite(led_verde, HIGH);
  digitalWrite(led_vermelho, HIGH);

  Serial.print(F("Card UID:"));    //Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  
  
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  byte buffer[34];
  byte block;
  MFRC522::StatusCode status;
  byte len;

  // Sobrenome - nusp visitante
  buffer[0] = '2';
  buffer[1] = '2';

  block = 1;
  
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("PCD_Authenticate() failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    reseta_rfid();
    return 0;
  }
  else Serial.println(F("PCD_Authenticate() success: "));

  // Write block
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("MIFARE_Write() failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    reseta_rfid();
    return 0;
  }
  else Serial.println(F("MIFARE_Write() success: "));

  block = 2;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("PCD_Authenticate() failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    reseta_rfid();
    return 0;
  }

  // Write block
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("MIFARE_Write() failed: "));
    reseta_rfid();
    return 0;
  }
  else Serial.println(F("MIFARE_Write() success: "));

  // Primeiro nome 
    strcpy(buffer,"aluno"); // read first name from serial
  //for (byte i = len; i < 20; i++) buffer[i] = ' ';     // pad with spaces

  block = 4;
  //Serial.println(F("Authenticating using key A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) 
  {
    //Serial.print(F("PCD_Authenticate() failed: "));
    reseta_rfid();
    return 0;
  }

  // Write block
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("MIFARE_Write() failed: "));
    reseta_rfid();
    return 0;
  }
  else Serial.println(F("MIFARE_Write() success: "));

  block = 5;
  //Serial.println(F("Authenticating using key A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("PCD_Authenticate() failed: "));
    reseta_rfid();
    return 0;

  }

  // Write block
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("MIFARE_Write() failed: "));
    reseta_rfid();
    return 0;
  }
  else Serial.println(F("MIFARE_Write() success: "));


  Serial.println(" ");
  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
  ajusta_led();
  return 1; //Deu tudo certo!! 
}

void detecta_membro(long nusp_lido) //Função prototipo
{
  bool abre = false;
  int i;
  if(nusp_lido == NUMERO_MESTRE)
  {
    while(!cadastra_membro_temporario()) //Enquanto ele não conseguir cadastrar o membro, ficar tentando!
    {;}

    //Bipar o buzzer, indicando que deve aproximar o novo cartão a ser cadastrado
    return;
  } 
  
  for(i = 0; i <= indc; i++)
  { //Percorre todos os cadastros pesquisando por membros ou por cartões visitante
    if(cadastro[i].nusp == nusp_lido || nusp_lido == TEMP_MEMBRO)
    {
      ativar_servo(); 
      abre = true;
      break; //Saio do for neste instante
    } 
  }
  
  if(abre)
  { 
    Serial.println("\nBem vindo: ");
    //Serial.println(cadastro[i].nome); apresentação de nome meramente para testes
  }
  else
  {
    Serial.println("\n\nErro: Voce nao esta atutorizadx a entrar");
  }
  
}

void cadastra_membro(char nome[16], long nusp)
{ //No futuro, esta função deve carregar os cadastros presentes no cartão SD
  indc++;
  strcpy(cadastro[indc].nome, nome);
  cadastro[indc].nusp = nusp;
}

//*****************************************************************************************//
void setup() 
{
  servo.attach(motor);           // Servo conectado ao pino já definido
  pinMode(led_verde, OUTPUT);    // Led Verde conectado como output no pino já definido
  pinMode(led_vermelho, OUTPUT); // Led Vermelho conectado como output no pino já definido
  
  Serial.begin(9600);            // Inicialize a comunicação serial com o PC
  SPI.begin();                   // Init SPI bus
  mfrc522.PCD_Init();            // Init MFRC522 card
  //Depois de setar o servomotor
  
  ativar_servo();
  
  cadastra_membro("Alguem", 10228323L);
  cadastra_membro("Fulano", 8283712L);
  cadastra_membro("Marcia", 12345678L);

  Serial.println(F("Aproxime o seu cartão:"));    //shows in serial that it is ready to read
}

//*****************************************************************************************//
void loop() 
{
  
  if(digitalRead(but_in))//Apertaram o botão interno, movimentar o sistema
  {
    while(digitalRead(but_in) == HIGH){;}
    delay(10);
    ativar_servo();
  }
    

  if(digitalRead(but_out) && estado_porta == 1) //Se o botão externo foi pressionado e a porta estava aberta, fecha a porta
  {
    while(digitalRead(but_out) == HIGH){;}
    delay(10);
    ativar_servo();
  }
    
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
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

  Serial.println(F("**Cartão detectado:**"));

  //-------------------------------------------

  //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card

  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));      //uncomment this to see all blocks in hex

  //-------------------------------------------

  byte buffer1[18];

  block = 4;
  len = 18;

  //------------------------------------------- GET FIRST NAME
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("Authentication failed: "));
    reseta_rfid();
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer1, &len);
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("Reading failed: "));
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
  
  Serial.write(nome);
  Serial.print(" ");

  //---------------------------------------- GET LAST NAME

  byte buffer2[18];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("Authentication failed: "));
    reseta_rfid();
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("Reading failed: "));
    reseta_rfid();
    return;
  }

  //PRINT LAST NAME
  
  for (uint8_t i = 0; i < 16; i++)
  {
    if(buffer2[i] != 0)
      nusp_char[i] = buffer2[i];
  }
  
  long nusp_l = atol(nusp_char);
  Serial.print(nusp_l);

  //----------------------------------------

  Serial.println(F("\n**Terminou**\n"));

  delay(1000); //change value if you want to read cards faster

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();


  //Tenho que esperar o RFID completar seu ciclo, para, se necessário, eu poder utilizá-lo no cadastro
   detecta_membro(nusp_l);
  //Zerando as variaveis globais
  for(int j = 0; j < 16; j++)
  {
    nusp_char[j] = 32; //Colocando nulo em todas as posições dos vetores
    nome[j] = 32;
  }
}
//*****************************************************************************************//
