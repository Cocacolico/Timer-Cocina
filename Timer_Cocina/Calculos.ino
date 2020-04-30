//La interrupción que gestiona el giro de la ruleta.
void sumador() {
	cli(); //stop interrupts happening before we read pin values
	flagInteract = 1;
	delayMicroseconds(100);
	if (digitalRead(14))
		valor++;
	else
		valor--;
	//delayMicroseconds(18000);//En principio no es necesario.
	sei(); //restart interrupts
}

//Donde nos aseguramos de que valor esté dentro de los márgenes.
void procesadoDeValor() {
	if (valor < 0)
		valor = 0;
	if (valor > 180)
		valor = 180;
	if(whoIsSelected)
		alarma1.tiempoProgramado = valor * 15;
	else
		alarma0.tiempoProgramado = valor * 15;


}


int numeroASegMin(int entrada) {
	int salida=0;
	while (entrada > 59) {
		salida++;
		entrada = entrada - 60;
	}
	salida = entrada + salida * 100;
	return salida;
}

//La acción que cuenta atrás lo que esté activo y activa el flag de sonería.
void cuentaAtras() {
	if (secondPrev != second()) {
		if (alarma0.timerIsOn)alarma0.tiempoReal--;
		if (alarma1.timerIsOn)alarma1.tiempoReal--;
	}
	if (!alarma0.tiempoReal && alarma0.timerIsOn) {//Tiene que pitar timer0.
		alarma0.timerIsOn = 0;
		flagInteract = 1;//Mantenemos el standBy fuera durante 10 segs.
		flagTiempoEs0 = 1;
		alarma0.flagAlarmaReconocida = 1;
		finPitido = millis() + 3000;
	}
	if (!alarma1.tiempoReal && alarma1.timerIsOn) {//Tiene que pitar timer1.
		alarma1.timerIsOn = 0;
		flagInteract = 1;//Mantenemos el standBy fuera durante 10 segs.
		alarma1.flagAlarmaReconocida = 1;
		flagTiempoEs0 = 1;
		finPitido = millis() + 3000;
	}
	secondPrev = second();
}

//La gestión de los pitidos, de ordenarTimers() y de bajar el flagTiempoEs0.
void tiempoEs0(){
	long int opcion = finPitido - millis();
	if (opcion < 0) {//En cuanto es negativo, ya tenemos que marcharnos.
		flagTiempoEs0 = 0;
		pitarNo();
		ordenarTimers();//Los ordenamos, pues ahora sí que me interesa poner arriba el bueno.
	}
	else if (opcion > 2750)
		pitarSi();
	else if (opcion > 2500)
		pitarNo();
	else if (opcion > 2000)
		pitarSi();
	else if (opcion > 1750)
		pitarNo();
	else if (opcion > 1500)
		pitarSi();
	else if (opcion > 1000)
		pitarNo();
	else if (opcion > 750)
		pitarSi();
	else if (opcion > 500)
		pitarNo();
	else if (opcion > 0)
		pitarSi();
}

//Emitimos el pitido.
void pitarSi() {
	digitalWrite(16, 1);
}
//Dejamos de emitir el pitido.
void pitarNo() {
	digitalWrite(16, 0);
}

//Devuelve tiempoProgramado según lo que esté seleccionado.
int tiempoSeleccionado() {
	if (whoIsSelected)
		return alarma1.tiempoProgramado;
	else
		return alarma0.tiempoProgramado;
}

//Si timer0 acaba más tarde que timer1, estando encendidos los dos, los cambio de orden.
//Si timer0 no está activo y timer1 sí, los cambio de orden.
void ordenarTimers() {
	int AUX1;
	if (alarma0.timerIsOn & alarma1.timerIsOn) //Nótese que no es un AND normal, tienen que dar los dos 1.
		if (alarma0.tiempoReal > alarma1.tiempoReal) {//Hemos de cambiarlos.
			AUX1 = alarma0.tiempoReal;
			alarma0.tiempoReal = alarma1.tiempoReal;
			alarma1.tiempoReal = AUX1; 
			AUX1 = alarma0.tiempoProgramado;
			alarma0.tiempoProgramado = alarma1.tiempoProgramado;
			alarma1.tiempoProgramado = AUX1;
		}
	if (alarma0.timerIsOn == 0 && alarma1.timerIsOn == 1) {//Otro caso, hemos de mover el de abajo arriba.
		alarma0.tiempoReal = alarma1.tiempoReal;
		AUX1 = alarma0.tiempoProgramado;
		alarma0.tiempoProgramado = alarma1.tiempoProgramado;
		alarma1.tiempoProgramado = AUX1;
		alarma0.timerIsOn = 1;
		alarma1.timerIsOn = 0;
	}
	emisionFR24(1);//Envío timer0 porque sé que es el que menos le queda.
	if (!alarma0.timerIsOn && !alarma1.timerIsOn)
		emisionFR24(2);//Pido apagar el timer en el otro arduino.
}


