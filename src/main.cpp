#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const unsigned long BAUD_RATE = 115200;
const int CALC_RX = 16;
const int CALC_TX = 17;
const size_t CALC_LINE_LENGTH = 30;

HardwareSerial CalcSerial(2);

String line = "";
String ssid = "";
String password = "";
bool wifiConnected = false;

const String openAIURL = "https://api.openai.com/v1/responses";
const char *openAIAPIKey = OPENAI_API_KEY;

String cleanForCalc(String text)
{
  text.replace("\n", " ~ ");
  text.replace("\r", "");
  text.trim();

  if (text.length() > 900)
  {
    text = text.substring(0, 900) + "...";
  }

  return text;
}

void sendLine(const String &message)
{
  if (message.length() == 0)
  {
    CalcSerial.print("\r\n");
    CalcSerial.flush();
    Serial.println("TX:");
    return;
  }

  for (size_t start = 0; start < message.length(); start += CALC_LINE_LENGTH)
  {
    size_t end = start + CALC_LINE_LENGTH;
    if (end > message.length())
    {
      end = message.length();
    }

    String chunk = message.substring(start, end);
    CalcSerial.print(chunk);
    CalcSerial.print("\r\n");

    Serial.print("TX: ");
    Serial.println(chunk);
  }

  CalcSerial.flush();
}

String askOpenAI(const String &prompt)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    wifiConnected = false;
    return "ERR wifi disconnected";
  }

  WiFiClientSecure client;
  HTTPClient http;

  client.setInsecure();

  if (!http.begin(client, openAIURL))
  {
    return "ERR http begin failed";
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(openAIAPIKey));

  JsonDocument requestDoc;

  requestDoc["model"] = "gpt-4.1-mini";
  requestDoc["instructions"] =
      "You are running on a TI-Nspire calculator through an ESP32. "
      "Answer briefly and clearly. Use plain text only. "
      "If given a math problem, solve it showing the main steps and give a very brief explanation. "
      "Make your response brief yet readable. Use ~ instead of new lines.";
  requestDoc["input"] = prompt;
  requestDoc["max_output_tokens"] = 500;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  int statusCode = http.POST(requestBody);
  String responseBody = http.getString();
  http.end();

  if (statusCode <= 0)
  {
    return "ERR http " + http.errorToString(statusCode);
  }

  if (statusCode != 200)
  {
    return cleanForCalc("ERR API " + String(statusCode) + " " + responseBody);
  }

  JsonDocument responseDoc;
  DeserializationError error = deserializeJson(responseDoc, responseBody);

  if (error)
  {
    return "ERR json parse failed";
  }

  String answer = responseDoc["output"][0]["content"][0]["text"].as<String>();

  if (answer.length() == 0)
  {
    return "ERR empty AI response";
  }

  return cleanForCalc(answer);
}

void handleCommand(String command)
{
  command.trim();

  Serial.print("RX CMD: ");
  Serial.println(command);

  if (command.startsWith("ISTI"))
  {
    sendLine("TISTEM");
  }
  else if (command == "PING")
  {
    sendLine("PONG");
  }
  else if (command.startsWith("WIFI "))
  {
    String wifiData = command.substring(5);
    int separatorIndex = wifiData.indexOf('|');

    if (separatorIndex == -1)
    {
      sendLine("ERR use WIFI ssid|password");
      return;
    }

    ssid = wifiData.substring(0, separatorIndex);
    password = wifiData.substring(separatorIndex + 1);

    wifiConnected = false;

    WiFi.disconnect(true);
    delay(200);

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000)
    {
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      wifiConnected = true;
      sendLine("WIFI OK");
    }
    else
    {
      wifiConnected = false;
      sendLine("WIFI FAIL");
    }
  }
  else if (command.startsWith("ASK "))
  {
    String prompt = command.substring(4);

    if (wifiConnected)
    {
      String answer = askOpenAI(prompt);
      sendLine(answer);
    }
    else
    {
      sendLine("ERR no wifi connection");
    }
  }
  else
  {
    sendLine("ERR unknown command");
  }
}

