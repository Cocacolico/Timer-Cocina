#include <SPI.h>
#include <Sync.h>
//#include <RF24Network_config.h>
#include <RF24Network.h>
#include <Time.h>
#include <Arduino.h>
#include <TM1637Display.h>
#include <TimeLib.h>

//#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
//#include <RF24_config.h>
//        CE, CSN
RF24 radio(9, 10);
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 01;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t nodo_reloj = 00;
struct datos_RF {                 // Estructura de env�os y recepciones.
	byte tipoDato;
	unsigned long dato;
};

// Display i2c connection pins (Digital Pins)
#define CLK 4
#define DIO 5
#define B_TIMER0 17
#define B_TIMER1 18
#define L_TIMER0 6 
#define L_TIMER1 8

TM1637Display display(CLK, DIO);

volatile int valor = 0;//La variable que alteramos con la ruleta.
int tiempoProgramado0 = 0;//El tiempo en segundos programado para el timer.
int tiempoReal0 = 0;//El tiempo en segundos que quedan en el timer.
int tiempoProgramado1 = 0;//El tiempo en segundos programado para el timer.
int tiempoReal1 = 0;//El tiempo en segundos que quedan en el timer.
bool timer0IsOn = 0;//Si=1, estamos contando atr�s.
bool timer1IsOn = 0;//Si=1, estamos contando atr�s.
bool flagAlarma0 = 0;//Si=1, a�n no hemos reconocido la alarma.
bool flagAlarma1 = 0;//Si=1, a�n no hemos reconocido la alarma.
byte secondPrev; //Lo usar� para ir restando segundos al contador.
bool selected = 0;//Si=0, hablamos de timer0.
unsigned long nextTimeQuery = 0;//Tiempo entre preguntas al arduino nodriza por la hora.
bool flagTiempoEs0 = 0;//Cuando haya que pitar! 
unsigned long finPitido;//El momento en millis donde parar� el pitido.
bool flagInteract = 0;//Se pondr� a 1 si interactuamos o el equipo necesita mostrar algo.
bool standBy = 0;//Si=1, mostramos la hora o el vac�o.
unsigned long standByTimeOut;//Alcanzado un determinado valor, pone el equipo en standby.
bool tenemosHora = 0;//Hemos adquirido la hora del arduino principal.
int contador;//Esta variable va sum�ndose y altera con el tiempo otras variables.

byte hora[3];

//Declaraciones de funciones:
void sumador();
void procesadoDeValor();
int numeroASegMin(int);
void cuentaAtras();
void tiempoEs0();
void pitarSi();
void pitarNo();
int tiempoSeleccionado();
void ordenarTimers();
void mostrarHora();
void emisionFR24(byte);
void recepcionDatos();
void ajustarHora(long int);

void setup()
{
	SPI.begin();
	Serial.begin(9600);
	radio.begin();// Open a writing and reading pipe on each radio, with opposite addresses
	network.begin(90,this_node);

	display.setBrightness(7, true); // Turn on
	display.showNumberDec(14, false, 2, 1); // Expect: _14_ //True hace ceros por delante.
	//delay(2444);
	//display.showNumberDecEx(6634, B01000000, false, 4, 0);
	//delay(2444);
	

	pinMode(3, INPUT);//El pin de las interrupciones. Rotatory encoder.
	pinMode(14, INPUT);//El otro pin implicado, del bot�n. Rotatory encoder.
	pinMode(15, INPUT);//Pulsador del bot�n. Rotatory encoder.
	pinMode(B_TIMER0, INPUT);//Bot�n timer0.
	pinMode(B_TIMER1, INPUT);//Bot�n timer1.
	pinMode(16, OUTPUT);//Zumbador.
	pinMode(L_TIMER0, OUTPUT);//Led timer0.
	pinMode(L_TIMER1, OUTPUT);//Led timer1.
	digitalWrite(3, 1);//PullUpResistors.
	digitalWrite(14, 1);//
	digitalWrite(B_TIMER0, 1);
	digitalWrite(B_TIMER1, 1);

	attachInterrupt(digitalPinToInterrupt(3), sumador, RISING);
}

