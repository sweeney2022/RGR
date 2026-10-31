#include <Arduino.h>
#include <math.h>

#define PIN_OUT_DAC 26//пін для виводу модульованого сигналу

uint8_t out_dac_conv = 0;


const int min_frequency = 0;
const int max_frequency = 10000;
const int min_amplitude = 0;
const float max_amplitude = 1.5;
volatile int frequency = 0;
float amplitude = 1;
float dt = 0.0f;
uint64_t lastTime = 0;

volatile char signalMode = 'S';// S - синус, T = трикутний, R = Прямокутний, 
float duty_cycle = 50.0;//Коефіцієнт заповнення для прямокутного сигналу
float phase = 0.0;
float value = 0.0;



void mathSin(float *dt)
{
  phase+= (2.0*PI*frequency*(*dt));
  if(phase >= 2*PI){phase -= 2.0*PI;}
  value = sinf(phase);
}

void mathTriangular(float *dt)
{
  phase+= frequency*(*dt);
  if(phase>=1)
  {
    phase -= 1.0;
  }
  if(phase < 0.5)
  {
    value = 4.0*phase - 1.0;
  }
  else
  {
    value = 3.0-4.0*phase;
  }
}

void mathRectangular(float *dt)
{
  phase+= frequency*(*dt);

  if(phase >= 1.0)
  {
    phase -= 1.0;
  }
  if(phase < duty_cycle/100){value = 1;}
  else{value = -1;}
}

void dacGenerator(void *parameter)
{
  while(true)
  {
    uint64_t CurrentTime = esp_timer_get_time();
    dt = (CurrentTime-lastTime)/1000000.0f;
    lastTime = CurrentTime;
    
    if(signalMode == 'S')
    {
      mathSin(&dt);
    }
    else if(signalMode == 'T')
    {
      mathTriangular(&dt);
    }
    else if(signalMode == 'R')
    {
      mathRectangular(&dt);
    }
    out_dac_conv = 128+(value*85*amplitude);
    dacWrite(PIN_OUT_DAC, out_dac_conv);
  }
}

void PrintHelp()
{
  Serial.println("-----------------------");
  Serial.println("List of main command: ");
  Serial.println("MS - to choose sinusoidal signal");
  Serial.println("MT - to choose triangular signal");
  Serial.println("MR - to choose rectangular signal");
  Serial.println("F1000 - to set 1000 Hz ");
  Serial.println("D50 - to set duty cycle for rectangular signal");
  Serial.println("A1.5 - to set amplitude signal");
  Serial.print("Frequency range: ");
  Serial.print(min_frequency);
  Serial.print("...");
  Serial.print(max_frequency);
  Serial.println("Hz\nDuty cycle range: 0...100%");
}

void processCommand(String input)
{
  input.trim();
  if(input.length() == 0){return;}
  char command = toupper(input.charAt(0));
  //Вибір типу сигналу
  if(command == 'M')
  {
    if(input.length() < 2){Serial.println("MS - sin, MT - triangular, MR - rectangular");return;}
    char newMode = toupper(input.charAt(1));

    if(newMode != 'S' and newMode!= 'T' and newMode != 'R')
    {
      Serial.println("\n\nPlease enter mode in right form\nMS - Sinusoidal, MT - triangular, MR - rectangular");
      return;
    }
    else if(newMode == 'S'){Serial.println("\nYou did choice new mode of signal\nSignal = Sinusoidal");}
    else if(newMode == 'T'){Serial.println("\nYou did choice new mode of signal\nSignal = Triangular");}
    else if(newMode == 'R'){Serial.println("\nYou did choice new mode of signal\nSignal = Rectangular");}
    signalMode = newMode;
  }
  //вибір частоти сигналу
  else if(command == 'F')
  {
    int set_frequency = input.substring(1).toInt();
    if(set_frequency < min_frequency or set_frequency > max_frequency)
    {
      Serial.println("Please, enter wish frequency of signal in range from: ");
      Serial.print(min_frequency);
      Serial.print("...");
      Serial.print(max_frequency);
      Serial.println("");
      return;
    }
    frequency = set_frequency;
    Serial.print("\nYou did choice of frequency\nFrequency = ");
    Serial.print(frequency);
    Serial.println(" Hz");
  }
  else if(command == 'A')
  {
    float set_amplitude = input.substring(1).toFloat();
    if(set_amplitude<0 or amplitude>1.5)
    {
      Serial.println("Please, enter amplitude of signal in range from:");
      Serial.print(min_amplitude);
      Serial.print("...");
      Serial.print(max_amplitude);
      Serial.println("");
    }
    amplitude = set_amplitude;
    Serial.print("\nYou did choice of amplitude\nAmplitude = ");
    Serial.print(amplitude);
    Serial.println(" V");
  }
  else if(command == 'D')
  {
    int set_duty = input.substring(1).toInt();
    if(set_duty < 0 or set_duty > 100)
    {
      Serial.println("Please, enter wish duty of signal in range from 0 to 100 %");
      return;
    }
    duty_cycle = set_duty;
    Serial.print("\nYou did choice duty cycle of signal \nDuty = ");
    Serial.print(duty_cycle);
    Serial.println(" %");
  }
  else
  {
    Serial.println("You entered unknown command!!!");
    
    PrintHelp();
    return;
  }
  

}

void setup()
{
  Serial.begin(115200);
  delay(200);
  disableCore0WDT();//відключення Whatchdog-таймера на ядрі 0, щоб безперервний цикл генерації не перезавантажував esp32
  Serial.println("=======================");
  Serial.println("It's a little generator signal, which contain sin, triangular, rectangular");

  PrintHelp();

  xTaskCreatePinnedToCore(
    dacGenerator,   // Функція задачі
    "DAC_Generator",    // Ім'я задачі
    4096,               // Розмір стеку
    NULL,               // Параметри
    1,                  // Пріоритет
    NULL,               // Дескриптор
    0                   // Ядро 0
  );

}

void loop()
{
  if(Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    processCommand(input);
  }
}