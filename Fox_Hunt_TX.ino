/*   
 *   Much of this code was copied from / inspired by the project
 *   "rot13labs fox" https://github.com/c0ldbru/fox
 *   which was in turn copied from / inspired by Yet Another Foxbox
 *   (YAFB) by Gregory Stoike (KN4CK) which can be found here:
 *       https://github.com/N8HR/YAFB.
 *
 *   It has been adapted for use with the
 *       HackerBox Fox Hunt Radio Project:
 *       https://hackerboxes.com/products/hackerbox-fox-hunt
 *   Which uses the TTGO ESP32 T-Display and an SA818 transciever.
 *
 *   This project is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This project is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this. If not, see <https://www.gnu.org/licenses/>.
 */

#include <Adafruit_GFX.h> 
#include <Adafruit_ST7789.h> 
#include <SPI.h>

#define TFT_DC        16
#define TFT_CS         5
#define TFT_SCL       18
#define TFT_SDA       19
#define TFT_RES       23
#define TFT_BLK        4

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RES);

// hardware I/O pins
#define SA818_AudioOn 17
#define SA818_AF_OUT  38  //Analog input to MCU
#define SA818_PTT     13
#define SA818_PD      27
#define SA818_HL      33
#define SA818_RXD     21
#define SA818_TXD     22
#define SA818_AF_IN 	25  //Analog output from MCU
#define Button_A      15
#define Button_B      26
#define Button_C      32

String callmessage = "N0CALL"; // your callsign goes here
String morse = ""; // leave this blank for now; it will be filled in during setup
float frequency = 146.565; // 146.565 is the normal TX frequency for foxes

void setup(void) {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, SA818_TXD, SA818_RXD);

  morse = createMorse(callmessage);

  pinMode(Button_A, INPUT_PULLDOWN);
  pinMode(Button_B, INPUT_PULLDOWN);
  pinMode(Button_C, INPUT_PULLDOWN);

  pinMode(SA818_AudioOn, INPUT);  //SA818 drives pin LOW to tell MCU that a signal is being received

  // SA818_AF_IN is an output from the MCU, but we set it as an INPUT when not in use
  pinMode(SA818_AF_IN, INPUT);

  //Output Power High/Low
  //  High Power: Float (MCU=input)
  //  Low Power: Drive pin LOW
  //  INPORTANT NOTE: NEVER Drive the pin HIGH (undefined state)
  pinMode(SA818_HL, OUTPUT);   
  digitalWrite(SA818_HL, LOW);   
  
  pinMode(SA818_PTT, OUTPUT);      //Push to Talk (Transmit)
  digitalWrite(SA818_PTT, HIGH);   //(0=TX, 1=RX) DO NOT Transmit Without License

  pinMode(SA818_PD, OUTPUT); 
  digitalWrite(SA818_PD, HIGH);    //Release Powerdown (1=Radio ON)

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
  tft.init(135, 240);
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.fillScreen(ST77XX_RED);
  splash();
  delay(500);
  tft.fillScreen(ST77XX_BLACK);
  
  tft.drawRect(0, 0, tft.width(), tft.height(), ST77XX_WHITE);
  tft.setTextColor(ST77XX_RED); 
  tft.setCursor(0, 60);
  tft.println("      FOX HUNT");
  delay(3000);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW,ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.println("Radio Connect:");
  Serial2.println("AT+DMOCONNECT\r");  // connect communications with SA818
  delay(100);
  tft.print(Serial2.readString());     // returns :0 if good
  tft.println("Radio Version:");
  Serial2.print("AT+VERSION\r\n");     // get version
  delay(100);
  tft.print(Serial2.readString());     // returns: version number
  delay(2000);

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.println("Set Group:");           // group set
  setfreq();                           // set frequency
  delay(100);
  tft.print(Serial2.readString());     // returns :0 if good
  
  tft.println("Set Volume: ");
  Serial2.print("AT+DMOSETVOLUME=8\r\n"); // set volume
  delay(100);
  tft.print(Serial2.readString());        // returns :0 if good
  delay(2000);

  tft.fillScreen(ST77XX_BLACK);
  disp_channel();
}

void loop() {
  digitalWrite(SA818_PTT,LOW);      // assert push to talk
  delay(750);
  //playMelody();
  //delay(750);
  playMorse();
  digitalWrite(SA818_PTT,HIGH);     // release push to talk
  delay(15000); 
}

void setfreq() {
  Serial2.print("AT+DMOSETGROUP=0,");
  Serial2.print(String(frequency,4));
  Serial2.print(",");
  Serial2.print(String(frequency,4));
  Serial2.print(",0000,1,0000\r\n");
}

