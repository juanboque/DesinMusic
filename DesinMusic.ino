/***********************************************************************************
*This program is a demo of how to display a bmp picture from SD card
*This demo was made for LCD modules with 8bit or 16bit data port.
*This program requires the the LCDKIWI library.

* File                : show_bmp_picture.ino
* Hardware Environment: Arduino UNO
* Build Environment   : Arduino

*Set the pins to the correct ones for your development shield or breakout board.
*This demo use the BREAKOUT BOARD only and use these 8bit data lines to the LCD,
*pin usage as follow:
*                  LCD_CS  LCD_CD  LCD_WR  LCD_RD  LCD_RST  SD_SS  SD_DI  SD_DO  SD_SCK 
*     Arduino Uno    A3      A2      A1      A0      A4      10     11     12      13                                                     

*                  LCD_D0  LCD_D1  LCD_D2  LCD_D3  LCD_D4  LCD_D5  LCD_D6  LCD_D7  
*     Arduino Uno    8       9       2       3       4       5       6       7

*Remember to set the pins to suit your display module!
*
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE 
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**********************************************************************************/

#include <SD.h>
#include <SPI.h>
#include <TouchScreen.h> //touch library
#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library


//if the IC model is known or the modules is unreadable,you can use this constructed function
//LCDWIKI_KBV my_lcd(ILI9341,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset
LCDWIKI_KBV my_lcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset
//if the IC model is not known and the modules is readable,you can use this constructed function
//LCDWIKI_KBV my_lcd(320,480,A3,A2,A1,A0,A4);//width,height,cs,cd,wr,rd,reset
//param calibration from kbv
#define TS_MINX 906
#define TS_MAXX 116

#define TS_MINY 92 
#define TS_MAXY 952

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define GREY    0xDDDD
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define PIXEL_NUMBER  (my_lcd.Get_Display_Width()/4)
#define FILE_NUMBER 3
#define FILE_NAME_SIZE_MAX 20
#define NO_SCREEN -1
#define PORTADA 0
#define MENU 1
#define UVC2MIN 2
#define UVC3MIN 3
#define UVC4MIN 4
#define UVC5MIN 5
#define UVC7MIN 7
#define UVC10MIN 10
#define OZONO2MIN 11
#define OZONO3MIN 12
#define OZONO4MIN 13
#define OZONO5MIN 14
#define OZONO7MIN 15
#define OZONO10MIN 16
#define INTERVALO 1000

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define MINPRESSURE 10
#define MAXPRESSURE 1000
#define UV_C_LED 23
#define OZONO_ON 25
#define FAN 27
// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

/*MQ131*/
const int MQ_PIN = A15;      // Pin del sensor
//const int RL_VALUE = 5;      // Resistencia RL del modulo en Kilo ohms
const int RL_VALUE = 120;      // Resistencia RL del modulo en Kilo ohms
//const int R0 = 10;          // Resistencia R0 del sensor en Kilo ohms
const int R0 = 122;          // Resistencia R0 del sensor en Kilo ohms
 
// Datos para lectura multiple
const int READ_SAMPLE_INTERVAL = 10;    // Tiempo entre muestras
const int READ_SAMPLE_TIMES = 5;       // Numero muestras
 
// Ajustar estos valores para vuestro sensor según el Datasheet
// (opcionalmente, según la calibración que hayáis realizado)
const float X0 = 10;
const float Y0 = 1;
const float X1 = 1000;
const float Y1 = 8;
 
// Puntos de la curva de concentración {X, Y}
const float punto0[] = { log10(X0), log10(Y0) };
const float punto1[] = { log10(X1), log10(Y1) };
 
// Calcular pendiente y coordenada abscisas
const float scope = (punto1[1] - punto0[1]) / (punto1[0] - punto0[0]);
const float coord = punto0[1] - punto0[0] * scope;
/*END MQ131*/

uint32_t bmp_offset = 0;
uint16_t s_width = my_lcd.Get_Display_Width();  
uint16_t s_heigh = my_lcd.Get_Display_Height();
//int16_t PIXEL_NUMBER;

//bool bPortadaOpen = false; //to open bmp only once
//bool bMenuOpen = false; //to open bmp only once
//bool bUVC2MIN = false;
bool bStop = false;
int iPantalla = -1;
bool bSDisOK = true;
//int iCS = 10; //Chip Select SPI UNO
int iCS = 53; //Chip Select SPI MEGA
String sTime;
String sConcentration;
int iMin = 2;
int iSecond = 00;

char file_name[FILE_NUMBER][FILE_NAME_SIZE_MAX] ={{"portada.bmp"},{"menu.bmp"},{"UVC2min.bmp"}};

