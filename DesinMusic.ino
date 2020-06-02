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
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define PIXEL_NUMBER  (my_lcd.Get_Display_Width()/4)
#define FILE_NUMBER 3
#define FILE_NAME_SIZE_MAX 20
#define PORTADA 0
#define MENU 1
#define UVC2MIN 2

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define MINPRESSURE 10
#define MAXPRESSURE 1000
// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

uint32_t bmp_offset = 0;
uint16_t s_width = my_lcd.Get_Display_Width();  
uint16_t s_heigh = my_lcd.Get_Display_Height();
//int16_t PIXEL_NUMBER;

bool bPortadaOpen = false; //to open bmp only once
bool bMenuOpen = false; //to open bmp only once
int iCS = 10; //Chip Select SPI UNO
//int iCS = 53; //Chip Select SPI MEGA

//char file_name[16] = "portada.bmp";
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

bool LoadPicFromSDCard(int iPic)
{
  File bmp_file;
      
  bmp_file = SD.open(file_name[iPic]);
  if(!bmp_file)
      {
          my_lcd.Set_Text_Back_colour(WHITE);
          my_lcd.Set_Text_colour(BLUE);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("didnt find BMPimage!",0,10);
          return false;
      } else if (!analysis_bpm_header(bmp_file))
      {  
          my_lcd.Set_Text_Back_colour(WHITE);
          my_lcd.Set_Text_colour(BLUE);    
          my_lcd.Set_Text_Size(3);
          my_lcd.Print_String("bad bmp picture!",0,0);
          return false;
      } else
      {
        draw_bmp_picture(bmp_file);
        bmp_file.close(); 
        return true;  
      }  

}

void setup() 
{
   Serial.begin(9600);
   my_lcd.Init_LCD();
   Serial.println(my_lcd.Read_ID(), HEX);
   pinMode(13, OUTPUT);
  
   //Init SD_Card int 
   pinMode(iCS, OUTPUT);
   my_lcd.Fill_Screen(WHITE);
   if (!SD.begin(iCS)) 
   {
    my_lcd.Set_Text_Back_colour(WHITE);
    my_lcd.Set_Text_colour(BLUE);    
    my_lcd.Set_Text_Size(3);
    my_lcd.Print_String("SD Card Init fail!",0,0);
    return false;
  }
}

