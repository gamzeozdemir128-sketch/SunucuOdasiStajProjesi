#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

const char* ssid = "WIFI_ADI";
const char* password = "********";

const char* FIREBASE_HOST = "sunucuodasitakipvesogutma-default-rtdb.firebaseio.com";
const char* FIREBASE_AUTH = "FIREBASE_AUTH_BILGISI";

#define DHTPIN 0
#define BUZZERPIN 14
#define LED_YESIL 12
#define LED_SARI 13
#define LED_KIRMIZI 15
#define ROLE_PIN 16
#define DHTTYPE DHT11

LiquidCrystal_I2C lcd(0x3F,16,2);
DHT dht(DHTPIN,DHTTYPE);

float sonNem=0;
float sonSicaklik=0;
bool fanAktif=false;
int sonDurumLogID=-1;

unsigned long sonSensorZamani=0;
unsigned long sonSifreZamani=0;
unsigned long sonSeriZamani=0;

String eskiBulutSifresi="";

void firebaseIstegi(String method,String path,String payload=""){
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url="https://"+String(FIREBASE_HOST)+path+"?auth="+String(FIREBASE_AUTH);
  if(!http.begin(client,url)) return;

  int code=-1;
  if(method=="PATCH") code=http.PATCH(payload);
  else if(method=="POST") code=http.POST(payload);
  else if(method=="PUT") code=http.PUT(payload);
  else if(method=="GET") code=http.GET();

  Serial.print("Firebase ");Serial.print(method);Serial.print(" ");
  Serial.print(path);Serial.print(" -> ");Serial.println(code);
  http.end();
}

void bulutaVeriYolla(){
  String json="{\"sicaklik\":"+String(sonSicaklik,1)+
              ",\"nem\":"+String(sonNem,1)+
              ",\"fan\":"+(fanAktif?String("true"):String("false"))+"}";
  firebaseIstegi("PATCH","/SistemVerileri.json",json);
}

void gecmisKaydet(String path,float deger){
  String json="{\"deger\":"+String(deger,1)+",\"timestamp\":{\".sv\":\"timestamp\"}}";
  firebaseIstegi("POST",path+".json",json);
}

void logEkle(String seviye,String kaynak,String aciklama){
  aciklama.replace("\"","'");
  kaynak.replace("\"","'");
  seviye.replace("\"","'");
  String json="{\"zaman\":\"Anlık Zaman\",\"seviye\":\""+seviye+
              "\",\"kaynak\":\""+kaynak+
              "\",\"aciklama\":\""+aciklama+
              "\",\"timestamp\":{\".sv\":\"timestamp\"}}";
  firebaseIstegi("POST","/OlayKayitlari.json",json);
}

String sifreCek(){
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url="https://"+String(FIREBASE_HOST)+"/serverguard/aktif_sifre.json?auth="+String(FIREBASE_AUTH);
  String s="";
  if(http.begin(client,url)){
    int code=http.GET();
    if(code==200){s=http.getString();s.replace("\"","");s.trim();}
    http.end();
  }
  return s;
}

void arduinoyaSifreGonder(String sifre){
  if(sifre.length()==4){
    Serial.println("SET_PASS:"+sifre);
    Serial.flush();
    Serial.println("SET_PASS:"+sifre);
  }
}

void sifreKontrol(){
  String s=sifreCek();
  if(s.length()==4){
    if(s!=eskiBulutSifresi){
      eskiBulutSifresi=s;
      arduinoyaSifreGonder(s);
      logEkle("Bilgi","ServerGuard","Şifre değiştirildi.");
    }
  }
}

int durum(float t){
  if(t<18)return 0;
  if(t<27)return 1;
  if(t<31)return 2;
  if(t<36)return 3;
  return 4;
}