uint16_t read_16(File fp)
{
    uint8_t low;
    uint16_t high;
    low = fp.read();
    high = fp.read();
    return (high<<8)|low;
}

uint32_t read_32(File fp)
{
    uint16_t low;
    uint32_t high;
    low = read_16(fp);
    high = read_16(fp);
    return (high<<8)|low;   
 }
 
bool analysis_bpm_header(File fp)
{
    if(read_16(fp) != 0x4D42)
    {
      return false;  
    }
    //get bpm size
    read_32(fp);
    //get creator information
    read_32(fp);
    //get offset information
    bmp_offset = read_32(fp);
    //get DIB infomation
    read_32(fp);
    //get width and heigh information
    uint32_t bpm_width = read_32(fp);
    uint32_t bpm_heigh = read_32(fp);
    if((bpm_width != s_width) || (bpm_heigh != s_heigh))
    {
      return false; 
    }
    if(read_16(fp) != 1)
    {
        return false;
    }
    read_16(fp);
    if(read_32(fp) != 0)
    {
      return false; 
     }
     return true;
}

void draw_bmp_picture(File fp)
{
  uint16_t i,j,k,l,z,m=0;
  uint8_t bpm_data[PIXEL_NUMBER*3] = {0};
  uint16_t bpm_color[PIXEL_NUMBER];
  fp.seek(bmp_offset);

//  Serial.print("Valor s_heigh:");
//    Serial.println(s_heigh);
//    Serial.print("Valor s_width:");
//    Serial.println(s_width);
  for(i = 0;i < s_heigh;i++)
  {
//    Serial.print("Valor i:");
//    Serial.println(i);
    for(j = 0;j<s_width/PIXEL_NUMBER;j++)
    {
//      Serial.print("Valor j:");
//      Serial.println(j);
      m = 0;
      fp.read(bpm_data,PIXEL_NUMBER*3);
      for(k = 0;k<PIXEL_NUMBER;k++)
      {
//       Serial.print("Valor k:");
//        Serial.println(k);
        bpm_color[k]= my_lcd.Color_To_565(bpm_data[m+2], bpm_data[m+1], bpm_data[m+0]); //change to 565
        m +=3;
        my_lcd.Set_Draw_color(bpm_color[k]);
        my_lcd.Draw_Pixel(j*PIXEL_NUMBER+k,i);
        //my_lcd.Draw_Pixel(i, j*PIXEL_NUMBER+k);
      }
     }
   }    
}

void CountDownStr(int *iMin, int *iSecond, String *sTime)
{
  
  *sTime = *iMin;// + ":" + iSecond;
  *sTime += ":";

  if (*iSecond == 0)
    *sTime += "00";
  else if (*iSecond == 1)
    *sTime += "01";
  else if (*iSecond == 2)
    *sTime += "02";
  else if (*iSecond == 3)
    *sTime += "03";
  else if (*iSecond == 4)
    *sTime += "04";
  else if (*iSecond == 5)
    *sTime += "05";
  else if (*iSecond == 6)
    *sTime += "06";
  else if (*iSecond == 7)
    *sTime += "07";
  else if (*iSecond == 8)
    *sTime += "08";
  else if (*iSecond == 9)
    *sTime += "09";
  else
    *sTime += *iSecond;

  if (*iSecond == 0)
  {
    *iSecond = 59;
    *iMin = *iMin - 1;
  }
  else
    *iSecond = *iSecond - 1;
}

void LoadPicFromSDCard(int iPic)
{
      File bmp_file;
      
      bmp_file = SD.open(file_name[iPic]);
      if(!bmp_file)
      {
          my_lcd.Set_Text_Back_colour(WHITE);
          my_lcd.Set_Text_colour(BLUE);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("didnt find BMP!",0,30);
          my_lcd.Fill_Screen(BLACK);
          bSDisOK = false;
         // return false;
      } else if (!analysis_bpm_header(bmp_file))
      {  
          my_lcd.Set_Text_Back_colour(WHITE);
          my_lcd.Set_Text_colour(BLUE);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("bad bmp picture!",0,0);
          my_lcd.Fill_Screen(BLACK);
          bSDisOK = false;
         // return false;
      } else
      {
        draw_bmp_picture(bmp_file);
        bmp_file.close(); 
        bSDisOK = true;
       // return true;  
      }  

}

