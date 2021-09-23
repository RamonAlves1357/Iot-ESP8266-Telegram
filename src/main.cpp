#include <Arduino.h>
#include <string.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//Sensor de Temperatura
#include <DHT.h>
#include <Adafruit_Sensor.h>

// Loguin para o wifi
char ssid[] = "Samsung-Direct-N92I020";
char password[] = "1234567890";

#define BOT_TOKEN "1791417002:AAGOOcfdZ9A3psMofvi-rQ2CipOeqfsEVMs"

//Quantidade de usuários que podem interagir com o bot
#define SENDER_ID_COUNT 3
//Ids dos usuários que podem interagir com o bot.
String ID_Ramon = "1159192127";
String ID_Aryon = "1888341656";
String ID_Gabriel = "1247734721";
String ID_Jose = "1269677321";
String validSenderIds[SENDER_ID_COUNT] = {ID_Ramon, ID_Aryon, ID_Gabriel};

String ChatIdPadrao = "xxxxxxxxxxxx";

//Cliente para conexões seguras
WiFiClientSecure client;
//Objeto com os métodos para comunicarmos pelo Telegram
UniversalTelegramBot bot(BOT_TOKEN, client);
//Tempo em que foi feita a última checagem
uint32_t lastCheckTime = 0;
int Bot_mtbs = 800;

// Led RGB
#define Led_RED 12
#define Led_Green 13
#define Led_Blue 15
int ValorRed = 255;
int ValorGreen = 0;
int ValorBlue = 0;

// Relé
#define Rele 5
int relayStatus;

// Sensor Temp & Umidade
#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE); // Config DHT

unsigned long runningTime = 0;
String tipoTime;

void setupWiFi()
{
  //WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_AP_STA);
  //WiFi.disconnect();
  delay(500);

  Serial.println("Conectando à rede " + String(ssid));
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);

    //ESP.restart();
  }

  delay(900);
  Serial.println();
  Serial.println("Conectado!");
  Serial.print("IP da conexão: ");
  Serial.println(WiFi.localIP());

  client.setFingerprint("F2:AD:29:9C:34:48:DD:8D:F4:CF:52:32:F6:57:33:68:2E:81:C1:90");
  while (!client.connect("api.telegram.org", 443))
  {
    Serial.print("Client Connect\n");

    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
  }
}

void setColor(int vermelho, int verde, int azul)
{
  /*
#ifdef COMMON_ANODE          //SE O LED RGB FOR DEFINIDO COMO ANODO COMUM, FAZ
  vermelho = 255 - vermelho; //VARIÁVEL RECEBE O RESULTADO DA OPERAÇÃO '255 MENOS O PARÂMETRO (vermelho) INFORMADO' NA CHAMADA DA FUNÇÃO
  verde = 255 - verde;       //VARIÁVEL RECEBE O RESULTADO DA OPERAÇÃO '255 MENOS O PARÂMETRO (verde) INFORMADO' NA CHAMADA DA FUNÇÃO
  azul = 255 - azul;         //VARIÁVEL RECEBE O RESULTADO DA OPERAÇÃO '255 MENOS O PARÂMETRO (azul) INFORMADO' NA CHAMADA DA FUNÇÃO
#endif*/
  analogWrite(Led_RED, vermelho); //DEFINE O BRILHO DO LED DE ACORDO COM O VALOR INFORMADO PELA VARIÁVEL 'vermelho'
  analogWrite(Led_Green, verde);  //DEFINE O BRILHO DO LED DE ACORDO COM O VALOR INFORMADO PELA VARIÁVEL 'verde'
  analogWrite(Led_Blue, azul);    //DEFINE O BRILHO DO LED DE ACORDO COM O VALOR INFORMADO PELA VARIÁVEL 'azul'
}