void sistemiUygula(float t){
  int d=durum(t);
  if(d==0){
    digitalWrite(LED_YESIL,LOW);digitalWrite(LED_SARI,LOW);digitalWrite(LED_KIRMIZI,HIGH);
    digitalWrite(BUZZERPIN,HIGH);digitalWrite(ROLE_PIN,HIGH);fanAktif=false;
  }else if(d==1){
    digitalWrite(LED_YESIL,HIGH);digitalWrite(LED_SARI,LOW);digitalWrite(LED_KIRMIZI,LOW);
    digitalWrite(BUZZERPIN,LOW);digitalWrite(ROLE_PIN,HIGH);fanAktif=false;
  }else if(d==2){
    digitalWrite(LED_YESIL,LOW);digitalWrite(LED_SARI,HIGH);digitalWrite(LED_KIRMIZI,LOW);
    digitalWrite(BUZZERPIN,LOW);digitalWrite(ROLE_PIN,HIGH);fanAktif=false;
  }else if(d==3){
    digitalWrite(LED_YESIL,LOW);digitalWrite(LED_SARI,LOW);digitalWrite(LED_KIRMIZI,HIGH);
    digitalWrite(BUZZERPIN,LOW);digitalWrite(ROLE_PIN,LOW);fanAktif=true;
  }else{
    digitalWrite(LED_YESIL,LOW);digitalWrite(LED_SARI,LOW);digitalWrite(LED_KIRMIZI,HIGH);
    digitalWrite(BUZZERPIN,HIGH);digitalWrite(ROLE_PIN,LOW);fanAktif=true;
  }

  if(d!=sonDurumLogID){
    if(d==0)logEkle("Düşük","Sıcaklık","Alt sıcaklık limiti aşıldı.");
    if(d==1)logEkle("Normal","Sıcaklık","Ortam sıcaklığı güvenli aralıkta.");
    if(d==2)logEkle("Uyarı","Sıcaklık","Sıcaklık uyarı tolerans sınırında.");
    if(d==3)logEkle("Yüksek","Havalandırma","Sıcaklık yüksek seviyede.Otomatik soğutma aktif!");
    if(d==4)logEkle("Kritik","Alarm","Sıcaklık kritik düzeye ulaştı.Acil durum alarmı aktif!");
    sonDurumLogID=d;
  }
}

void arduinoMesajlariniOku(){
  while(Serial.available()){
    String mesaj=Serial.readStringUntil('\n');
    mesaj.trim();
    if(mesaj=="ACCESS_DENIED"){
      logEkle("Yetkisiz","ServerGuard","Yetkisiz giriş denemesi!!!");
    }else if(mesaj=="ACCESS_GRANTED"){
      logEkle("Bilgi","ServerGuard","Yetkili giriş başarılı. Kilit açıldı.");
    }
  }
}

void setup(){
  Serial.begin(115200);
  Serial.setTimeout(80);

  pinMode(BUZZERPIN,OUTPUT);
  pinMode(LED_YESIL,OUTPUT);
  pinMode(LED_SARI,OUTPUT);
  pinMode(LED_KIRMIZI,OUTPUT);
  pinMode(ROLE_PIN,OUTPUT);

  digitalWrite(ROLE_PIN,HIGH);
  digitalWrite(BUZZERPIN,LOW);
  digitalWrite(LED_YESIL,LOW);
  digitalWrite(LED_SARI,LOW);
  digitalWrite(LED_KIRMIZI,LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("   Wi-Fi   ");
  lcd.setCursor(0,1);
  lcd.print(" Baglaniyor...");

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){delay(500);Serial.print(".");}

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" SISTEM AKTIF ");
  

  dht.begin();
  delay(1000);

  logEkle("Bilgi","Sistem","Merkezi izleme aktif");

  
  sifreKontrol();
}

void loop(){
  arduinoMesajlariniOku();

  if(millis()-sonSensorZamani>=3000){
    sonSensorZamani=millis();

    sonNem = dht.readHumidity() * 1.3889;
    sonSicaklik=dht.readTemperature();

    if(isnan(sonNem)||isnan(sonSicaklik)){
      lcd.clear();lcd.setCursor(0,0);lcd.print("Sensor Hatasi!");
      return;
    }

    sistemiUygula(sonSicaklik);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Sicaklik:");
    lcd.print(sonSicaklik,1);
    lcd.print(" C");
    if(durum(sonSicaklik)>=3){lcd.setCursor(12,0);lcd.print("!!!!");}
    lcd.setCursor(0,1);
    lcd.print("Nem:");
    lcd.print(sonNem,1);
    lcd.print("% RH");
    if(fanAktif){lcd.setCursor(12,1);lcd.print("FAN AKTİF");}

    if(WiFi.status()==WL_CONNECTED){
      bulutaVeriYolla();
      gecmisKaydet("/SicaklikGecmisi",sonSicaklik);
      gecmisKaydet("/NemGecmisi",sonNem);
    }
  }

  if(millis()-sonSifreZamani>=9000){
    sonSifreZamani=millis();
    if(WiFi.status()==WL_CONNECTED)sifreKontrol();
  }
}
