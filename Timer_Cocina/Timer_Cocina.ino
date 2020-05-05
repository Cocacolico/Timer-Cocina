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
RF24 radio(8, 10);//OJO!! En la placa están puestos los pines 9,10.
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 01;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t nodo_reloj = 00;
struct datos_RF {                 // Estructura de envíos y recepciones.
	byte tipoDato;
	long dato;
};

// Display i2c connection pins (Digital Pins)
#define CLK 4
#define DIO 5
#define B_TIMER0 17
#define B_TIMER1 18
#define L_TIMER0 6 
#define L_TIMER1 9//OJO!! En la placa está puesto el pin D8.

TM1637Display display(CLK, DIO);



//La estructura de las alarmas.
struct Alarmas {
	bool IsSelected = 0;//Un 1 implica que está activa y es la primera en acabar.
	byte pinDeLED;//El pin que usaremos para el led.
	byte pinDeBoton;//El pin que usaremos para el botón relacionado.
	bool timerIsOn = 0;//El timer está corriendo y no en pausa.
	bool flagAlarmaReconocida = 1;//Hemos reconocido esta alarma tras pitar?
	int tiempoReal = 0;//El tiempo en segundos que quedan en el timer.
	int tiempoProgramado=0;//El tiempo en segundos programado para el timer.
	unsigned long millisFinTimer = 0;//El momento en millis que acabará la alarma.
	bool soyTimer1 = 0;//El que tenga un 1 será el que mostremos en pantalla.
};
Alarmas alarma0;
Alarmas alarma1;//Creo las dos alarmas.

volatile int valor = 0;//La variable que alteramos con la ruleta.
byte secondPrev; //Lo usaré para ir restando segundos al contador.
bool whoIsSelected = 0;//Si=0, hablamos de timer0. Se usa para cuando interactuamos.
unsigned long nextTimeQuery = 0;//Tiempo entre preguntas al arduino nodriza por la hora.
bool flagTiempoEs0 = 0;//Cuando haya que pitar! 
unsigned long finPitido;//El momento en millis donde parará el pitido.
bool flagInteract = 0;//Se pondrá a 1 si interactuamos o el equipo necesita mostrar algo.
bool standBy = 0;//Si=1, mostramos la hora o el vacío.
unsigned long standByTimeOut;//Alcanzado un determinado valor, pone el equipo en standby.
bool tenemosHora = 0;//Hemos adquirido la hora del arduino principal.
int contador;//Esta variable va sumándose y altera con el tiempo otras variables.

struct Horas {
	byte hora;
	byte minuto;
	byte segundo;
};
Horas Hora;//Creo una instancia de la estructura.

//Declaraciones de funciones:
void sumador();
void procesadoDeValor();
int numeroASegMin(int);
void cuentaAtras();
void tiempoEs0();
void pitarSi();
void pitarNo();
void ordenarTimers();
void mostrarHora();
void emisionFR24(byte);
void recepcionDatos();
void ajustarHora(long);


void setup()
{
	SPI.begin();
	Serial.begin(9600);
	radio.begin();// Open a writing and reading pipe on each radio, with opposite addresses
	network.begin(90,this_node);

	display.setBrightness(7, true); // Turn on
	//display.showNumberDec(14, false, 2, 1); // Expect: _14_ //True hace ceros por delante.
	//delay(2444);
	//display.showNumberDecEx(6634, B01000000, false, 4, 0);
	//delay(2444);
	
	//Inputs
	pinMode(3, INPUT);//El pin de las interrupciones. Rotatory encoder.
	pinMode(14, INPUT);//El otro pin implicado, del botón. Rotatory encoder.
	pinMode(15, INPUT);//Pulsador del botón. Rotatory encoder.
	alarma0.pinDeBoton = B_TIMER0;//Le asigno el botón que tiene.
	alarma1.pinDeBoton = B_TIMER1;
	pinMode(B_TIMER0, INPUT);//Botón timer0.
	pinMode(B_TIMER1, INPUT);//Botón timer1.

	//Outputs
	pinMode(16, OUTPUT);//Zumbador.
	alarma0.pinDeLED = L_TIMER0;
	alarma0.pinDeLED = L_TIMER1;
	pinMode(L_TIMER0, OUTPUT);//Led timer0.
	pinMode(L_TIMER1, OUTPUT);//Led timer1.
	digitalWrite(3, 1);//PullUpResistors.
	digitalWrite(14, 1);//
	digitalWrite(B_TIMER0, 1);
	digitalWrite(B_TIMER1, 1);

	//Inicializamos la interrupción del rotatory encoder.
	attachInterrupt(digitalPinToInterrupt(3), sumador, RISING);
}