void fedeColors()
{
  for (ValorGreen = 0; ValorGreen < 255; ValorGreen = ValorGreen + 5)
  {
    analogWrite(Led_Green, ValorGreen);
    delay(50);
  }
  for (ValorRed = 255; ValorRed > 0; ValorRed = ValorRed - 5)
  {
    analogWrite(Led_RED, ValorRed);
    delay(50);
  }
  for (ValorBlue = 0; ValorBlue < 255; ValorBlue = ValorBlue + 5)
  {
    analogWrite(Led_Blue, ValorBlue);
    delay(50);
  }

  for (ValorGreen = 255; ValorGreen > 0; ValorGreen = ValorGreen - 5)
  {
    analogWrite(Led_Green, ValorGreen);
    delay(50);
  }
  for (ValorRed = 0; ValorRed < 255; ValorRed = ValorRed + 5)
  {
    analogWrite(Led_RED, ValorRed);
    delay(50);
  }
  for (ValorBlue = 255; ValorBlue > 0; ValorBlue = ValorBlue - 5)
  {
    analogWrite(Led_Blue, ValorBlue);
    delay(50);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando...");

  //Inicializa o WiFi e se conecta à rede
  setupWiFi();

  // Config Led Status
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Config Rele
  pinMode(Rele, OUTPUT);
  digitalWrite(Rele, HIGH);

  // Iniciando Sensor de temperatura (DHT)
  dht.begin();

  // Config Led RGB
  pinMode(Led_RED, OUTPUT);
  pinMode(Led_Green, OUTPUT);
  pinMode(Led_Blue, OUTPUT);
  setColor(0, 0, 0); // Inicia os led (Desligado)

  // Opa, tudo certo até agora.
  Serial.println("\nControle ativo! ");
  bot.sendMessage(ID_Ramon, "Controle Ativo!", "HTML");
}

//time_t prevDisplay = 0;
boolean validSender(String SenderId)
{
  //Para cada id de usuário que pode interagir com este bot
  for (int i = 0; i < SENDER_ID_COUNT; i++)
  {
    //Se o id do remetente faz parte do array retornamos que é válido
    if (SenderId == validSenderIds[i])
    {
      return true;
    }
  }
  //Se chegou aqui significa que verificou todos os ids e não encontrou no array
  return false;
}

void handleNewMessages(int numNewMessages)
{
  String answer;

  for (int i = 0; i < numNewMessages; i++)
  { //para cada mensagem nova

    String chatId = String(bot.messages[i].chat_id);   //id do chat
    String senderId = String(bot.messages[i].from_id); //id do contato

    String text = bot.messages[i].text; //texto que chegou
    String from_name = bot.messages[i].from_name;

    Serial.println("ID: " + chatId + " \tNome: " + from_name);

    boolean validate = validSender(senderId);

    if (!validate)
    { //se não for um remetente válido
      answer = "Desculpe, mas você não tem permissão! Fale com o admin desse bot (@ramon1357) para conseguir a permição.";
      bot.sendMessage(chatId, answer, "HTML");
      continue; //continua para a próxima iteração do for (vai para próxima mensgem, não executa o código abaixo)
    }

    Serial.println("Recebido de " + from_name + ": " + text);

    if (text == "/start" || text == "start" || text == "Start" || text == "/Start")
    {
      answer = "Sejá Bem Vindo **Sr(a) " + from_name + "!** \nO bot já está iniciado.\n\n";
      answer += "Digite /Comandos para saber os comandos disponiveis.";
    }
    else if (text == "Status" || text == "/Status" || text == "status" || text == "/status")
    {

      runningTime = millis() / 1000;
      tipoTime = "segundos";
      if (runningTime >= 60)
      {
        runningTime = runningTime / 60;
        tipoTime = "minuto(s)";
        if (runningTime >= 60)
        {
          runningTime = runningTime / 60;
          tipoTime = "hora(s)";
          if (runningTime >= 24)
          {
            runningTime = runningTime / 24;
            tipoTime = "dia(s)";
          }
        }
      }

      answer = "Sistema no AR á " + String(runningTime) + " " + tipoTime + "\n";
      answer += "Está tudo bem por aqui, obrigado por perguntar! 👍";
    }
    else if (text == "Ajuda" || text == "/Ajuda" || text == "ajuda" || text == "/ajuda")
    {
      answer = "Precisando de Ajuda? \nEntre em contato com o ADMIN(@ramon1357) deste BOT.";
    }
    else if (text == "Comandos" || text == "/Comandos" || text == "comandos" || text == "/comandos")
    {
      answer = "/start ou **start** \n__Descrição:__ Inicia o chat do bot. _(Comando não obrigatorio)_ \n\n";
      answer += "/Status ou **Status** \n**Descrição:** Retorna o status do bot. \n\n";
      answer += "/Ajuda ou **Ajuda** \n**Descrição:** Mais informação e contato para ajuda. \n\n";
      answer += "/Comandos ou **Comandos** \n**Descrição:** Retorna todos os possiveis comandos. \n\n";
      answer += "/Ligar ou **Ligar** \n**Descrição:** Liga a luz. \n\n";
      answer += "/Desligar ou **Desligar** \n**Descrição:** Desliga a luz. \n\n";
      answer += "/Piscar ou **Piscar** \n**Descrição:** Irá fazer com que a luz pisque. \n\n";
      answer += "/Temperatura ou **Temperatura** \n**Descrição:** Informa a temperatura e umidade. \n\n";
    }
    else if (text == "Ligar" || text == "/Ligar" || text == "ligar" || text == "/ligar")
    {
      digitalWrite(Rele, LOW);
      answer = from_name + " o dispositivo de SAÍDA 1 foi LIGADO ✔";
    }
    else if (text == "Desligar" || text == "/Desligar" || text == "desligar" || text == "/desligar")
    {
      digitalWrite(Rele, HIGH);
      answer = from_name + " o dispositivo de SAÍDA 1 foi DESLIGADO ❌";
    }
    else if (text == "Piscar" || text == "/Piscar" || text == "piscar" || text == "/piscar")
    {
      digitalWrite(Rele, HIGH);
      digitalWrite(Rele, LOW);
      delay(400);
      digitalWrite(Rele, HIGH);
      delay(400);
      digitalWrite(Rele, LOW);
      delay(400);
      digitalWrite(Rele, HIGH);
      answer = "Feito! 😉";
    }
    else if (text == "Temperatura" || text == "/Temperatura" || text == "temperatura" || text == "/temperatura")
    {
      float temperatura_lida = dht.readTemperature();
      float umidade_lida = dht.readHumidity();

      answer = "🌡 Temperatura: " + String(int(temperatura_lida)) + "ºC \n";
      answer += "💦 Umidade: " + String(int(umidade_lida)) + "%.";
    }
    else if (text == "Led apagado" || text == "/Led apagado" || text == "led apagado" || text == "/led apagado")
    {
      setColor(0, 0, 0);
      answer = "Feito! 😉";
    }
    else if (text == "Led ligado" || text == "/Led ligado" || text == "led ligado" || text == "/led ligado")
    {
      setColor(255, 250, 250);
      answer = "Feito! 😉";
    }
    else if (text == "Led vermelho" || text == "/Led vermelho" || text == "led vermelho" || text == "/led vermelho")
    {
      setColor(0, 0, 0);
      setColor(255, 0, 0);
      answer = "Feito! 😉";
    }
    else if (text == "Led verde" || text == "/Led verde" || text == "led verde" || text == "/led verde")
    {
      setColor(0, 0, 0);
      setColor(0, 255, 0);
      answer = "Feito! 😉";
    }
    else if (text == "Led azul" || text == "/Led azul" || text == "led azul" || text == "/led azul")
    {
      setColor(0, 0, 0);
      setColor(0, 0, 255);
      answer = "Feito! 😉";
    }
    else if (text == "Led amarelo" || text == "/Led amarelo" || text == "led amarelo" || text == "/led amarelo")
    {
      setColor(255, 255, 0);
      answer = "Feito! 😉";
    }
    else if (text == "Led azul aqua" || text == "/Led azul aqua" || text == "led azul aqua" || text == "/led azul aqua")
    {
      setColor(0, 0, 0);
      setColor(0, 255, 255);
      answer = "Feito! 😉";
    }
    else if (text == "Led turquesa" || text == "/Led turquesa" || text == "led turquesa" || text == "/led turquesa")
    {
      setColor(0, 0, 0);
      setColor(0, 206, 209);
      answer = "Feito! 😉";
    }
    else if (text == "Led indigo" || text == "/Led indigo" || text == "led indigo" || text == "/led indigo")
    {
      setColor(0, 0, 0);
      setColor(75, 0, 130);
      answer = "Feito! 😉";
    }
    else if (text == "Led margenta" || text == "/Led margenta" || text == "led margenta" || text == "/led margenta")
    {
      setColor(0, 0, 0);
      setColor(139, 0, 139);
      answer = "Feito! 😉";
    }
    else if (text == "Led carmesim" || text == "/Led carmesim" || text == "led carmesim" || text == "/led carmesim")
    {
      setColor(0, 0, 0);
      setColor(220, 20, 60);
      answer = "Feito! 😉";
    }
    else if (text == "Led roxo" || text == "/Led roxo" || text == "led roxo" || text == "/led roxo")
    {
      setColor(0, 0, 0);
      setColor(75, 0, 125);
      answer = "Feito! 😉";
    }
    else if (text == "Led violeta" || text == "/Led violeta" || text == "led violeta" || text == "/led violeta")
    {
      setColor(0, 0, 0);
      setColor(82, 0, 82);
      answer = "Feito! 😉";
    }
    else if (text == "Led marron" || text == "/Led marron" || text == "led marron" || text == "/led marron")
    {
      setColor(0, 0, 0);
      setColor(128, 10, 10);
      answer = "Feito! 😉";
    }
    else if (text == "Led rosa" || text == "/Led rosa" || text == "led rosa" || text == "/led rosa")
    {
      setColor(0, 0, 0);
      setColor(255, 192, 203);
      answer = "Feito! 😉";
    }
    /*else if (text == "Led fade" || text == "/Led fade" || text == "led fade" || text == "/led fade")
    {

      setColor(0, 0, 0);
      fedeColors();
      answer = "Feito! 😉";
    }*/
    else
    {
      answer = "Comando não aceito, por favor, tente novamente.";
      answer += "\n\nCaso não saiba o comando, digite(ou clique) /Comandos para saber os comandos disponiveis.";
    }

    Serial.println("Resposta para " + from_name + ": " + answer + "\n");
    bot.sendMessage(chatId, answer, "Markdown");
  }
}

void loop()
{
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
  }

  //Tempo agora desde o boot
  uint32_t now = millis();

  //Se o tempo passado desde a última checagem for maior que o intervalo determinado
  if (now - lastCheckTime >= Bot_mtbs)
  {
    //Coloca o tempo de útlima checagem como agora e checa por mensagens
    lastCheckTime = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    handleNewMessages(numNewMessages);
  }
  delay(100);
}