void disp_channel() {
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
  tft.println("Ham Radio Fox Hunt");
  tft.setTextColor(ST77XX_YELLOW,ST77XX_BLACK);
  tft.println("Leave in place, or ");
  tft.println("Call (XXX)XXX-XXXX");
  tft.println("for more info");
  tft.setTextColor(ST77XX_BLUE,ST77XX_BLACK);
  tft.print("Freq: ");
  tft.println(String(frequency,4));  
}

void splash() {
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, tft.height()-1, x, 0, ST77XX_WHITE);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, tft.height()-1, tft.width()-1, y, ST77XX_WHITE);
    delay(0);
  }
}

String createMorse(String toconvert)
{
  struct dict
  {
      char character;
      char morse[8];
  };

  dict morseLookup[54] = 
  {
    { 'a', ".-" },
    { 'b', "-..." },
    { 'c', "-.-." },
    { 'd', "-.." },
    { 'e', "." },
    { 'f', "..-." },
    { 'g', "--." },
    { 'h', "...." },
    { 'i', ".." },
    { 'j', ".---" },
    { 'k', "-.-" },
    { 'l', ".-.." },
    { 'm', "--" },
    { 'n', "-." },
    { 'o', "---" },
    { 'p', ".--." },
    { 'q', "--.-" },
    { 'r', ".-." },
    { 's', "..." },
    { 't', "-" },
    { 'u', "..-" },
    { 'v', "...-" },
    { 'w', ".--" },
    { 'x', "-..-" },
    { 'y', "-.--" },
    { 'z', "--.." },
    { '0', "-----" },
    { '1', ".----" },
    { '2', "..---" },
    { '3', "...--" },
    { '4', "....-" },
    { '5', "....." },
    { '6', "-...." },
    { '7', "--..." },
    { '8', "---.." },
    { '9', "----." },
    { '.', ".-.-.-" },
    { ',', "--..--" },
    { '\?', "..--.." },
    { '\'', ".----." },
    { '!', "-.-.--" },
    { '/', "-..-." },
    { '(', "-.--." },
    { ')', "-.--.-" },
    { '&', ".-..." },
    { ':', "---..." },
    { ';', "-.-.-." },
    { '=', "-...-" },
    { '+', ".-.-." },
    { '-', "-....-" },
    { '_', "..--.-" },
    { '\"', ".-..-." },
    { '$', "...-..-" },
    { '@', ".--.-." },
  };
  
  morse = "";
  toconvert.toLowerCase();

  for (int messagei = 0; messagei < toconvert.length(); messagei++)
  {
    for (int structi = 0; structi < sizeof(morseLookup)/sizeof(dict); structi++)
    {
      if (isSpace(toconvert.charAt(messagei)))
      {
        morse.concat("/ ");
        break;
      }
      else if (toconvert.charAt(messagei) == morseLookup[structi].character)
      {
        morse.concat(morseLookup[structi].morse);
        morse.concat(" ");
      }
    }
  }

  return morse;
}

void playMorse() 
{
  /*
   * short mark, dot or "dit" (.): "dot duration" is one time unit long      
   * longer mark, dash or "dah" (-): three time units long
   * inter-element gap between the dots and dashes within a character: one dot duration or one unit long
   * short gap (between letters): three time units long      
   * medium gap (between words): seven time units long
   */

  int Sound_Pin = SA818_AF_IN;
  // ledc is used to create different tones
  int ledc_freq = 2000;
  int ledc_channel = 0;
  int ledc_resolution = 8;
  int wpm = 13;
  int morsetone = 800;
  int wpmduration = (60000) / (wpm*50);

  ledcAttachChannel(Sound_Pin, ledc_freq, ledc_resolution, ledc_channel);
  // convertedmessage
  Serial.println("Playing morse start");
  for (int i = 0; i < morse.length(); i++)
  {
    // only have . - / SPACE
    switch (morse.charAt(i))
    {
      case '.':
        ledcWriteTone(Sound_Pin, morsetone * 10);
        delay(1 * wpmduration);
        ledcWriteTone(Sound_Pin, 0);
        delay(1 * wpmduration);

        break;
      case '-':
        ledcWriteTone(Sound_Pin, morsetone * 10);
        delay(3 * wpmduration);
        ledcWriteTone(Sound_Pin, 0);
        delay(1 * wpmduration);

        break;
      case ' ':
        delay(2 * wpmduration);
        break;
      case '/':
        delay(6 * wpmduration);
        break;
    }
  }
  Serial.println("Playing morse end");
  ledcDetach(Sound_Pin);
}