/*
 Reprap firmware based on Sprinter
 I2C LCD Function 
 by Doppler Michael
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 
 I2C Libary for Display
 www: http://hmario.home.xs4all.nl/arduino/LiquidCrystal_I2C/
 
 */



#include <avr/pgmspace.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "fastio.h"
#include "Sprinter.h"
#include "i2c_lcd_display.h"
#include "heater.h"


void show_main_page(void);

unsigned char menu_level_ID = 0;
volatile char buttons=0;  //the last checked buttons in a bit array.
int encoderpos=0;
short lastenc=0;

extern int calc_plannerpuffer_fill(void);

extern float current_position[NUM_AXIS];
extern uint32_t filesize;
extern uint32_t sdpos;
extern bool sdmode;
extern volatile int feedmultiply;
extern volatile int extrudemultiply;
#ifdef PIDTEMP
  extern volatile unsigned char g_heater_pwm_val;
#endif


LiquidCrystal_I2C lcd(0x20,20,4);  // set the LCD address to 0x20 for a 20 chars and 4 line display

void lcdProgMemprint(const char *str)
{
  char ch=pgm_read_byte(str);
  while(ch)
  {
    lcd.print(ch);
    ch=pgm_read_byte(++str);
  }
}

#define lcdprintPGM(x) lcdProgMemprint(PSTR(x))

//---------------------------------------------------------------------------
//----------------------------- CHECK ENCODER    ----------------------------
//---------------------------------------------------------------------------
#define EN_C (1<<2)
#define EN_B (1<<1)
#define EN_A (1<<0)

void check_encoder_rotation(void)
{
  char enc=0;
  
  if(buttons&EN_A)
    enc|=(1<<0);
  if(buttons&EN_B)
    enc|=(1<<1);
    
  if(enc!=lastenc)
  {
    switch(enc)
    {
    case 0:
      if(lastenc==3)
        encoderpos++;
      else if(lastenc==1)
        encoderpos--;
    break;
    case 1:
      if(lastenc==0)
        encoderpos++;
      else if(lastenc==2)
        encoderpos--;
    break;
    case 2:
      if(lastenc==1)
        encoderpos++;
      else if(lastenc==3)
        encoderpos--;
    break;
    case 3:
      if(lastenc==2)
        encoderpos++;
      else if(lastenc==0)
        encoderpos--;
    break;
    default:
    break;
    }
  }
  lastenc=enc;
}
//----------------------------- END CHECK ENCODER    ------------------------
//---------------------------------------------------------------------------



void init_I2C_LCD(void)
{
  lcd.init();                      // initialize the lcd 
 
  //Backlight ON
  lcd.backlight();
}



void manage_display(void)
{
  static unsigned long previous_millis_LCD = 0;
  static unsigned long previous_millis_button = 0;
  
  if((millis() - previous_millis_button) > 100 )
  {
    previous_millis_button = millis();
  
    check_encoder_rotation();
    
  }  
  
  if((millis() - previous_millis_LCD) > 250 )
  {
    previous_millis_LCD = millis();
    
    switch(menu_level_ID)
    {
      case 0:
        show_main_page();
      break;
      case 1:
          
      break;
      default:
        menu_level_ID = 0;
      break;  
    }
  }
  
  
  
}


void show_main_page(void)
{
    
  static unsigned long start_millis_SD_card = 0;
  static unsigned char state_cnt_LCD = 0;
  int hotendtC_LCD,bedtempC_LCD,bed_tardet_temp;

  long help_pos_calc = 0;
  unsigned long help_time_calc = 0;
  int help_int_calc = 0;
  float help_float_calc = 0;
  
    switch(state_cnt_LCD)
    {
    
      case 0:
        #if (TEMP_0_PIN > -1) || defined (HEATER_USES_MAX6675)|| defined HEATER_USES_AD595
          hotendtC_LCD = analog2temp(current_raw);
        #endif
        #if TEMP_1_PIN > -1 || defined BED_USES_AD595
          bedtempC_LCD = analog2tempBed(current_bed_raw);
          bed_tardet_temp = analog2tempBed(target_bed_raw);
        #endif

        lcd.setCursor(0,0);
        lcdprintPGM("                    ");
        lcd.setCursor(0,0);
        lcdprintPGM("H:");
        lcd.print(hotendtC_LCD);
        lcdprintPGM("/");
        lcd.print(target_temp);
        lcdprintPGM(" B:");    
        lcd.print(bedtempC_LCD);
        lcdprintPGM("/");
        lcd.print(bed_tardet_temp);
        
        state_cnt_LCD=1;
      break;
      case 1:
        lcd.setCursor(0,1);
        lcdprintPGM("                    ");
        lcd.setCursor(0,1);
        help_pos_calc = current_position[0];
        lcdprintPGM("X:");
        lcd.print(help_pos_calc);
        help_pos_calc = current_position[1];
        lcdprintPGM(" Y:");
        lcd.print(help_pos_calc);
        lcdprintPGM(" Z:");
        lcd.print(current_position[2]);

        state_cnt_LCD=2;
      break;
      case 2:
        state_cnt_LCD=3;
        lcd.setCursor(0,2);
        lcdprintPGM("                    ");
        lcd.setCursor(0,2);
        help_pos_calc = current_position[3];        
        lcdprintPGM("E:");
        lcd.print(help_pos_calc);
        lcdprintPGM("mm ");
        
        if(sdmode == true)
        {
          lcdprintPGM("SD:");
          if(filesize > 0)
            help_float_calc = ((float)sdpos*100) / (float)filesize;
          else  
            help_float_calc = 0;
            
          lcd.print(help_float_calc);
          lcdprintPGM("%");  
        }
        else
        {
           start_millis_SD_card = millis();
           #ifdef PIDTEMP
             lcdprintPGM(" PWM:");
             lcd.print((unsigned char)g_heater_pwm_val*1);
           #endif
        }

      break;
      case 3:
        state_cnt_LCD=0;
        lcd.setCursor(0,3);
        lcdprintPGM("                    ");
        lcd.setCursor(0,3);
        if(sdmode == true)
        {
          help_time_calc = (millis() - start_millis_SD_card)/1000;
          lcd.print(help_time_calc/60);
          lcdprintPGM(":");  
          if((help_time_calc%60 < 10))
            lcdprintPGM("0");  
          lcd.print(help_time_calc%60);
          lcdprintPGM(" / ");  
          if(filesize > 0)
          {
            help_float_calc = (float)sdpos / (float)filesize;
            if(help_float_calc > 0.01)
            {
              help_time_calc = (unsigned long)((float)help_time_calc / help_float_calc)-help_time_calc;
            }
            else  
              help_time_calc = 0;
          }
          lcd.print(help_time_calc/60);
          lcdprintPGM(":");  
          if((help_time_calc%60 < 10))
            lcdprintPGM("0");  
          lcd.print(help_time_calc%60);
          
          lcdprintPGM(" / ");
        }
        else
        {
          
          lcdprintPGM("FM/EM:");
          lcd.print(feedmultiply);
          lcdprintPGM("/");
          lcd.print(extrudemultiply);
          
          lcdprintPGM(" P:");
        }

         
        lcd.print(calc_plannerpuffer_fill());

      break;
      default:
        state_cnt_LCD=0;
      break;
      
    } 
 
  
}