void loop()
{
	/*
	Serial.print(flagAlarma0);
	Serial.print(flagAlarma1);
	Serial.print(standBy);
	Serial.print(selected);
	Serial.print(" ");
	Serial.print(tenemosHora);
	Serial.print(hour());
	Serial.print(minute());
	Serial.print("  ");
	Serial.print(hora[0]);
	Serial.println(hora[1]);
	*/


	procesadoDeValor();
	cuentaAtras();


	///RADIO 
	network.update();
	if (network.available())
		recepcionDatos();

	if(flagInteract){
		standByTimeOut = millis() + 10000;//El equipo pasa a descansar en 10 segundos.
		flagInteract = 0;
		standBy = 0;
	}
	if (standByTimeOut < millis()) {
		standBy = 1;
	}

	///Muestra de la luz de los botones:
	byte auxContador;
	contador++;
	if (contador > 250)
		contador = 0;
	if (contador < 125)
		auxContador = contador * 2;
	else
		auxContador = 250 - (contador - 125) * 2;
	if (standBy) {//En standBy no usamos selected
		if (timer0IsOn)
			analogWrite(L_TIMER0, auxContador);
		if (timer1IsOn)
			analogWrite(L_TIMER1, auxContador);
	}
	else if (selected) {
		digitalWrite(L_TIMER1, 1);
		//Iluminar continuo BOTON1.
	}
	else {
		digitalWrite(L_TIMER0, 1);
		//Iluminar continuo BOTON0.
	}

	if (flagAlarma0)
		if (contador < 110)
			digitalWrite(L_TIMER0, 0);
		else
			digitalWrite(L_TIMER0, 1);
		//Destellos contador de timer0
	if(flagAlarma1)
		if (contador < 110)
			digitalWrite(L_TIMER1, 0);
		else
			digitalWrite(L_TIMER1, 1);
		//Destellos contador de timer1.
	


	///Qu� mostramos en los displays:
	if (timer0IsOn) {//Primero siempre el timer0.
		display.showNumberDecEx(numeroASegMin(tiempoReal0), B01000000, true, 4, 0);
	}
	else if (timer1IsOn) {//Despu�s timer1.
		display.showNumberDecEx(numeroASegMin(tiempoReal1), B01000000, true, 4, 0);
	}
	else//Si est�n los 2 timers parados:
	{
		if (flagTiempoEs0 == 0)
			if (standBy == 0)
				display.showNumberDecEx(numeroASegMin(tiempoSeleccionado()), B01000000, true, 4, 0);
			else if (tenemosHora)
				mostrarHora();
			else//No tenemos hora y estamos en standBy:
				display.clear();
		else //FlagTiempoEs0=1, mostramos 00:00. Y muestro los dos puntos, el timer est� parado.
			display.showNumberDecEx(numeroASegMin(0), B01000000, true, 4, 0);
		
	}
	
	///Pulsamos alguno de los dos botoncitos:
	if (!digitalRead(B_TIMER0)) {//Recuerda que debe dar un 0 cuando lo pulsamos.
		flagInteract = 1;//Hemos interactuado.
		flagAlarma0 = 0;
		delay(25);
		while (!digitalRead(B_TIMER0));//Esperamos a que lo suelte.
		selected = 0;
		digitalWrite(L_TIMER1, 0); //Apagamos el otro bot�n.
	}

	if (!digitalRead(B_TIMER1)) {//Recuerda que debe dar un 0 cuando lo pulsamos.
		flagInteract = 1;//Hemos interactuado.
		flagAlarma1 = 0;
		delay(25);
		while (!digitalRead(B_TIMER1));//Esperamos a que lo suelte.
		selected = 1;
		digitalWrite(L_TIMER0, 0); //Apagamos el otro bot�n.
	}

	///Pulsamos el bot�n e iniciamos o paramos la cuenta atr�s:
	if (!digitalRead(15)) {
		flagInteract = 1;//Hemos interactuado.
		delay(100);//ELIMINAR, reduce los rebotes.
		while (!digitalRead(15));//Me espero hasta que suelte el bot�n.

		if(selected)//Estamos hablando de timer1.
			if (timer1IsOn) {//Paramos el timer.
				timer1IsOn = 0;
			}
			else
			{
				if (tiempoProgramado1 > 0) {
					timer1IsOn = 1;
					tiempoReal1 = tiempoProgramado1;
				}
			}
		else//Hablamos de timer0.
			if (timer0IsOn) {
				timer0IsOn = 0;
			}
			else
			{
				if (tiempoProgramado0 > 0) {
					timer0IsOn = 1;
					tiempoReal0 = tiempoProgramado0;
				}
			}
		ordenarTimers();
	}

	if (flagTiempoEs0)
		tiempoEs0();

	if(nextTimeQuery < millis())//Entraremos al principio y luego de vez en cuando.
		if (tenemosHora) {
			nextTimeQuery = 604800000 + millis();//Una semana en segundos.
		}
		else
		{
			nextTimeQuery = 3600000 + millis();//Preguntaremos cada hora.
			emisionFR24(8);//Ocho es el n�mero que quiere el otro arduino.
		}

	
}