//Portas Analógicas     A0 A1 A2 A3 A4 A5
//Portas Digitais 12 13 14 15 16 17 18 19

// CW => Controla o sentido do motor (HIGH > Anti-horário | LOW > Horário
// EN => Controla o Driver TB6560 Ligando / Desligando (HIGH > Desliga | LOW > Liga)

// Define Os Pinos De CLK (MOTOR)
#define CLK1 11 // OCR1A
#define CLK2 10 // OCR1B
#define CLK3 9 // OCR2A

// Define Os Pinos De CW (DIREÇÃO)
#define CW1 8
#define CW2 7
#define CW3 6

// Define Os Pinos De EN (DRIVER)
#define EN1 5
#define EN2 4
#define EN3 3

// EM BREVE
// Define Os Pinos Dos Fins De Curso (FDCI = Fim De Curso Inicial | FDCF = Fim De Curso Final)
// #define FDCI1 13
// #define FDCF1 12
// #define FDCI2 14
// #define FDCF2 15
// #define FDCI3 16
// #define FDCF3 17
// EM BREVE

int16_t steps[3];
uint16_t timer_reset_v[3];

/*
  MOTOR [0] (COMP1A)
  Função ativada por comparação em A para Timer 1
*/

ISR(TIMER1_COMPA_vect)
{
  // Configura próximo valor de OCR1A baseado em timer_reset_v[0] calculado, vai dar overflow corretamente para o TCNT.
  OCR1A += timer_reset_v[0];
  if (steps[0] == 0)
  {
    digitalWrite(EN1, HIGH);
    digitalWrite(CLK1, LOW);
  }

  else if(steps[0] > 0)
  {
    steps[0]--;
  }
}

/*
  MOTOR [1] (COMP1B)
  Função ativada por comparação em B para Timer 1
*/

ISR(TIMER1_COMPB_vect)
{
  // Configura próximo valor de OCR1B baseado em timer_reset_v[1] calculado, vai dar overflow corretamente para o TCNT.
  OCR1B += timer_reset_v[1];
  if (steps[1] == 0)
  {
    digitalWrite(EN2, HIGH);
    digitalWrite(CLK2, LOW);
  }

  else if(steps[1] == 0)
  {
    steps[1]--;
  }
}

/*
  MOTOR [2] (COMP2A)
  Função ativada por comparação em A para Timer 2
*/

ISR(TIMER2_COMPA_vect)
{
  // Configura próximo valor de OCR2A baseado em timer_reset_v[1] calculado, vai dar overflow corretamente para o TCNT.
  OCR2A += timer_reset_v[2];
  if (steps[2] == 0)
  {
    digitalWrite(EN3, HIGH);
    digitalWrite(CLK3, LOW);
  }

  else if (steps[2] > 0)
  {
    digitalWrite(CLK3, HIGH);
    steps[2]--;
  }
}

void setup()
{
  // Taxa De Atualização Do Monitor Serial
  Serial.begin(9600);


  pinMode(CLK1, OUTPUT);
  pinMode(CLK2, OUTPUT);
  pinMode(CLK3, OUTPUT);

  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(EN3, OUTPUT);

  pinMode(CW1, OUTPUT);
  pinMode(CW2, OUTPUT);
  pinMode(CW3, OUTPUT);

  digitalWrite(EN1, HIGH);
  digitalWrite(EN2, HIGH);
  digitalWrite(EN3, HIGH);

  /*
    REGISTRADORES
    TCCR1A / TCCR2A = 0b01010000 -> Ativa o Modo De Comparação A e B {Bits 6 & 4}
    TCCR1B / TCCR2B = 0b00000010 -> Ativa o Clock Com Prescale Configurado Para 8 {Bit 1}
    TISMK1 / TIMSK2 = 0b00000000 -> Desabilita As Interrupções
  */

  TCCR1A = 0b01010000;
  TCCR1B = 0b00000010;
  TIMSK1 = 0b00000000;

  TCCR2A = 0b01000000;
  TCCR2B = 0b00000111;
  TIMSK2 = 0b00000000;
}

void loop() 
{
  short motor_n, velocidade;
  int16_t distance;
  int frequency;
  bool direction;

  // Solicita Ao Usuário Qual Motor Ele Deseja Controlar
  // Decremento A Variável Pois Usarei O Mesmo Para Inserir Dados Em timer_reset_v[motor_n] / steps[motor_n]
  do
  {
    Serial.println("Qual motor Você Deseja Controlar?\n[1] - Motor Esquerdo\n[2] - Motor Central\n[3] - Motor Direito");
    while (!Serial.available()){}
    motor_n = Serial.readStringUntil('\n').toInt();
  } while(motor_n < 1 || motor_n > 3);
  motor_n--;

  // Solicita Ao Usuário Qual Velocidade Ele Irá Usar
  // Utilizo A Velocidade Inserida Para Calcular A Frequência
  do
  {
    Serial.println("Qual Velocidade Deseja Usar? (0% - 100%)");
    while(!Serial.available()){}
    velocidade = Serial.readStringUntil('\n').toInt();
  } while(velocidade < 1 || velocidade > 100);
  frequency = 8 * velocidade + 200;

  // Solicita Ao Usuário Qual A Distância Que O "Carro" Irá Pecorrer
  do
  {
    Serial.println("Insira a Distância de Movimento (mm)");
    while(!Serial.available()){}
    distance = Serial.readStringUntil('\n').toInt();
  }while(distance != -1 && (distance < 0 || distance > 3000));

  // Solicita Ao Usuário A Direção Do "Carro"
  do 
  {
    Serial.println("Insira a Direção: [0] - Horário\n[1] - Anti-Horário");
    while (!Serial.available()){}
    direct = Serial.readStringUntil('\n').toInt();
  }while (direct < 0 || direct > 1);

  /*
    Configurações
    De Acordo Com [motor_n] entrará em uma das condições a seguir para configurar as seguintes coisas:

    steps[motor_n] = (distance / 1.7) * 400; -> Calcula a distância de movimento baseado em fuso com 8
    timer_reset_v[motor_n] = (1000000 / frequency) - 1; -> // Calcula contador para frequência baseado no datasheet do ATMEGA328P
    digitalWrite(EN1, LOW); -> Liga O Driver TB6560 Do Motor[motor_n]
    digitalWrite(CW1, direction); -> Define A Direção Do Motor[motor_n] de acordo com a variável "direction"
    TIMSK1 / TIMSK2 = 0b00000010; -> Habilita A Interrupção Do Modo De Comparação A
    TIMSK1 = 0b00000100; -> Habilita A Interrupção Do Modo De Comparação B
  */
  
  if (motor_n == 0) 
  {
    digitalWrite(CW1, direct);
    steps[0] = (distance / 8) * 400;
    timer_reset_v[0] = (1000000 / frequency) - 1;
    digitalWrite(EN1, LOW);
    TIMSK1 = 0b00000010;
  } 
  
  else if (motor_n == 1) 
  {
    steps[1] = (distance / 8) * 400;
    timer_reset_v[0] = (1000000 / frequency) - 1;
    digitalWrite(EN2, LOW);
    digitalWrite(CW2, direction);
    TIMSK1 = 0b00000100;
  } 
  
  else 
  {
    steps[2] = (distance / 8) * 400;
    timer_reset_v[0] = (1000000 / frequency) - 1;
    digitalWrite(EN3, LOW);
    digitalWrite(CW3, direction);
    TIMSK2 = 0b00000010;
  }
}
