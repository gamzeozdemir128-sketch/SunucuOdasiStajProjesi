#include <Keypad.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(12,11,A2,A3,A4,A5);

#define KILIT_ROLE 13
#define BUZZER_PIN 9

const byte SATIR=4;
const byte SUTUN=3;

char tuslar[SATIR][SUTUN]={
  {'1','5','9'},
  {'2','6','0'},
  {'3','7','*'},
  {'4','8','#'}
};

byte satirPinleri[SATIR]={2,3,4,5};
byte sutunPinleri[SUTUN]={6,7,8};

Keypad keypad=Keypad(makeKeymap(tuslar),satirPinleri,sutunPinleri,SATIR,SUTUN);


String DOGRU_SIFRE=""; 
String girilenSifre="";

unsigned long ekranZamanlayici = 0;
bool ekranBeklemede = false;
bool sistemHazir = false; 

void setup(){
  Serial.begin(115200);
  Serial.setTimeout(20); 

  pinMode(KILIT_ROLE,OUTPUT);
  pinMode(BUZZER_PIN,OUTPUT);

  digitalWrite(KILIT_ROLE,LOW);
  digitalWrite(BUZZER_PIN,LOW);

  lcd.begin(16,2);
  
 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Sistem   ");
  lcd.setCursor(0,1);
  lcd.print("  Baglaniyor...");
}

void loop() {
  
  espSifreDinle();

  if (!sistemHazir) {
    return; 
  }

  if (ekranBeklemede && (millis() - ekranZamanlayici >= 2000)) {
    ekranBeklemede = false;
    resetEkran();
  }

  char tus=keypad.getKey();
  if(!tus)return;

  if (ekranBeklemede) {
    ekranBeklemede = false;
    resetEkran();
  }

  digitalWrite(BUZZER_PIN,HIGH);
  delay(40); 
  digitalWrite(BUZZER_PIN,LOW);

  if(tus=='#'){
    lcd.clear();

    if(girilenSifre==DOGRU_SIFRE){
      lcd.setCursor(0,0);
      lcd.print("SIFRE DOGRU!");

      digitalWrite(BUZZER_PIN,HIGH);
      delay(500);
      digitalWrite(BUZZER_PIN,LOW);

      digitalWrite(KILIT_ROLE,HIGH); 
      Serial.println("ACCESS_GRANTED");

      for(int i=3;i>0;i--){
        lcd.setCursor(0,1);
        lcd.print("Kilit Acildi. ");
        lcd.print(i);
        lcd.print(" ");
        delay(1000); 
      }

      digitalWrite(KILIT_ROLE,LOW); 
      delay(300);       
      lcd.begin(16, 2);      
      resetEkran();
    }else{
      lcd.setCursor(0,0);
      lcd.print("HATALI SIFRE!");
      

      digitalWrite(BUZZER_PIN,HIGH); delay(120); digitalWrite(BUZZER_PIN,LOW);
      delay(80);
      digitalWrite(BUZZER_PIN,HIGH); delay(120); digitalWrite(BUZZER_PIN,LOW);

      Serial.println("ACCESS_DENIED");
      ekranZamanlayici = millis();
      ekranBeklemede = true;
    }
  }
  else if(tus=='*'){
    if(girilenSifre.length()>0){
      girilenSifre.remove(girilenSifre.length()-1);
      ekraniGuncelle();
    }
  }
  else{
    if(girilenSifre.length()<4){
      girilenSifre+=tus;
      ekraniGuncelle();
    }
  }
}

void espSifreDinle(){
  if(Serial.available() > 0){ 
    String gelen=Serial.readStringUntil('\n');
    gelen.trim();

    if(gelen.startsWith("SET_PASS:")){
      String yeni=gelen.substring(9);
      yeni.trim();

      bool rakam=true;
      for(byte i=0;i<yeni.length();i++){
        if(!isDigit(yeni[i])){rakam=false;break;}
      }

      if(yeni.length()==4 && rakam){
        DOGRU_SIFRE=yeni; 

        
        if (!sistemHazir) {
          sistemHazir = true;
          
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("    BAGLANTI     ");
          lcd.setCursor(0,1);
          lcd.print("    BASARILI!    ");
          
          digitalWrite(BUZZER_PIN,HIGH);delay(100);digitalWrite(BUZZER_PIN,LOW);
          delay(100);
          digitalWrite(BUZZER_PIN,HIGH);delay(100);digitalWrite(BUZZER_PIN,LOW);
          
          delay(1500);
          resetEkran();
          return;
        }

      

        digitalWrite(BUZZER_PIN,HIGH);delay(100);digitalWrite(BUZZER_PIN,LOW);
        delay(100);
        digitalWrite(BUZZER_PIN,HIGH);delay(100);digitalWrite(BUZZER_PIN,LOW);

        ekranZamanlayici = millis();
        ekranBeklemede = true;
      }
    }
  }
}

void ekraniGuncelle(){
  lcd.setCursor(0,1);
  lcd.print("Sifre:          ");
  lcd.setCursor(7,1);
  for(byte i=0;i<girilenSifre.length();i++) lcd.print("*");
}

void resetEkran(){
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  HOS GELDINIZ  ");
  lcd.setCursor(0,1);
  lcd.print("Sifre Giriniz:");
  girilenSifre="";
}
