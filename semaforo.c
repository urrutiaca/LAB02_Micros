#include <avr/io.h>
#include <avr/interrupt.h>


// De acuerdo al enunciado se tienen 4 STATES para los semaforos, entonces:

#define LDPV    0 // luz verde semaforo vehiculos - paso de vehiculos
#define LDPP    1 // luz verde semaforo peatonal - paso de peatones
#define LDVD    2 // luz roja semaforo vehiculos - paso detenido vehiculos
#define LDPD    3 // luz roja semafoto peatonal 
// Más estados para el parpadeo
#define PP_V    4 // para parpadeo vehiculo
#define PP_P    5 // para parpadeo personas 

// --------------------- Maquina de STATE para el control de semaforos --------
int STATE;
// Manejo de botones 
int push_boton1 = 0;
int push_boton2 = 0;
//contador general
int contador_tiempo = 0;
int contador_parpadeo = 0;
int contador_parpadeo_p = 0;


// Para el parpadeo. Del diagrama del enunciado se nota que siempre son 3 ciclos de parpadeo 
// ocurre un cambio de alto a bajo o viceversa cada medio segundo
// cada segundo se puede decir que ocurre un parpadeo, por lo tanto:
 void parpadeo(){
   if (contador_tiempo == 30){
   switch (STATE)
   {
   case (PP_V): // para vehiculos
       PORTB ^= (1<<PB3); 
   break;
   case (PP_P): // para peatones
       PORTB ^= (1<<PB0);
   break;
   default:
     break;
     }} 
  
     if (contador_tiempo == 60){
       contador_tiempo = 0;
        switch (STATE)
        {
          case (PP_V): // para vehiculos
            PORTB ^= (1<<PB3); 
          break;
          case (PP_P): // para peatones
            PORTB ^= (1<<PB0);
          break;
          default:
            break;
        }
       ++ contador_parpadeo_p; //permiten contar las veces que se ha parpadeado 
       ++ contador_parpadeo;
    } // así, la máquina sabe cuando cambiar de estado puesto que se cumplen las condiciones.

        else  contador_tiempo ++; // aumentar el "tiempo"
}

void reset_contadores(){
      contador_parpadeo = 0;
      contador_parpadeo_p = 0;
      contador_tiempo = 0;
}

// Timers AND Interrupciones ----------- De acuerdo a lo estudiado en las siguientes fuentes y lo estudiado en clase
// se realiza el manejo de las interrupciones y los timers
//https://www.gadgetronicx.com/attiny85-timer-tutorial-generating-time-delay-interrupts/
//https://www.gadgetronicx.com/attiny85-external-pin-change-interrupt/

ISR (TIMER0_OVF_vect)// Vector de interrupcion en el timer 0
{

 parpadeo();

} 

void timer_interrupt (){
  //Timer control registers as normal   
  TCCR0A = 0x00;
  TCCR0B = 0x00;
  TCCR0B |= (1 << CS00) | (1 << CS02); //prescaling 
  sei(); // para las interrupciones
  TCNT0 = 0;
  TIMSK |= (1 << TOIE0);
}

ISR(INT0_vect){ // PARA PD2 - BOTON1
  push_boton1=1;
}

ISR(INT1_vect){ // PARA PD3 - BOTON2
  push_boton2=1;
}

void interruptor_externo()  

{ DDRB |= (1 << PB3)|(1 << PB2)|(1 << PB1) |(1 << PB0); //Manejo de pines
  GIMSK |= (1<<INT0)|(1<<INT1);     // habilitando la interrupcion(interrupción externa)
  MCUCR |= (1<<ISC01)|(1<<ISC10)|(1<<ISC11); // Configurando como flanco 
}

// ------ Semaforo vehiculos ------- mediante maquina de estados
// EL funcionamiento es bastante simple. Si se cumple el tiempo o los parpadeos establecidos se pasa de estado, caso contrario
// se mantiene en ese estado.
// -----------------------------------------------------------------------
void semaforo(){

switch (STATE)
{
  // Paso de vehiculos
  case (LDPV):
    PORTB = (1<<PB3) |(0<<PB2) |(1<<PB1)|(0<<PB0); // Activan pines de interes
    if (push_boton1 == 1 || push_boton2 == 1){ // al Presionar cualquier boton, empieza conteo para parpadeo
      if(contador_parpadeo>=10){  // > por si se mantiene el boton presionado 10+s
        // se debe iniciar parpadeo entonces Cambio de estado:
        STATE = PP_V;
        reset_contadores();  
      }
    } else{
        STATE=LDPV; //de acuerdo al enunciado, si no, se mantiene el estado.
      }
  break;
  
  // Parpadeo vehiculos
  case(PP_V):
    if (contador_parpadeo == 3){ // al pasar 3 parpadeos
      STATE = LDVD; //pasar a rojo - vehiculos detenidos
      reset_contadores();
    }
    else{
      STATE=PP_V;
    }
  break;

   // Vehiculos detenidos 
  case(LDVD):
    PORTB = (0<<PB3) |(0<<PB2) |(1<<PB1)|(0<<PB0); 
    if (contador_parpadeo == 1){
      STATE=LDPP; //Pasa a peatonal en verde
      reset_contadores();
    } else {
      STATE=LDVD;
    }
  break;

    //paso de peatones
  case(LDPP):
    PORTB = (0<<PB3) |(1<<PB2) |(0<<PB1)|(1<<PB0); 
    if (contador_parpadeo_p == 10){
      STATE = PP_P; // a los 10 s del cambio empezar a parpadear
      reset_contadores();
    }else{
      STATE = LDPP;
    }
    break;


  // Parpadeo peatones
  case(PP_P):
    if (contador_parpadeo_p == 3){
      STATE = LDPD; //pasar a rojo - peatones detenidos
      reset_contadores();
    }
    else{
    STATE = PP_P;
    }
  break;

  //Final secuencia 
  // Peatones detenidos
  case(LDPD):
    PORTB = (0<<PB3) |(1<<PB2) |(1<<PB1)|(0<<PB0);
    if(contador_parpadeo_p == 1){
      STATE=LDPV; //pasar a vehiculos verde
      reset_contadores();
      push_boton1 = 0; // reset de botones
      push_boton2 = 0;
    } else{
      STATE = LDPD;
    }
    break;
  
 
  
  default:
  break;
}
}


// -------------------- MAIN ---------------------------------------
int main(void)
{
  // inicializa en estado - semaforo vehicular en verde
  STATE = LDPV;
  // Inicialzan pines
  PORTB &= (0<<PB0) | (0<<PB1)| (0<<PB2)|(0<<PB3); 
 
  interruptor_externo();
  timer_interrupt();

  while(1)
  {
    semaforo();
  }


}