void LoadMenu(int iMinuteMode)
{
    my_lcd.Fill_Screen(BLACK);
    switch (iMinuteMode) {
      case OZONO7MIN:
          //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          //my_lcd.Fill_Round_Rectangle(30, 70, 70, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("OZONO",50, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(GREEN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("0.00 ppm",70, 100);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("7",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case OZONO5MIN:
          //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          //my_lcd.Fill_Round_Rectangle(30, 70, 70, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("OZONO",50, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(GREEN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("0.00 ppm",70, 100);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("5",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case OZONO4MIN:
          //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          //my_lcd.Fill_Round_Rectangle(30, 70, 70, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("OZONO",50, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(GREEN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("0.00 ppm",70, 100);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("4",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case OZONO3MIN:
          //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          //my_lcd.Fill_Round_Rectangle(30, 70, 70, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("OZONO",50, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(GREEN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("0.00 ppm",70, 100);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("3",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case OZONO2MIN:
          //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          //my_lcd.Fill_Round_Rectangle(30, 70, 70, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("OZONO",50, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(GREEN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("0.00 ppm",70, 100);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("2",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case UVC2MIN:
          //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          //my_lcd.Fill_Round_Rectangle(30, 70, 70, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("UV-C",75, 70);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("2",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Mode(2);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_colour(MAGENTA);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("WWW.MUSIKARTE.NET",10,0);      
          my_lcd.Set_Text_Mode(3);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case UVC3MIN:
        // statements
        //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("UV-C",75, 70);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("3",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Mode(2);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_colour(MAGENTA);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("WWW.MUSIKARTE.NET",10,0);      
          my_lcd.Set_Text_Mode(3);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case UVC4MIN:
        // statements
        //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("UV-C",75, 70);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("4",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Mode(2);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_colour(MAGENTA);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("WWW.MUSIKARTE.NET",10,0);      
          my_lcd.Set_Text_Mode(3);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case UVC5MIN:
        // statements
        //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("UV-C",75, 70);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("5",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Mode(2);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_colour(MAGENTA);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("WWW.MUSIKARTE.NET",10,0);      
          my_lcd.Set_Text_Mode(3);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case UVC7MIN:
        // statements
        //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("UV-C",75, 70);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("7",85, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",160, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Mode(2);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_colour(MAGENTA);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("WWW.MUSIKARTE.NET",10,0);      
          my_lcd.Set_Text_Mode(3);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case OZONO10MIN:
        // statements
        //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          //my_lcd.Fill_Round_Rectangle(30, 70, 70, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("OZONO",50, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(GREEN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("0.00 ppm",70, 100);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("10",60, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",190, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);    
          my_lcd.Set_Text_Mode(3);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      case UVC10MIN:
        // statements
        //debería ser un procedimiento: ShowMenu(UVC2MIN)
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Set_Text_Mode(5);
          my_lcd.Set_Text_Size(8);
          my_lcd.Set_Text_colour(CYAN);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("UV-C",75, 70);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Print_String("10",60, 175);
          my_lcd.Set_Text_Size(4);
          my_lcd.Print_String("MIN",190, 210);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_Size(5);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          my_lcd.Set_Text_Mode(2);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Text_colour(MAGENTA);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("WWW.MUSIKARTE.NET",10,0);      
          my_lcd.Set_Text_Mode(3);
          my_lcd.Set_Text_Size(2);
          my_lcd.Set_Text_colour(GREY);    
          my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      break;
      default:
        // statements
      break;
    }
  
}

void setup() 
{
   Serial.begin(9600);
   my_lcd.Init_LCD();
   Serial.println(my_lcd.Read_ID(), HEX);
   pinMode(13, OUTPUT);//ardunion uno es un 13
   pinMode(52, OUTPUT);//ardunion MEGA es un 52
   pinMode(UV_C_LED, OUTPUT);
   pinMode(OZONO_ON, OUTPUT);
   pinMode(FAN, OUTPUT);
   digitalWrite(OZONO_ON, HIGH); 
   digitalWrite(FAN, LOW);
   digitalWrite(UV_C_LED, LOW); //turn off LED
   
   //Init SD_Card int 
   pinMode(iCS, OUTPUT);
   if (!SD.begin(iCS)) 
   {
    my_lcd.Set_Text_Back_colour(WHITE);
    my_lcd.Set_Text_colour(BLUE);    
    my_lcd.Set_Text_Size(3);
    my_lcd.Print_String("SD Card Init fail!",0,0);
    bSDisOK = false;
    //return false;
  }
  my_lcd.Fill_Screen(BLACK);
}

