#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "UbidotsESPMQTT.h"
#include <ESP8266WiFi.h>


#define u8 unsigned char
#define On LOW
#define Off HIGH

#define b_on_off D5
#define b_up D6
#define b_down D7
#define power D2
#define testp D8


//#define TOKEN "BBFF-eg2hA0fhc4wDWzqHMDtyNnbtYe2O6m"//F
//#define TOKEN "BBFF-VZg1gPgNY5U3v8dryETYu9ig70Dk2B"    //C Your Ubidots TOKEN
#define TOKEN "BBFF-MPR5uxfVSfRPzO75NgUayUW7O5x4zl"
#define WIFINAME "Marwa"  // Your SSID
#define WIFIPASS "00000000"  // Your Wifi Pass

Ubidots client(TOKEN);
String data;
int onoroff =0;



#define MAX_TASKS    4
u8 TaskCount = 0;

const byte cooler = 2;
const byte heater = 16;

void ICACHE_RAM_ATTR ISR(void);

typedef enum states {off, initial, temp_setting, check_state, heat, cool, sat} states;
typedef enum buttons{ On_OFF_Button, Up_Button, Down_Button, none} buttons;

states Current_State = off;

buttons Button = none;

int init_set_temp = 0;
int sensor_temp = 0;
int last_init_set_temp = 0;
int set_temp_create = false;
int counter =0;
String rec_data;

void Wifi_Init(void)
{
  
  client.setDebug(true);  // Pass a true or false bool value to activate debug messages
  client.wifiConnection(WIFINAME, WIFIPASS);
  client.begin(callback);
  client.ubidotsSubscribe("demo", "on_off");  // Insert the dataSource and Variable's Labels
  client.ubidotsSubscribe("demo", "num"); 
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message arrived ");
   data ="";
   rec_data = "";
   rec_data =topic;
  for(int i =0; i<length-2; i++)
  {
    data += (char)payload[i];
  } 
   Serial.println(data);
   if (rec_data.indexOf("on_off")>= 0)
   { int x=data.toInt();
     if (Current_State ==off && x==1)
      {
        onoroff = x;
        
      }
      else if (Current_State !=off && x==0)
      {
        onoroff = x;
        
      }
   }
   
   else if (rec_data.indexOf("num")>= 0 && Current_State != off && Current_State != temp_setting)
   {
    Serial.print("temp");
    init_set_temp = data.toInt();
   }
  
}
void heater_off(void)
{
              digitalWrite(heater,Off); 
              digitalWrite(cooler,Off); 
              digitalWrite(power,LOW);
              client.add("on_off", 0);
              client.add("num", 0);
              client.add("initial-state", 0);
              client.add("sat-state", 0);
              
              client.ubidotsPublish("demo");

              client.add("cool-state", 0);
              client.add("heat-state", 0);
              client.add("set-temp-state", 0);
        
              client.ubidotsPublish("demo");
              onoroff=0;  
}

typedef struct {
  void (*Ptr2Fun)(void)   ;
  int Period      ;
  int dly       ;
  int runme        ;
}stask ;
static stask Tasks[MAX_TASKS];

void Temp_Mode_Check(void);

void Sched_vInit(void){
  
  for (u8 count = 0 ; count < MAX_TASKS ; count ++ ){

    Tasks[count].Ptr2Fun  = NULL    ;

    Tasks[count].dly    = 0   ;

    Tasks[count].Period = 0   ;

    Tasks[count].runme       = 0   ;

  }
  
  timer1_attachInterrupt(ISR);
  timer1_enable(TIM_DIV16, TIM_EDGE,TIM_SINGLE);
  timer1_write(125000);
  
}


void Sched_vCreatTask(void (*Ptr2Task)(void) , int Periodicity ,
            int dly)

{
  if ( MAX_TASKS >TaskCount )
  {

  Tasks[TaskCount].Ptr2Fun      = Ptr2Task                ;

  Tasks[TaskCount].Period   = Periodicity    ;

  Tasks[TaskCount].dly     = dly       ;

  Tasks[TaskCount].runme       = 0       ;

  TaskCount ++ ;
  }
  else 
  {
    //tasks list is full
  }
}

void Sched_Disp(void)
{
for (u8 i =0 ; i<MAX_TASKS; i++)
    {
        if(Tasks[i].runme > 0)
        {
         Tasks[i].Ptr2Fun() ;  
         Tasks[i].runme -= 1 ;
         if (Tasks[i].Period ==0)
         {
          Delete_Task(i);
         }
        }
    }
}    
void Delete_Task(u8 index)
{
  Tasks[index].Ptr2Fun=NULL ;
  Tasks[index].runme = 0 ;
  TaskCount--;         
}