void loop() 
{
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    //Load Picture from SDCard
    if (bPortadaOpen == false)
    {
      bPortadaOpen = LoadPicFromSDCard(PORTADA);
    }


    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) 
    {
      if (bMenuOpen == false)
      {
        bMenuOpen = LoadPicFromSDCard(MENU);
        p.x = 0;  
        p.y = 0;
      }
      p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
      /* MOSTRAR ESTO EN EL PUERTO SERIE NOS INDICA EN QUÉ ZONA ESTÁN LOS PIXELES QUE SE ESTÁN TOCANDO EN LA PANTALLA*/
      Serial.print("p.x DESPUES del map: ");
      Serial.println(p.x); //105
      Serial.print("p.y DESPUES del map: ");
      Serial.println(p.y); //105
      /* MOSTRAR ESTO EN EL PUERTO SERIE NOS INDICA EN QUÉ ZONA ESTÁN LOS PIXELES QUE SE ESTÁN TOCANDO EN LA PANTALLA*/
      my_lcd.Set_Draw_color(WHITE);
      my_lcd.Draw_Line(60, 165, 260, 165); //primera linea horizontal
      my_lcd.Draw_Line(60, 200, 260, 200); //segunda linea horizontal
      my_lcd.Draw_Line(60, 235, 260, 235); //tercera linea horizontal
      
      my_lcd.Draw_Line(60, 330, 260, 330); //cuarta linea horizontal
      my_lcd.Draw_Line(60, 365, 260, 365); //quinta linea horizontal
      my_lcd.Draw_Line(60, 400, 260, 400); //sexta linea horizontal
      
      my_lcd.Draw_Line(60, 165, 60, 400); //primera linea vertical que es común a todas las areas
      
      my_lcd.Draw_Line(135, 165, 135, 200); //segunda linea vertical de la primera fila de uvc entre los numeros 2 y 3
      my_lcd.Draw_Line(195, 165, 195, 200); //tercera linea vertical de la primera fila de uvc entre los numeros 3 y 4
      my_lcd.Draw_Line(125, 200, 125, 235); //segunda linea vertical de la segunda fila de uvc entre los numeros 5 y 7
      my_lcd.Draw_Line(185, 200, 185, 235); //tercera linea vertical de la segunda fila de uvc entre los numeros 7 y 10

      my_lcd.Draw_Line(135, 330, 135, 365); //segunda linea vertical de la primera fila de uvc entre los numeros 2 y 3
      my_lcd.Draw_Line(195, 330, 195, 365); //tercera linea vertical de la primera fila de uvc entre los numeros 3 y 4
      my_lcd.Draw_Line(125, 365, 125, 400); //segunda linea vertical de la segunda fila de uvc entre los numeros 5 y 7
      my_lcd.Draw_Line(185, 365, 185, 400); //tercera linea vertical de la segunda fila de uvc entre los numeros 7 y 10
      
      my_lcd.Draw_Line(260, 165, 260, 400); //ultima linea vertical que es común a todas las areas
      //void Draw_Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
      if (((p.x >= 60) && (p.x <= 135)) && ((p.y >= 165) && (p.y <= 200)))
      {
        //estoy en dos minutos UVC
        Serial.println("estoy en DOS minutos UVC");
        delay(5000);
      } else if (((p.x >= 135) && (p.x <= 195)) && ((p.y >= 165) && (p.y <= 200)))
      {
        //estoy en tres minutos UVC
        Serial.println("estoy en TRES minutos UVC");
        delay(5000);
      } else if (((p.x >= 195) && (p.x <= 260)) && ((p.y >= 165) && (p.y <= 200)))
      {
        //estoy en cuatro minutos UVC
        Serial.println("estoy en CUATRO minutos UVC");
        delay(5000);
      } else if (((p.x >= 60) && (p.x <= 135)) && ((p.y >= 330) && (p.y <= 365)))
      {
        //estoy en dos minutos ozono
        Serial.println("estoy en DOS minutos OZONO");
        delay(5000);
      } else if (((p.x >= 135) && (p.x <= 195)) && ((p.y >= 330) && (p.y <= 365)))
      {
        //estoy en tres minutos OZONO
        Serial.println("estoy en TRES minutos OZONO");
        delay(5000);
      } else if (((p.x >= 195) && (p.x <= 260)) && ((p.y >= 330) && (p.y <= 365)))
      {
        //estoy en cuatro minutos OZONO
        Serial.println("estoy en CUATRO minutos OZONO");
        delay(5000);
      } else if (((p.x >= 60) && (p.x <= 125)) && ((p.y >= 200) && (p.y <= 235)))
      {
        //estoy en 5 minutos UVC
        Serial.println("estoy en CINCO minutos UVC");
        delay(5000);
      } else if (((p.x >= 125) && (p.x <= 185)) && ((p.y >= 200) && (p.y <= 235)))
      {
        //estoy en 7 minutos UVC
        Serial.println("estoy en SIETE minutos UVC");
        delay(5000);
      } else if (((p.x >= 185) && (p.x <= 260)) && ((p.y >= 200) && (p.y <= 235)))
      {
        //estoy en 10 minutos UVC
        Serial.println("estoy en DIEZ minutos UVC");
        delay(5000);
      } else if (((p.x >= 60) && (p.x <= 125)) && ((p.y >= 365) && (p.y <= 400)))
      {
        //estoy en 5 minutos OZONO
        Serial.println("estoy en CINCO minutos OZONO");
        delay(5000);
      } else if (((p.x >= 125) && (p.x <= 185)) && ((p.y >= 365) && (p.y <= 400)))
      {
        //estoy en 7 minutos OZONO
        Serial.println("estoy en SIETE minutos OZONO");
        delay(5000);
      } else if (((p.x >= 185) && (p.x <= 260)) && ((p.y >= 365) && (p.y <= 400)))
      {
        //estoy en 10 minutos OZONO
        Serial.println("estoy en DIEZ minutos OZONO");
        delay(5000);
      }           
    }
}