void MenuGraficoPortada()
{
      my_lcd.Set_Text_Mode(1);
      my_lcd.Set_Text_Back_colour(BLACK);
      my_lcd.Set_Text_colour(MAGENTA);    
      my_lcd.Set_Text_Size(3);
      my_lcd.Print_String("WWW.MUSIKARTE.NET",10,0);
      my_lcd.Set_Text_Size(2);
      my_lcd.Set_Text_colour(GREY);    
      my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
      my_lcd.Set_Draw_color(BLUE);
      my_lcd.Fill_Round_Rectangle(30, 120, 290, 350, 5);
      my_lcd.Set_Text_colour(WHITE);    
      my_lcd.Set_Text_Size(3);
      my_lcd.Set_Text_Mode(2);
      my_lcd.Print_String("Toca para",70,150);
      my_lcd.Print_String("seleccionar",50,220);
      my_lcd.Print_String("desinfeccion",40,290);
}


// Obtener la resistencia promedio en N muestras
float readMQ(int mq_pin)
{
   float rs = 0;
   for (int i = 0;i<READ_SAMPLE_TIMES;i++) {
      rs += getMQResistance(analogRead(mq_pin));
      delay(READ_SAMPLE_INTERVAL);
   }
   return rs / READ_SAMPLE_TIMES;
}
 
// Obtener resistencia a partir de la lectura analogica
float getMQResistance(int raw_adc)
{
   return (((float)RL_VALUE / 1000.0*(1023 - raw_adc) / raw_adc));
}
 
// Obtener concentracion 10^(coord + scope * log (rs/r0)
float getConcentration(float rs_ro_ratio)
{
   return pow(10, coord + scope * log(rs_ro_ratio));
}

void LoadMenuGrafico()
{

  my_lcd.Fill_Screen(BLACK);
  my_lcd.Set_Text_Mode(2);
  my_lcd.Set_Text_Back_colour(BLACK);
  my_lcd.Set_Text_colour(MAGENTA);    
  my_lcd.Set_Text_Size(3);
  my_lcd.Print_String("WWW.MUSIKARTE.NET",10,0);      
  my_lcd.Set_Text_Mode(3);
  my_lcd.Set_Text_Size(2);
  my_lcd.Set_Text_colour(GREY);    
  my_lcd.Print_String("INFO@XKEMATIC.COM",65,465);
  my_lcd.Set_Text_Mode(9);
  my_lcd.Set_Text_Size(5);
  my_lcd.Set_Text_colour(WHITE);
  my_lcd.Set_Text_Back_colour(BLACK);
  my_lcd.Set_Draw_color(BLUE);/////////////////////////////////////////////////////////////////////////////////////////////
  my_lcd.Fill_Round_Rectangle(90, 110, 220, 155, 5);////////////////////////////////////////////////////////////////////////
  my_lcd.Print_String("UV-C",100, 115);

  my_lcd.Set_Text_Mode(9);
  my_lcd.Set_Text_Size(5);
  my_lcd.Set_Text_colour(WHITE);
  my_lcd.Set_Text_Back_colour(BLACK);
  my_lcd.Set_Draw_color(BLUE);/////////////////////////////////////////////////////////////////////////////////////////////
  my_lcd.Fill_Round_Rectangle(80, 275, 240, 325, 5);////////////////////////////////////////////////////////////////////////
  my_lcd.Print_String("OZONO",90, 280);

  my_lcd.Set_Text_Size(3);
  my_lcd.Set_Text_colour(WHITE);
  my_lcd.Set_Text_Back_colour(BLACK);
  my_lcd.Print_String("2",75, 170);
  my_lcd.Print_String("2",75, 335);
  my_lcd.Print_String("3",145, 170);
  my_lcd.Print_String("3",145, 335);
  my_lcd.Print_String("4",210, 170);
  my_lcd.Print_String("4",210, 335);
  my_lcd.Print_String("5",75, 208);
  my_lcd.Print_String("5",75, 373);
  my_lcd.Print_String("7",145, 208);
  my_lcd.Print_String("7",145, 373);
  my_lcd.Print_String("10",200, 208);
  my_lcd.Print_String("10",200, 373);
  my_lcd.Set_Text_Size(2);
  my_lcd.Set_Text_colour(WHITE);
  my_lcd.Set_Text_Back_colour(BLACK);
  my_lcd.Print_String("m",97, 177);// del 2
  my_lcd.Print_String("m",97, 342);// del 2
  my_lcd.Print_String("m",97, 215);//del 5
  my_lcd.Print_String("m",97, 380);//del 5
  my_lcd.Print_String("m",167, 215);//del 7
  my_lcd.Print_String("m",167, 380);//del 7
  my_lcd.Print_String("m",167, 177);//del 3
  my_lcd.Print_String("m",167, 342);//del 3
  my_lcd.Print_String("m",237, 177);//del 4
  my_lcd.Print_String("m",237, 342);//del 4
  my_lcd.Print_String("m",240, 215);//del 10
  my_lcd.Print_String("m",240, 380);//del 10  
}

