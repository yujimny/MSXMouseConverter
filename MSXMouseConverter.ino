#include <hidboot.h> // Notice: Comment out right sentence or cannot get 0 move event. -> if(pmi->dX || pmi->dY) {
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

#define JoyPin1 2
#define JoyPin2 3
#define JoyPin3 4
#define JoyPin4 5
//      JoyPin5 +5V (N/C or power)
#define JoyPin6 6
#define JoyPin7 7
#define JoyPin8 12
//      JoyPin9 GND (Connect to Arduino GND)
#define InfoPin LED_BUILTIN // Status LED

char x=0,y=0,prevX=0,prevY=0,StatCnt=0,mstat=0,scale=25;
long time;

class MouseRptParser : public MouseReportParser
{
protected:
	void OnMouseMove	(MOUSEINFO *mi);
	void OnLeftButtonUp	(MOUSEINFO *mi);
	void OnLeftButtonDown	(MOUSEINFO *mi);
	void OnRightButtonUp	(MOUSEINFO *mi);
	void OnRightButtonDown	(MOUSEINFO *mi);
	void OnMiddleButtonUp	(MOUSEINFO *mi);
	void OnMiddleButtonDown	(MOUSEINFO *mi);
};

/*
USBHostShield2.0のOnMouseMoveは移動量が0だと飛んでこないためマウスの動きが不安定になる
USBHostShield2.0のソースコードhidboot.cppの108行目付近OnMouseMoveを呼び出す部分の
if(pmi->dX || pmi->dY)
をコメントアウトすることで解決
https://github.com/felis/USB_Host_Shield_2.0/blob/master/hidboot.cpp
ArduinoでUSB Mouseを扱うことに関しては以下を参考にした
https://qiita.com/topipo/items/0b2ca0eb48e90b4a5945
*/
void MouseRptParser::OnMouseMove(MOUSEINFO *mi)
{
  x = -mi->dX * scale / 20;
  y = -mi->dY * scale / 20;
  Serial.print("dx=");
  Serial.print(mi->dX, DEC);
  Serial.print(" dy=");
  Serial.println(mi->dY, DEC);
};
void MouseRptParser::OnLeftButtonUp	(MOUSEINFO *mi)
{
  mstat &= ~1;
  Serial.println("L Butt Up");
};
void MouseRptParser::OnLeftButtonDown	(MOUSEINFO *mi)
{
  mstat |= 1;
  Serial.println("L Butt Dn");
};
void MouseRptParser::OnRightButtonUp	(MOUSEINFO *mi)
{
  mstat &= ~2;
  Serial.println("R Butt Up");
};
void MouseRptParser::OnRightButtonDown	(MOUSEINFO *mi)
{
  mstat |= 2;
  Serial.println("R Butt Dn");
};
void MouseRptParser::OnMiddleButtonUp	(MOUSEINFO *mi)
{
  scale -= 5;
  if (scale < 5) scale = 25;
  Serial.println("M Butt Up");
};
void MouseRptParser::OnMiddleButtonDown	(MOUSEINFO *mi)
{
  Serial.println("M Butt Dn");
};

USB     Usb;
USBHub     Hub(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE>    HidMouse(&Usb);

MouseRptParser                               Prs;


void setup() {
  digitalWrite(JoyPin1,LOW);
  digitalWrite(JoyPin2,LOW);
  digitalWrite(JoyPin3,LOW);
  digitalWrite(JoyPin4,LOW);
  digitalWrite(JoyPin6,LOW);
  digitalWrite(JoyPin7,LOW);

  digitalWrite(InfoPin,LOW);
  pinMode(InfoPin,OUTPUT);

  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
    Serial.println("OSC did not start.");

  delay( 200 );

  HidMouse.SetReportParser(0, &Prs);
}

void sendMSX(char c)
{
    while (digitalRead(JoyPin8)==LOW) {if (millis()>time) return;};
    if(c&128) pinMode(JoyPin4,INPUT); else pinMode(JoyPin4,OUTPUT);
    if(c&64)  pinMode(JoyPin3,INPUT); else pinMode(JoyPin3,OUTPUT);
    if(c&32)  pinMode(JoyPin2,INPUT); else pinMode(JoyPin2,OUTPUT);
    if(c&16)  pinMode(JoyPin1,INPUT); else pinMode(JoyPin1,OUTPUT);
    while (digitalRead(JoyPin8)==HIGH) {if (millis()>time) return;};
    if(c&8)   pinMode(JoyPin4,INPUT); else pinMode(JoyPin4,OUTPUT);
    if(c&4)   pinMode(JoyPin3,INPUT); else pinMode(JoyPin3,OUTPUT);
    if(c&2)   pinMode(JoyPin2,INPUT); else pinMode(JoyPin2,OUTPUT);
    if(c&1)   pinMode(JoyPin1,INPUT); else pinMode(JoyPin1,OUTPUT);
}
      
void JoyHigh()
{
    pinMode(JoyPin1,INPUT);
    pinMode(JoyPin2,INPUT);
    pinMode(JoyPin3,INPUT);
    pinMode(JoyPin4,INPUT);
}

void MsxMouse()
{
  StatCnt++;
  if (StatCnt==50) StatCnt=0;
  time=millis()+40;
  
  if(mstat&1) pinMode(JoyPin6,OUTPUT); else pinMode(JoyPin6,INPUT);
  if(mstat&2) pinMode(JoyPin7,OUTPUT); else pinMode(JoyPin7,INPUT);

  if (x == prevX) x = 0;
  if (y == prevY) y = 0;
  prevX = x;
  prevY = y;
  sendMSX(x);
  sendMSX(y);
  if (millis()<time) {
      if (StatCnt<25) digitalWrite(InfoPin,LOW); else digitalWrite(InfoPin,HIGH);
      sendMSX(0);
      sendMSX(0);
      time=millis()+2;
    } else if (StatCnt<48) digitalWrite(InfoPin,LOW); else digitalWrite(InfoPin,HIGH);

  while (digitalRead(JoyPin8)==LOW) {if (millis()>time) break;};
  JoyHigh();
}

void loop() {
  Usb.Task();
  MsxMouse();
}
