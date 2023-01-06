//---------------------------------------------БИБЛИОТЕКИ---------------------------------------------\\

#define OV7670_PIXEL_CLOCK_PIN 9 //обьявление пина PCLK на 9 пин ардуино уно (у меня подключение отличается от классического)
#define OV7670_PIXEL_CLOCK (PINB & 0b00000010) //обьявление пина PCLK на 9 пин ардуино уно

#include"CameraOV7670.h"
#include "SdFat.h"//стандартная библиотека SD.h довольно медленная, поэтому использую sdFat.h
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

//---------------------------------------------НАСТРОЙКИ_КАМЕРЫ_И_SD_КАРТЫ---------------------------------------------\\

SdFat32 sd;//объект SD карты
File32 file;//объект файла

//параметры картинки и настройка камеры, обьявление необходимых переменных
void processRgb160x120FrameBuffered();
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud  = 9600;
const uint16_t lineBufferLength = lineLength * 2;//длинна буфера строки в 2 раза болше количества пикселей строки т.к. 1 пиксель = 2 байта
uint8_t lineBuffer [lineBufferLength];
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_RGB565, 34);

//---------------------------------------------РАДИО---------------------------------------------\\

RF24 radio(10, 8);//объект радиомодуля

uint64_t readingPipe = 0xff7f7f7f7f, writingPipe = 0xff6f6f6f6f;//имена труб отправки и чтения
byte readingChannel = 0x70, writingChannel = 0x60;//номера каналов передачи и чтения
byte command = 0;//код текущей команды
byte outBuf = 0;
const byte commandCaptureImage = 0xFF;
const byte commandTransmitImage = 0xEE;
const byte commandImageStart = 0xBB;

void radioSettings(void)//устанавливает все настройки радио.
{
  radio.begin();
  delay(2000);
  radio.setAutoAck(1);
  radio.setRetries(0, 15);
  radio.enableAckPayload();
  radio.setPayloadSize(32);

  radio.openReadingPipe(1, readingPipe);//открытие трубы для чтения
  radio.openWritingPipe(writingPipe);//открытие трубы для отправки

  radio.setPALevel (RF24_PA_MAX);
  radio.setDataRate (RF24_1MBPS);
  radio.powerUp();
  delay(100);
}

void toRX (void)//переключает режим радио на приём и переходит на соответсвующий канал связи.
{
  delay(100);
  radio.setChannel(readingChannel);
  radio.startListening();
  delay(100);
}

void toTX (void)//переключает режим радио на передачу и переходит на соответсвующий канал связи.
{
  delay(100);
  radio.setChannel(writingChannel);
  radio.stopListening();
  delay(100);
}

void transmitFile(void)//передаёт весь файл кадра по радио
{
  toTX();
  outBuf = 0;
  
  while(!radio.write(&commandImageStart, sizeof(commandImageStart))){;}
  delay(100);
  Serial.println("test1");

  file = sd.open("IMG.dat", O_READ);
  
  while(file.available())
  {
    outBuf = file.read();
    while(!radio.write(&outBuf, sizeof(outBuf))){;}
    Serial.println("test2");
  }
  file.close();
}

byte waitForCommand(void)//ожидает каких либо команд по радио. Возвращяет код команды.
{
  byte incCom = 0;
  toRX();
  while(!radio.available()){;}
  radio.read(&incCom, sizeof(incCom));
  return(incCom);
}

void commandDone(void)//отправляет сообщение по радио, говорящее о выполнении какой-то команды.
{
  byte outBuf = 0xAA;
  toTX();
  while(!radio.write(&outBuf, sizeof(outBuf))){;}
  delay(100);
}

//---------------------------------------------ЗАБОР_КАРТИНКИ---------------------------------------------\\

//основной процесс программы. записывает картинку на sd карту
void processFrame()
{
  sd.remove("IMG.dat");//удаление старого файла картинки. пока их не нужно оставлять.
  file = sd.open("IMG.dat", O_WRITE | O_CREAT);//создание файла для картинки.
  //файл обязательно .dat. В тектовый файл числа будут записываться посимвольно.
  //Пример: 255 в .dat - это только 255. В .txt это 50 53 53 три байта на три символа (в соответсвии с системой ASCII).
  //это позволяет писать в файл только числа, без разделителей, что ускоряет запись.
  noInterrupts();//отключение прерываний. запрещает микроконтроллеру "отвлекаться" от записи кадра и ускоряет работу микроконтроллера
  processRgb160x120FrameBuffered();//функция забора кадра
  interrupts();//возврат прерываний
  file.close();//закрытие файла
}

//основной процесс забора картинки для 160x120
void processRgb160x120FrameBuffered()
{
  //задержки перед кадром
  camera.waitForVsync();

  for (uint8_t i = 0; i < 2; i++)
  {
    camera.ignoreHorizontalPaddingLeft();
    for (uint16_t x = 0; x < 160 * 2; x++)
    {
      camera.waitForPixelClockRisingEdge();
    }
    camera.ignoreHorizontalPaddingRight();
  }

  // начало основного цикла забора картинки
  for (uint16_t y = 0; y < lineCount; y++)
  {
    camera.ignoreHorizontalPaddingLeft();

    for (uint16_t x = 0; x < lineBufferLength;  x++)
    {
      camera.waitForPixelClockRisingEdge();
      camera.readPixelByte(lineBuffer[x]);
    }

    camera.ignoreHorizontalPaddingRight();

    //запись массива строки на sd карту
    file.write(lineBuffer, lineBufferLength);
    file.flush();
  }
}

//---------------------------------------------ГЛАВНЫЙ_РАЗДЕЛ---------------------------------------------\\

void setup()
{
  Serial.begin(baud);//старт UART
  camera.init();//старт камеры
  sd.begin(16);//старт sd карты
  radioSettings();//старт радио
}

void loop()
{
  command = waitForCommand();
  delay(100);

  if(command == commandCaptureImage)
  {
    processFrame();
    commandDone();
  }

  if(command == commandTransmitImage)
  {
    transmitFile();
    commandDone();
  }
}