void loop() 
{
    digitalWrite(13, HIGH); //ardunion uno es un 13
    digitalWrite(52, HIGH); //ardunion uno es un 13
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW); //ardunion uno es un 13
    digitalWrite(52, LOW); //ardunion uno es un 13
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    //Load Picture from SDCard
    if (iPantalla == NO_SCREEN)
    {
      if (bSDisOK)
      {
        LoadPicFromSDCard(PORTADA);
      } else
      {
        MenuGraficoPortada();
      }
      iPantalla = PORTADA;
    }


    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) 
    {
      if (iPantalla == PORTADA)
      {
        if (bSDisOK)
        {
          LoadPicFromSDCard(MENU);
        } else
        {
          LoadMenuGrafico();
        }
        iPantalla = MENU;
        p.x = 0;  
        p.y = 0;
      }
      p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
      /* MOSTRAR ESTO EN EL PUERTO SERIE NOS INDICA EN QUÉ ZONA ESTÁN LOS PIXELES QUE SE ESTÁN TOCANDO EN LA PANTALLA*/
//      Serial.print("p.x DESPUES del map: ");
//      Serial.println(p.x); //105
//      Serial.print("p.y DESPUES del map: ");
//      Serial.println(p.y); //105
      /* MOSTRAR AREAS DE MENUS PARA UV-C U OZONO Y SU TIEMPO*/
//      my_lcd.Set_Draw_color(RED);
//      my_lcd.Draw_Line(60, 165, 260, 165); //primera linea horizontal
//      my_lcd.Draw_Line(60, 200, 260, 200); //segunda linea horizontal
//      my_lcd.Draw_Line(60, 235, 260, 235); //tercera linea horizontal
//      
//      my_lcd.Draw_Line(60, 330, 260, 330); //cuarta linea horizontal
//      my_lcd.Draw_Line(60, 365, 260, 365); //quinta linea horizontal
//      my_lcd.Draw_Line(60, 400, 260, 400); //sexta linea horizontal
//      
//      my_lcd.Draw_Line(60, 165, 60, 400); //primera linea vertical que es común a todas las areas
//      
//      my_lcd.Draw_Line(135, 165, 135, 200); //segunda linea vertical de la primera fila de uvc entre los numeros 2 y 3
//      my_lcd.Draw_Line(195, 165, 195, 200); //tercera linea vertical de la primera fila de uvc entre los numeros 3 y 4
//      my_lcd.Draw_Line(125, 200, 125, 235); //segunda linea vertical de la segunda fila de uvc entre los numeros 5 y 7
//      my_lcd.Draw_Line(185, 200, 185, 235); //tercera linea vertical de la segunda fila de uvc entre los numeros 7 y 10
//
//      my_lcd.Draw_Line(135, 330, 135, 365); //segunda linea vertical de la primera fila de uvc entre los numeros 2 y 3
//      my_lcd.Draw_Line(195, 330, 195, 365); //tercera linea vertical de la primera fila de uvc entre los numeros 3 y 4
//      my_lcd.Draw_Line(125, 365, 125, 400); //segunda linea vertical de la segunda fila de uvc entre los numeros 5 y 7
//      my_lcd.Draw_Line(185, 365, 185, 400); //tercera linea vertical de la segunda fila de uvc entre los numeros 7 y 10
//      
//      my_lcd.Draw_Line(260, 165, 260, 400); //ultima linea vertical que es común a todas las areas
/* FIN MOSTRAR AREAS DE MENUS PARA UV-C U OZONO Y SU TIEMPO*/
      //void Draw_Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
      if (iPantalla == MENU)
      {// Aquí estoy con el menú activo. //debería ser una función con parámetros de entrada las coordenadas y salida la opcion marcada.
        if (((p.x >= 60) && (p.x <= 135)) && ((p.y >= 165) && (p.y <= 200)))
        {
            LoadMenu(UVC2MIN);
            iPantalla = UVC2MIN;
            //bUVC2MIN = true;
            //bMenuOpen = false;
        } else if (((p.x >= 135) && (p.x <= 195)) && ((p.y >= 165) && (p.y <= 200)))
        {
  //        //estoy en tres minutos UVC
            LoadMenu(UVC3MIN);
            iPantalla = UVC3MIN;
            //bUVC2MIN = true;
            //bMenuOpen = false;
        } else if (((p.x >= 195) && (p.x <= 260)) && ((p.y >= 165) && (p.y <= 200)))
        {
  //        //estoy en cuatro minutos UVC
            LoadMenu(UVC4MIN);
            iPantalla = UVC4MIN;
        } else if (((p.x >= 60) && (p.x <= 135)) && ((p.y >= 330) && (p.y <= 365)))
        {
  //        //estoy en dos minutos ozono
            LoadMenu(OZONO2MIN);
            iPantalla = OZONO2MIN;
        } else if (((p.x >= 135) && (p.x <= 195)) && ((p.y >= 330) && (p.y <= 365)))
        {
  //        //estoy en tres minutos OZONO
              LoadMenu(OZONO3MIN);
            iPantalla = OZONO3MIN;
        } else if (((p.x >= 195) && (p.x <= 260)) && ((p.y >= 330) && (p.y <= 365)))
        {
  //        //estoy en cuatro minutos OZONO
              LoadMenu(OZONO4MIN);
            iPantalla = OZONO4MIN;
        } else if (((p.x >= 60) && (p.x <= 125)) && ((p.y >= 200) && (p.y <= 235)))
        {
  //        //estoy en 5 minutos UVC
            LoadMenu(UVC5MIN);
            iPantalla = UVC5MIN;
        } else if (((p.x >= 125) && (p.x <= 185)) && ((p.y >= 200) && (p.y <= 235)))
        {
  //        //estoy en 7 minutos UVC
            LoadMenu(UVC7MIN);
            iPantalla = UVC7MIN;
        } else if (((p.x >= 185) && (p.x <= 260)) && ((p.y >= 200) && (p.y <= 235)))
        {
          //estoy en 10 minutos UVC
            LoadMenu(UVC10MIN);
            iPantalla = UVC10MIN;
        } else if (((p.x >= 60) && (p.x <= 125)) && ((p.y >= 365) && (p.y <= 400)))
        {
          //estoy en 5 minutos OZONO
                      LoadMenu(OZONO5MIN);
            iPantalla = OZONO5MIN;
        } else if (((p.x >= 125) && (p.x <= 185)) && ((p.y >= 365) && (p.y <= 400)))
        {
          //estoy en 7 minutos OZONO
                      LoadMenu(OZONO7MIN);
            iPantalla = OZONO7MIN;
        } else if (((p.x >= 185) && (p.x <= 260)) && ((p.y >= 365) && (p.y <= 400)))
        {
          //estoy en 10 minutos OZONO
                      LoadMenu(OZONO10MIN);
            iPantalla = OZONO10MIN;
        }
      }else if (iPantalla == OZONO10MIN)
     {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 10;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("10:00",30, 185);
          digitalWrite(OZONO_ON, LOW); 
          digitalWrite(FAN, HIGH); 
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            my_lcd.Fill_Round_Rectangle(30, 100, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            if (sTime == "10:00")
            {
              my_lcd.Print_String(sTime,30, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...  
            } else
            {
              my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...  
            }
            my_lcd.Set_Text_Size(4);
            my_lcd.Set_Text_colour(GREEN);
            my_lcd.Set_Text_Back_colour(BLACK);
            float rs_med = readMQ(MQ_PIN);      // Obtener la Rs promedio
            float concentration = getConcentration(rs_med/R0);   // Obtener la concentración
            sConcentration = concentration;
            sConcentration.concat(" ppm");
            my_lcd.Print_String(sConcentration,70, 100);
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(OZONO_ON, HIGH); //turn off LED
                  digitalWrite(FAN, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(OZONO_ON, HIGH); //turn off LED
            digitalWrite(FAN, LOW); //turn off LED
          }          
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("10:00",30, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 10; iSecond=0; //reset timer
        }
      } else if (iPantalla == OZONO7MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 7;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("7:00",55, 185);
          digitalWrite(OZONO_ON, LOW); //turn on OZONO
          digitalWrite(FAN, HIGH); //turn on OZONO
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            //my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Fill_Round_Rectangle(30, 100, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            my_lcd.Set_Text_Size(4);
            my_lcd.Set_Text_colour(GREEN);
            my_lcd.Set_Text_Back_colour(BLACK);
            float rs_med = readMQ(MQ_PIN);      // Obtener la Rs promedio
            float concentration = getConcentration(rs_med/R0);   // Obtener la concentración
            sConcentration = concentration;
            sConcentration.concat(" ppm");
            my_lcd.Print_String(sConcentration,70, 100);
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(OZONO_ON, HIGH); //turn off LED
                  digitalWrite(FAN, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(OZONO_ON, HIGH); //turn off LED
            digitalWrite(FAN, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("7:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 7; iSecond=0; //reset timer
        }
      }   else if (iPantalla == OZONO5MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 5;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("5:00",55, 185);
          digitalWrite(OZONO_ON, LOW); //turn on OZONO
          digitalWrite(FAN, HIGH); //turn on OZONO
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            //my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Fill_Round_Rectangle(30, 100, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            my_lcd.Set_Text_Size(4);
            my_lcd.Set_Text_colour(GREEN);
            my_lcd.Set_Text_Back_colour(BLACK);
            float rs_med = readMQ(MQ_PIN);      // Obtener la Rs promedio
            float concentration = getConcentration(rs_med/R0);   // Obtener la concentración
            sConcentration = concentration;
            sConcentration.concat(" ppm");
            my_lcd.Print_String(sConcentration,70, 100);
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(OZONO_ON, HIGH); //turn off LED
                  digitalWrite(FAN, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(OZONO_ON, HIGH); //turn off LED
            digitalWrite(FAN, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("5:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 5; iSecond=0; //reset timer
        }
      }   else if (iPantalla == OZONO3MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 3;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("3:00",55, 185);
          digitalWrite(OZONO_ON, LOW); //turn on OZONO
          digitalWrite(FAN, HIGH); //turn on OZONO
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            //my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Fill_Round_Rectangle(30, 100, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            my_lcd.Set_Text_Size(4);
            my_lcd.Set_Text_colour(GREEN);
            my_lcd.Set_Text_Back_colour(BLACK);
            float rs_med = readMQ(MQ_PIN);      // Obtener la Rs promedio
            float concentration = getConcentration(rs_med/R0);   // Obtener la concentración
            sConcentration = concentration;
            sConcentration.concat(" ppm");
            my_lcd.Print_String(sConcentration,70, 100);
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(OZONO_ON, HIGH); //turn off LED
                  digitalWrite(FAN, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(OZONO_ON, HIGH); //turn off LED
            digitalWrite(FAN, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("3:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 3; iSecond=0; //reset timer
        }
      }else if (iPantalla == OZONO4MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 4;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("4:00",55, 185);
          digitalWrite(OZONO_ON, LOW); //turn on OZONO
          digitalWrite(FAN, HIGH); //turn on OZONO
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            //my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Fill_Round_Rectangle(30, 100, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            my_lcd.Set_Text_Size(4);
            my_lcd.Set_Text_colour(GREEN);
            my_lcd.Set_Text_Back_colour(BLACK);
            float rs_med = readMQ(MQ_PIN);      // Obtener la Rs promedio
            float concentration = getConcentration(rs_med/R0);   // Obtener la concentración
            sConcentration = concentration;
            sConcentration.concat(" ppm");
            my_lcd.Print_String(sConcentration,70, 100);
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(OZONO_ON, HIGH); //turn off LED
                  digitalWrite(FAN, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(OZONO_ON, HIGH); //turn off LED
            digitalWrite(FAN, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("4:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 4; iSecond=0; //reset timer
        }
      } else if (iPantalla == OZONO2MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 2;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("2:00",55, 185);
          digitalWrite(OZONO_ON, LOW); //turn on OZONO
          digitalWrite(FAN, HIGH); //turn on OZONO
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            //my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Fill_Round_Rectangle(30, 100, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            my_lcd.Set_Text_Size(4);
            my_lcd.Set_Text_colour(GREEN);
            my_lcd.Set_Text_Back_colour(BLACK);
            float rs_med = readMQ(MQ_PIN);      // Obtener la Rs promedio
            float concentration = getConcentration(rs_med/R0);   // Obtener la concentración
            sConcentration = concentration;
            sConcentration.concat(" ppm");
            my_lcd.Print_String(sConcentration,70, 100);
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(OZONO_ON, HIGH); //turn off LED
                  digitalWrite(FAN, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Set_Text_Mode(1);
            my_lcd.Set_Text_Size(9);
            my_lcd.Set_Text_colour(WHITE);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(OZONO_ON, HIGH); //turn off LED
            digitalWrite(FAN, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("2:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 2; iSecond=0; //reset timer
        }
      } else if (iPantalla == UVC2MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 2;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("2:00",55, 185);
          digitalWrite(UV_C_LED, HIGH); //turn on LED
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(UV_C_LED, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(UV_C_LED, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("2:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 2; iSecond=0; //reset timer
        }
      } else if (iPantalla == UVC3MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 3;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("3:00",55, 185);
          digitalWrite(UV_C_LED, HIGH); //turn on LED
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(UV_C_LED, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(UV_C_LED, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("3:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 2; iSecond=0; //reset timer
        }
      } else if (iPantalla == UVC4MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 4;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("4:00",55, 185);
          digitalWrite(UV_C_LED, HIGH); //turn on LED
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(UV_C_LED, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(UV_C_LED, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("4:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 4; iSecond=0; //reset timer
        }
      } else if (iPantalla == UVC5MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 5;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("5:00",55, 185);
          digitalWrite(UV_C_LED, HIGH); //turn on LED
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(UV_C_LED, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(UV_C_LED, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("5:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 5; iSecond=0; //reset timer
        }
      } else if (iPantalla == UVC7MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 7;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("7:00",55, 185);
          digitalWrite(UV_C_LED, HIGH); //turn on LED
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(UV_C_LED, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(UV_C_LED, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("7:00",55, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 7; iSecond=0; //reset timer
        }
      } else if (iPantalla == UVC10MIN)
      {//chequeo coordenadas para saber si inicia cuenta atrás o cancela. Debería ser una función
        //my_lcd.Set_Draw_color(RED);
        //my_lcd.Draw_Line(50, 270, 275, 270); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 340, 275, 340); //primera linea horizontal    
        //my_lcd.Draw_Line(50, 405, 275, 405); //primera linea horizontal  
        iMin = 10;
        iSecond = 0;
        bStop = false;
        if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
        {
          //estando en la pantalla de iniciar o volver he apretado VOLVER, cargamos imagen menu again
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("VOLVER",75, 355);
          //LoadPicFromSDCard(MENU);
          if (bSDisOK)
          {
            LoadPicFromSDCard(MENU);
          } else
          {
            LoadMenuGrafico();
          }
          iPantalla = MENU;
        } else if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 270) && (p.y <= 340)))
        {
          //estando en la pantalla de iniciar o volver he apretado INICAR, iniciamos la cuenta, paramos al pasar dso minutos o al pulsar STOP, tb hay que activar los LED UV-c
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(BLUE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(WHITE);
          //my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Set_Text_Back_colour(BLACK);
          my_lcd.Set_Draw_color(BLACK);
          my_lcd.Fill_Round_Rectangle(70, 355, 290, 400, 5);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(RED);
          my_lcd.Print_String("STOP",110, 355);
          my_lcd.Set_Text_Mode(1);
          my_lcd.Set_Text_Size(9);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("10:00",30, 185);
          digitalWrite(UV_C_LED, HIGH); //turn on LED
          //while ((iMin != 0) || (iSecond != 0))
          while (((iMin != 0) || (iSecond != 0)) && (!bStop))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            CountDownStr(&iMin,&iSecond,&sTime);
            if (sTime == "10:00")
            {
              my_lcd.Print_String(sTime,30, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...  
            } else
            {
              my_lcd.Print_String(sTime,55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...  
            }
            
            unsigned long currentMillis = millis();            
            while (((millis() - currentMillis) <= INTERVALO) && (!bStop))
            {
                digitalWrite(13, HIGH); //ardunion uno es un 13
                TSPoint p = ts.getPoint();
                digitalWrite(13, LOW); //ardunion uno es un 13
                pinMode(XM, OUTPUT);
                pinMode(YP, OUTPUT);
                p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
                p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
                if (((p.x >= 50) && (p.x <= 275)) && ((p.y >= 340) && (p.y <= 405)))
                {
                  bStop = true;
                  iMin = 0;
                  iSecond = 0;
                  digitalWrite(UV_C_LED, LOW); //turn off LED
                }
            }
            //delay(975);// sustituir este delay for while y que la pulsar stop se pare el contador y se apague el led
          }
          if ((iMin == 0) && (iSecond == 0))
          {
            my_lcd.Fill_Round_Rectangle(30, 165, 290, 260, 5);
            my_lcd.Print_String("0:00",55, 185);  //la cadena str es la que va a ir cambiando!!!! habrá que hacer un strconcatena y bla, bla...            
            digitalWrite(UV_C_LED, LOW); //turn off LED
          }
          my_lcd.Fill_Round_Rectangle(30, 165, 290, 400, 5);
          my_lcd.Print_String("10:00",30, 185);
          my_lcd.Set_Text_Size(5);
          my_lcd.Set_Text_colour(WHITE);
          my_lcd.Print_String("INICIAR",55, 290);
          my_lcd.Set_Text_colour(GREY);
          my_lcd.Print_String("VOLVER",75, 355);
          iMin = 10; iSecond=0; //reset timer
        }
      }
    }
}