void loop()
{
	

	procesadoDeValor();
	cuentaAtras();


	///RADIO 
	network.update();
	if (network.available())
		recepcionDatos();

	if(flagInteract){
		standByTimeOut = millis() + 5000;//El equipo pasa a descansar en 5 segundos.
		flagInteract = 0;
		standBy = 0;
	}
	if (standByTimeOut < millis()) {
		standBy = 1;
	}

	///Muestra de la luz de los botones:
	byte auxContador;
	contador += 3;
	if (contador > 250)
		contador = 0;
	if (contador < 125)
		auxContador = contador * 2;
	else
		auxContador = 250 - (contador - 125) * 2;


	if (standBy) {//En standBy no usamos whoIsSelected
		if (alarma0.timerIsOn)
			analogWrite(L_TIMER0, auxContador);
		else if (!alarma0.flagAlarmaReconocida)
			if (contador < 110)
				digitalWrite(L_TIMER0, 0);
			else
				digitalWrite(L_TIMER0, 1);
		//Destellos contador de timer0
		if (alarma1.timerIsOn)
			analogWrite(L_TIMER1, auxContador);
		else if (!alarma1.flagAlarmaReconocida)
			if (contador < 110)
				digitalWrite(L_TIMER1, 0);
			else
				digitalWrite(L_TIMER1, 1);
		//Destellos contador de timer1.
	}
	else if (whoIsSelected) {
		digitalWrite(L_TIMER1, 1);
		//Iluminar continuo BOTON1.
	}
	else {
		digitalWrite(L_TIMER0, 1);
		//Iluminar continuo BOTON0.
	}

	


	///Qué mostramos en los displays:
	if(standBy){
		if(flagTiempoEs0)//FlagTiempoEs0=1, mostramos 00:00. Y muestro los dos puntos, el timer está parado.
			display.showNumberDecEx(numeroASegMin(0), B01000000, true, 4, 0);
		else if (alarma0.IsSelected)//Muestro esta alarma.
			display.showNumberDecEx(numeroASegMin(alarma0.tiempoReal), B01000000, true, 4, 0);
		else if (alarma1.IsSelected)//Muestro esta alarma.
			display.showNumberDecEx(numeroASegMin(alarma1.tiempoReal), B01000000, true, 4, 0);
		else if (tenemosHora)//Si están los dos temporizadores parados, entonces mostramos la hora.
			mostrarHora();
		else//No tenemos hora y estamos en standBy:
			display.clear();
	}
	else//No estamos en stand by:
	{
		if (!whoIsSelected) {//Empiezo por timer0.
			if(alarma0.timerIsOn)
				display.showNumberDecEx(numeroASegMin(alarma0.tiempoReal), B01000000, true, 4, 0);
			else
				display.showNumberDecEx(numeroASegMin(alarma0.tiempoProgramado), B01000000, true, 4, 0);
		}
		else
		{
			if (alarma1.timerIsOn)
				display.showNumberDecEx(numeroASegMin(alarma1.tiempoReal), B01000000, true, 4, 0);
			else
				display.showNumberDecEx(numeroASegMin(alarma1.tiempoProgramado), B01000000, true, 4, 0);
		}
	}



	


	///Pulsamos alguno de los dos botoncitos:
	if (!digitalRead(B_TIMER0)) {//Recuerda que debe dar un 0 cuando lo pulsamos.
		flagInteract = 1;//Hemos interactuado.
		alarma0.flagAlarmaReconocida = 1;
		delay(25);
		while (!digitalRead(B_TIMER0));//Esperamos a que lo suelte.
		whoIsSelected = 0;
		digitalWrite(L_TIMER1, 0); //Apagamos el otro botón.
	}

	if (!digitalRead(B_TIMER1)) {//Recuerda que debe dar un 0 cuando lo pulsamos.
		flagInteract = 1;//Hemos interactuado.
		alarma1.flagAlarmaReconocida = 1;
		delay(25);
		while (!digitalRead(B_TIMER1));//Esperamos a que lo suelte.
		whoIsSelected = 1;
		digitalWrite(L_TIMER0, 0); //Apagamos el otro botón.
	}

	///Pulsamos el botón e iniciamos o paramos la cuenta atrás:
	if (!digitalRead(15)) {
		flagInteract = 1;//Hemos interactuado.
		delay(100);//ELIMINAR, reduce los rebotes.
		while (!digitalRead(15));//Me espero hasta que suelte el botón.

		if(whoIsSelected)//Estamos hablando de timer1.
			if (alarma1.timerIsOn) {//Paramos el timer.
				alarma1.timerIsOn = 0;
			}
			else
			{
				if (alarma1.tiempoProgramado > 0) {
					alarma1.timerIsOn = 1;
					alarma1.flagAlarmaReconocida = 1;
					alarma1.tiempoReal = alarma1.tiempoProgramado;
				}
			}
		else//Hablamos de timer0.
			if (alarma0.timerIsOn) {
				alarma0.timerIsOn = 0;
			}
			else
			{
				if (alarma0.tiempoProgramado > 0) {
					alarma0.timerIsOn = 1;
					alarma0.flagAlarmaReconocida = 1;
					alarma0.tiempoReal = alarma0.tiempoProgramado;
				}
			}
		ordenarTimers();
	}

	if (flagTiempoEs0)
		tiempoEs0();

	if(nextTimeQuery < millis())//Entraremos al principio y luego de vez en cuando.
		if (tenemosHora) {
			nextTimeQuery = 604800000 + millis();//Una semana en segundos.
			emisionFR24(8);//Ocho es el número que quiere el otro arduino.
		}
		else
		{
			nextTimeQuery = 3600000 + millis();//Preguntaremos cada hora.
			emisionFR24(8);//Ocho es el número que quiere el otro arduino.
		}

	
}