void readFromCalc()
{
  while (CalcSerial.available())
  {
    char c = CalcSerial.read();

    Serial.write(c);

    if (c == '\n')
    {
      line.trim();

      if (line.length() > 0)
      {
        handleCommand(line);
      }

      line = "";
    }
    else if (c != '\r')
    {
      line += c;
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  CalcSerial.begin(BAUD_RATE, SERIAL_8N1, CALC_RX, CALC_TX);

  Serial.println("nGPT ESP32 ready");
}

void loop()
{
  readFromCalc();
}
//  #include <Arduino.h>
//  #include <ArduinoJson.h>
//  #include <WiFi.h>
//  #include <HTTPClient.h>
//  #include <WiFiClientSecure.h>

// const unsigned long BAUD_RATE = 115200;
// const size_t CALC_LINE_LENGTH = 30;

// String line = "";
// String ssid = "";
// String password = "";
// bool wifiConnected = false;

// const String openAIURL = "https://api.openai.com/v1/responses";
// const char *openAIAPIKey = OPENAI_API_KEY;

// String cleanForCalc(String text)
// {
//   text.replace("\n", " ~ ");
//   text.replace("\r", "");
//   text.trim();

//   if (text.length() > 900)
//   {
//     text = text.substring(0, 900) + "...";
//   }

//   return text;
// }

// void sendLine(const String &message)
// {
//   if (message.length() == 0)
//   {
//     Serial.print("\r\n");
//     Serial.flush();
//     return;
//   }

//   for (size_t start = 0; start < message.length(); start += CALC_LINE_LENGTH)
//   {
//     size_t end = start + CALC_LINE_LENGTH;
//     if (end > message.length())
//     {
//       end = message.length();
//     }

//     String chunk = message.substring(start, end);
//     Serial .print(chunk);
//     Serial.print("\r\n");
//   }

//   Serial.flush();
// }

// String askOpenAI(const String &prompt)
// {
//   if (WiFi.status() != WL_CONNECTED)
//   {
//     wifiConnected = false;
//     return "ERR wifi disconnected";
//   }

//   WiFiClientSecure client;
//   HTTPClient http;

//   client.setInsecure();

//   if (!http.begin(client, openAIURL))
//   {
//     return "ERR http begin failed";
//   }

//   http.addHeader("Content-Type", "application/json");
//   http.addHeader("Authorization", "Bearer " + String(openAIAPIKey));

//   JsonDocument requestDoc;

//   requestDoc["model"] = "gpt-4.1-mini";
//   requestDoc["instructions"] =
//       "You are running on a TI-Nspire calculator through an ESP32. "
//       "Answer briefly and clearly. Use plain text only. "
//       "If given a math problem, solve it showing the main steps and give a very brief explanation. "
//       "Make your response brief yet readable. Use ~ instead of new lines.";
//   requestDoc["input"] = prompt;
//   requestDoc["max_output_tokens"] = 500;

//   String requestBody;
//   serializeJson(requestDoc, requestBody);

//   int statusCode = http.POST(requestBody);
//   String responseBody = http.getString();
//   http.end();

//   if (statusCode <= 0)
//   {
//     return "ERR http " + http.errorToString(statusCode);
//   }

//   if (statusCode != 200)
//   {
//     return cleanForCalc("ERR API " + String(statusCode) + " " + responseBody);
//   }

//   JsonDocument responseDoc;
//   DeserializationError error = deserializeJson(responseDoc, responseBody);

//   if (error)
//   {
//     return "ERR json parse failed";
//   }

//   String answer = responseDoc["output"][0]["content"][0]["text"].as<String>();

//   if (answer.length() == 0)
//   {
//     return "ERR empty AI response";
//   }

//   return cleanForCalc(answer);
// }

// void handleCommand(String command)
// {
//   command.trim();

//   if (command.startsWith("ISTI"))
//   {
//     sendLine("TISTEM");
//   }
//   else if (command == "PING")
//   {
//     sendLine("PONG");
//   }
//   else if (command.startsWith("WIFI "))
//   {
//     String wifiData = command.substring(5);
//     int separatorIndex = wifiData.indexOf('|');

//     if (separatorIndex == -1)
//     {
//       sendLine("ERR use WIFI ssid|password");
//       return;
//     }

//     ssid = wifiData.substring(0, separatorIndex);
//     password = wifiData.substring(separatorIndex + 1);

//     wifiConnected = false;

//     WiFi.disconnect(true);
//     delay(200);

//     WiFi.begin(ssid.c_str(), password.c_str());

//     unsigned long startTime = millis();

//     while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000)
//     {
//       delay(500);
//     }

//     if (WiFi.status() == WL_CONNECTED)
//     {
//       wifiConnected = true;
//       sendLine("WIFI OK");
//     }
//     else
//     {
//       wifiConnected = false;
//       sendLine("WIFI FAIL");
//     }
//   }
//   else if (command.startsWith("ASK "))
//   {
//     String prompt = command.substring(4);

//     if (wifiConnected)
//     {
//       String answer = askOpenAI(prompt);
//       sendLine(answer);
//     }
//     else
//     {
//       sendLine("ERR no wifi connection");
//     }
//   }
//   else
//   {
//     sendLine("ERR unknown command");
//   }
// }

// void readFromCalc()
// {
//   while (Serial.available())
//   {
//     char c = Serial.read();

//     if (c == '\n')
//     {
//       line.trim();

//       if (line.length() > 0)
//       {
//         handleCommand(line);
//       }

//       line = "";
//     }
//     else if (c != '\r')
//     {
//       line += c;
//     }
//   }
// }

// void setup()
// {
//   Serial.begin(115200);
//   delay(1000);
// }

// void loop()
// {
//   readFromCalc();
// }