void ICACHE_RAM_ATTR ISR(void)
{   //update
        
    for (u8 i =0 ; i<MAX_TASKS; i++)
    {
        if(Tasks[i].Ptr2Fun)
        {
          if(Tasks[i].dly==0)
          {
            Tasks[i].runme+=1;
            if(Tasks[i].Period)
            {
             Tasks[i].dly = Tasks[i].Period -1; 
            }
          }
          else
          {
              Tasks[i].dly--;  
          }
        }
  
    }
    timer1_write(125000);
  
}

void Heater_Control_Update (void)
{
  Serial.print(Current_State);
  if (Current_State == off)
     {   if (Button == On_OFF_Button)
        {
            Current_State = initial;
            init_set_temp = 60;
            digitalWrite(power,HIGH); 
            client.add("on_off", 1);
            client.add("num", init_set_temp);
            client.ubidotsPublish("demo");
            onoroff=1;
            
        }
        else{
          
            } 
       
        
     }
     else if (Current_State == initial)
     { 
      if (Button == On_OFF_Button)
          {  
            Current_State = off;
            init_set_temp = 0;
            heater_off();
        
          }
         else if (Button == Up_Button)
         {
            Current_State = temp_setting;
         }
         else if (Button ==Down_Button)
         { 
            Current_State = temp_setting;      
         }
         else {}
         
     }
     else if (Current_State == temp_setting)
     {
        if (Button == On_OFF_Button)
        {
              Current_State = off;
              init_set_temp = 0;
              heater_off();

         
        }
        else if (Button == Up_Button)
        {
          if (init_set_temp <= 70)
              {
                init_set_temp += 5;
                Serial.println();
                Serial.println(init_set_temp); 
              }
              else
              {}
        }
        else if (Button == Down_Button)
        {
              if (init_set_temp >= 20) //40
              {
                init_set_temp -= 5;
                Serial.println();
                Serial.println(init_set_temp); 
              }
              else {}
        }
        else {if (init_set_temp != last_init_set_temp)
              {
              client.add("num", init_set_temp);
              client.ubidotsPublish("demo");
              last_init_set_temp = init_set_temp;
              }}

              
     }
     else if (Current_State == check_state)
     {
        if(Button == On_OFF_Button)
        {   
              Current_State = off;
              init_set_temp = 0;
              heater_off();
              
        }
        else if (Button == Up_Button)
        {
            Current_State = temp_setting;
        }
        else if (Button == Down_Button)
        {
            Current_State = temp_setting;
        }
                  
        else{
              if (init_set_temp -5 > sensor_temp)  
              {
                Current_State = heat;
                
              }
              else if (init_set_temp +5 < sensor_temp)
              {
                Current_State = cool;
                
              }
              else 
              {
                Current_State = sat;
              }
              
        }
        }
        else if (Current_State == heat)
     {
        if(Button == On_OFF_Button)
        {
              Current_State = off;
              init_set_temp = 0;
              heater_off();

        }
        else if (Button == Up_Button ||Button == Down_Button)
        {
            Current_State = temp_setting;
        }
        else
        {
          digitalWrite(heater,On); 
          digitalWrite(cooler,Off); 
          Current_State = check_state;
        }
     }
     else if (Current_State == cool)
     {
        if(Button == On_OFF_Button)
        {
              Current_State = off;
              init_set_temp = 0;
              heater_off();
              
        }
        else if (Button == Up_Button)
        {
            Current_State = temp_setting;
        }
        else if (Button == Down_Button)
        {
            Current_State = temp_setting;
        }
        else
        {
          digitalWrite(heater,Off); 
          digitalWrite(cooler,On);
          Current_State = check_state; 
        }
     }
     else if (Current_State == sat)
     {
        if(Button == On_OFF_Button)
        {
              Current_State = off;
              init_set_temp = 0;
              heater_off();
  
        }
        else if (Button == Up_Button)
        {
            Current_State = temp_setting;
        }
        else if (Button == Down_Button)
        {
            Current_State = temp_setting;
        }
        else
        {
          digitalWrite(heater,Off); 
          digitalWrite(cooler,Off); 
          Current_State = check_state;
        }
     }
    else{}
Button = none;
}

