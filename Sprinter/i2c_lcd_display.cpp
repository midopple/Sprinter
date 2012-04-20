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


void init_I2C_LCD(void)
{
  lcd.init();                      // initialize the lcd 
 
  // Print a message to the LCD.
  lcd.backlight();
  //lcd.print("Sprinter with LCD");
}

void manage_display(void)
{
  static unsigned long previous_millis_LCD = 0;
  static unsigned long start_millis_SD_card = 0;
  static unsigned char state_cnt_LCD = 0;
  int hotendtC_LCD,bedtempC_LCD,bed_tardet_temp;

  long help_pos_calc = 0;
  unsigned long help_time_calc = 0;
  int help_int_calc = 0;
  float help_float_calc = 0;
  
  if((millis() - previous_millis_LCD) > 250 )
  {
    previous_millis_LCD = millis();
    
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
        lcd.print("                    ");
        lcd.setCursor(0,0);
        lcd.print("H:");
        lcd.print(hotendtC_LCD);
        lcd.print("/");
        lcd.print(target_temp);
        lcd.print(" B:");    
        lcd.print(bedtempC_LCD);
        lcd.print("/");
        lcd.print(bed_tardet_temp);
        
        state_cnt_LCD=1;
      break;
      case 1:
        lcd.setCursor(0,1);
        lcd.print("                    ");
        lcd.setCursor(0,1);
        help_pos_calc = current_position[0];
        lcd.print("X:");
        lcd.print(help_pos_calc);
        lcd.print(" ");
        help_pos_calc = current_position[1];
        lcd.print("Y:");
        lcd.print(help_pos_calc);
        lcd.print(" ");
        lcd.print("Z:");
        lcd.print(current_position[2]);

        state_cnt_LCD=2;
      break;
      case 2:
        state_cnt_LCD=3;
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        help_pos_calc = current_position[3];        
        lcd.print("E:");
        lcd.print(help_pos_calc);
        lcd.print("mm ");
        
        if(sdmode == true)
        {
          lcd.print("SD:");
          if(filesize > 0)
            help_float_calc = ((float)sdpos*100) / (float)filesize;
          else  
            help_float_calc = 0;
            
          lcd.print(help_float_calc);
          lcd.print("%");  
        }
        else
        {
           start_millis_SD_card = millis();
           #ifdef PIDTEMP
             lcd.print(" PWM:");
             lcd.print((unsigned char)g_heater_pwm_val*1);
           #endif
        }

      break;
      case 3:
        state_cnt_LCD=0;
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        if(sdmode == true)
        {
          help_time_calc = (millis() - start_millis_SD_card)/1000;
          lcd.print(help_time_calc/60);
          lcd.print(":");  
          if((help_time_calc%60 < 10))
            lcd.print("0");  
          lcd.print(help_time_calc%60);
          lcd.print(" / ");  
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
          lcd.print(":");  
          if((help_time_calc%60 < 10))
            lcd.print("0");  
          lcd.print(help_time_calc%60);
          
          lcd.print(" / ");
        }
        else
        {
          
          lcd.print("FM/EM:");
          lcd.print(feedmultiply);
          lcd.print("/");
          lcd.print(extrudemultiply);
          
          lcd.print(" P:");
        }

         
        lcd.print(calc_plannerpuffer_fill());

      break;
      default:
        state_cnt_LCD=0;
      break;
      
    }
    
    
  }
  
  
  
}

