#include <FAB_PIC_FM.h>

//#use rs232(baud=9600,uart1,parity=N,xmit=PIN_B6,rcv=PIN_B7,bits=8,stream=PC)

#use rs232(UART1,baud=9600,parity=N,bits=8,stream=MPIC,ERRORS)

#use spi(MASTER, CLK = PIN_C3, DI = PIN_C4, DO = PIN_C5,ENABLE = PIN_C2, BAUD = 9600, BITS = 16, STREAM = PORT2, MODE = 2 )

int8 fabData[39] = {0};                                                          //FAB data array
int8 adcsData[26] = {0};                                                         //Array for adcs data
unsigned int16 SRC_current = 0;
unsigned int32 bat_current = 0;
int8 heaterManual = 0;
#include <ADC_reading_func.c>

BYTE rx_chars[8] = {0};                                                          //array to get main PIC command

#int_rda                                                                         //UART1 Interrupt
void serial_isr(){
 
  rx_chars[0] = fgetc(MPIC);                                                     //Get main PIC command
} 


void main()
{  
   heaterManual = 0;                                                             //Heater Auto mode as default
   fabData[0] = 0x33;                                                            //FAB data header
  
   TRISD7 = 0; LATD7 = 1;                                                        //RD7=HIGH, enabling buffer for voltage measurements (PWR_BAT, SRC_V, RAW_V)
   
   TRISD0 = 0; LATD0 = 0;                                                        //RD0=LOW, disabling battery heater
   
   check = read_eeprom(check_memory);                                            //read EEPROM position 5 
   kill_flag = read_eeprom(memory);                                              //read EEPROM position 10
   
//!   kill_fab_forward(); //kill switch forward mode
//!   kill_OBC_forward();  

   if ((check!=8)&&(kill_flag!=5))
      {
        kill_fab_forward();                                                      //kill switch FAB forward mode
        kill_OBC_forward();                                                      //kill switch OBC forward mode
      }
   else
      {     
        kill_fab_reverse();                                                      //kill switch FAB reverse mode
        kill_OBC_reverse();                                                      //kill switch OBC reverse mode
      }
      
   
    SETUP_ADC(ADC_CLOCK_INTERNAL);                                               //To use internal clock
    SETUP_ADC_PORTS(ALL_ANALOG);                                                 //setting all analog ports (A0 A1 A2 A3 A5 E0 E1 E2 B2 B3 B1 B4 B0 B5 D1)
   
    enable_interrupts(INT_RDA);                                                  //UART interrupt enabled
    enable_interrupts(global); 

   while(TRUE)
   {
     
         battery_heater();                                                       //Heater flag control, Check battery temperature and battery voltage. Flag position = fabData[37]
         kill_status();                                                          //Check KILL_FAB_STAT and KILL_OBC_FAB_STAT. Flag position = fabData[38]
 /* Command for data collection*/   
          if(rx_chars[0] == 0x61)                                                 //if sensor data request interrupt received
          {             
           temperature();                                                        //reads temperatures data and saves it in the array fabDATA[]
           voltages();                                                           //reads voltages data and savse it in the array fabDATA[]
           adc_FAB();                                                            //reads the PIC ADC
           
           rx_chars[0]=0;
            
           delay_ms(10);
           battery_heater();                                                     //Heater flag control, Check battery temperature and battery voltage. Flag position = fabData[37]
           kill_status();                                                        //Check KILL_FAB_STAT and KILL_OBC_FAB_STAT. Flag position = fabData[38]
           for (int i=0;i<39;i++){                                               //Sends fabDATA to Main PIC
                  fputc(fabData[i],MPIC);                                          
                }  
            }
            
 /* Command for sending panels info for adcs*/   
          if(rx_chars[0] == 0x69)                                                //if panels data request interrupt received
          {             
           temperature();                                                        //reads temperatures data and saves it in the array fabDATA[]
           voltages();                                                           //reads voltages data and savse it in the array fabDATA[]
           adc_FAB();                                                            //reads the PIC ADC
           rx_chars[0]=0; 
           delay_ms(10);
           make_adcsData();                                                      //makes adcsData array
           for (int i=0;i<26;i++){                                               //Sends fabDATA to Main PIC
                  fputc(adcsData[i],MPIC);                                          
                }  
            }            
            
/*Command for Kill switch*/         
          if(rx_chars[0] == 0x17){                                              //if Kill switch reverse interrupt received
            
            rx_chars[0]=0;
            
            delay_ms(10);
            
            kill_fab_reverse();                                                  //kill switch FAB reverse mode
            kill_OBC_reverse();                                                  //kill switch OBC reverse mode
            
            write_eeprom(check_memory,8);
            check=8;
            write_eeprom(memory,5);
            kill_flag=5;
            
         }
         
           if(rx_chars[0] == 0x18){                                              //if Kill switch forward interrupt received
            
            rx_chars[0]=0;
            
            delay_ms(10);
            
            kill_fab_forward();                                                  //kill switch FAB forward mode
            kill_OBC_forward();                                                  //kill switch OBC forward mode
            
            write_eeprom(check_memory,1);
            check=1;
            write_eeprom(memory,1);
            kill_flag=1;
           }
/*Command for Battery Heater*/                                                   //Battery heater default mode is AUTO mode
           if(rx_chars[0] == 0x01)                                               //if Battery Heater Turn ON CMD received
           {                                              
            rx_chars[0]=0;
            delay_ms(10);
            RD0 = 1;                                                             //HEAT_CTL = HIGH
            fabData[37] = 1;                                                     //Flag in HIGH
            heaterManual = 1;                                                    //heater Manual Mode  
           }
           
           if(rx_chars[0] == 0x02)                                               //if Battery Heater Turn OFF CMD received
           {                                              
            rx_chars[0]=0;
            delay_ms(10);
            RD0 = 0;                                                             //HEAT_CTL = LOW
            fabData[37] = 0;                                                     //Heater Flag in LOW
            heaterManual = 1;                                                    //heater Manual Mode
           }
           
           if(rx_chars[0] == 0x03)                                               //if Battery Heater Auto Mode CMD received
           {                                              
            rx_chars[0]=0;
            delay_ms(10);
            heaterManual = 0;                                                    //heater Auto Mode
           }
   }

}