void Button_Update(void)
{
  static u8 current_onof = 0;
  static u8 current_up = 0;
  static u8 current_down = 0;
  counter++;
  
  if(!digitalRead(b_down)||current_down)//         if pressed
  {
    current_down = 1;
    if(digitalRead(b_down))
    {
      Button = Down_Button;
      current_down = 0;
    } 
      counter =0; 
  }
  else
  {
    //Button = none;  
  }
  if(! digitalRead(b_up) || current_up)
  {
     
    current_up = 1;
    if(digitalRead(b_up))
    {
      Button = Up_Button;
      current_up = 0;
    } 
      counter =0; 
  }
  else
  {
    //Button = none;  
  }
  if(!digitalRead(b_on_off)|| current_onof)//         if pressed
  {
    current_onof = 1;
    if(digitalRead(b_on_off))
    {
      Button = On_OFF_Button;
      current_onof = 0;
    } 
      counter =0; 
  }
  else
  {
    //Button = none;  
  }
 
  if (Current_State == temp_setting || Current_State == initial)   //   5sec passed
  { 
    if(Button == none )  //without any pressed buttons
    { 
      if(counter == 200)//5seconds
       {
        Current_State = check_state; 
        counter = 0;
       }
     
    }
  }else{counter = 0; }

}

void Dashboard_Update(void)
{
  static states Last_State = off;
  if (!client.connected()) 
  {
    client.reconnect();
    client.ubidotsSubscribe("demo", "on_off");  // Insert the dataSource and Variable's Labels
    client.ubidotsSubscribe("demo", "num");  
  }
  client.loop();
  if(Current_State == off && onoroff == 1 && Button!= On_OFF_Button )
  {
    Button = On_OFF_Button;
    counter =0;  
    Last_State = Current_State;
  }
  else if(Current_State != off && onoroff == 0 &&Current_State != Last_State )
  {
    Button = On_OFF_Button;
    counter =0;  
    Last_State = Current_State;
  }
  else{}
  static int last_sensor_read = 0;
  if (sensor_temp != last_sensor_read)
  { 
    last_sensor_read = sensor_temp;
    client.add("sensor", last_sensor_read);
    client.ubidotsPublish("demo");
    
  }

  
  if(Current_State != Last_State && Current_State != check_state )
  {
     if (Current_State == initial)
    {
        client.add("initial-state", 1);
        client.add("sat-state", 0);
        client.add("cool-state", 0);
        client.add("heat-state", 0);
        client.add("set-temp-state", 0);
        client.ubidotsPublish("demo");
    }
    else if (Current_State == temp_setting)
    {
        client.add("initial-state", 0);
        client.add("sat-state", 0);
        client.add("cool-state", 0);
        client.add("heat-state", 0);
        client.add("set-temp-state", 2);
        client.ubidotsPublish("demo");
    }
    else if (Current_State == heat)
    {
        client.add("initial-state", 0);
        client.add("sat-state", 0);
        client.add("cool-state", 0);
        client.add("heat-state", 4);
        client.add("set-temp-state", 0);
        client.ubidotsPublish("demo");
    }
    else if (Current_State == cool)
    {
        client.add("initial-state", 0);
        client.add("sat-state", 0);
        client.add("cool-state", 5);
        client.add("heat-state", 0);
        client.add("set-temp-state", 0);
        client.ubidotsPublish("demo");
    }
    else if (Current_State == sat)
    {
        client.add("initial-state", 0);
        client.add("sat-state", 6);
        client.add("cool-state", 0);
        client.add("heat-state", 0);
        client.add("set-temp-state", 0);
        client.ubidotsPublish("demo");
    }
   
  Last_State = Current_State;
  }  
    
}

void Sensor_Update (void)
{
if (Current_State == check_state)
  {
    sensor_temp = ((analogRead(A0)*3.3*100)/1024);
    Serial.println(); Serial.print("read temp: "); Serial.println(sensor_temp);
  
  }
  else {}
}
void setup() {
  Serial.begin(115200);
  pinMode(cooler, OUTPUT);
  pinMode(heater, OUTPUT);
  pinMode(power, OUTPUT);
  
  pinMode(b_on_off, INPUT_PULLUP);
  pinMode(b_up, INPUT_PULLUP);
  pinMode(b_down, INPUT_PULLUP);
  
  digitalWrite(cooler,Off); 
  digitalWrite(heater,Off);
  digitalWrite(power,LOW); 
  
  Sched_vInit();
  //tick interval = 50ms
  Sched_vCreatTask( Heater_Control_Update , 4 , 0) ;
  Sched_vCreatTask( Button_Update , 1 , 0   ) ;
  Sched_vCreatTask( Dashboard_Update , 4 , 0   ) ;
  Sched_vCreatTask( Sensor_Update , 4 , 0   ) ;
  
  Wifi_Init();

  
  
}


void loop() 
{
 Sched_Disp();
}
