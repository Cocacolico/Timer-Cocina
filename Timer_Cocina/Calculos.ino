//La interrupción que gestiona el giro de la ruleta.
void sumador() {
	cli(); //stop interrupts happening before we read pin values
	flagInteract = 1;
	delayMicroseconds(100);
	if (digitalRead(14))
		valor++;
	else
		valor--;
	sei(); //restart interrupts
}

//Donde nos aseguramos de que valor esté dentro de los márgenes.
void procesadoDeValor() {
	valor *= 30;//Multiplicamos por 30 el número.
	
	if (whoIsSelected) {
		if (alarma1.tiempoProgramado >= 600)//Si ya tenemos 10 minutos o más, subiremos de minuto en minuto.
			alarma1.tiempoProgramado += valor;
		alarma1.tiempoProgramado += valor;
		if (alarma1.tiempoProgramado > 5400)//Le damos de máximo hora y media.
			alarma1.tiempoProgramado = 5400;
		if (alarma1.tiempoProgramado < 0)
			alarma1.tiempoProgramado = 0;
	}
	else {
		if (alarma0.tiempoProgramado >= 600)
			alarma0.tiempoProgramado += valor;
		alarma0.tiempoProgramado += valor;
		if (alarma0.tiempoProgramado > 5400)
			alarma0.tiempoProgramado = 5400;
		if (alarma0.tiempoProgramado < 0)
			alarma0.tiempoProgramado = 0;
	}
	valor = 0;//Restauramos su cantidad.
}

//Para hacer números que el display muestre correctamente.
int numeroASegMin(int entrada) {
	int salida=0;
	while (entrada > 59) {
		salida++;
		entrada = entrada - 60;
	}
	salida = entrada + salida * 100;
	return salida;
}

//La acción que cuenta atrás lo que esté activo y activa el flag de sonería al acabar.
void cuentaAtras() {
	if (secondPrev != second()) {
		if (alarma0.timerIsOn)alarma0.tiempoReal--;
		if (alarma1.timerIsOn)alarma1.tiempoReal--;
	}
	if (!alarma0.tiempoReal && alarma0.timerIsOn) {//Timer0 ha llegado a 0 segundos, ha de pitar.
		alarma0.timerIsOn = 0;
		//flagInteract = 1;//Mantenemos el standBy fuera durante 5 segs.
		flagTiempoEs0 = 1;
		alarma0.IsSelected = 0;//Esto hará que no pueda mostrarse en pantalla.
		alarma0.flagAlarmaReconocida = 0;
		finPitido = millis() + 3000;
	}
	if (!alarma1.tiempoReal && alarma1.timerIsOn) {//Tiene que pitar timer1.
		alarma1.timerIsOn = 0;
		//flagInteract = 1;//Mantenemos el standBy fuera durante 5 segs.
		flagTiempoEs0 = 1;
		alarma1.IsSelected = 0;//Esto hará que no pueda mostrarse en pantalla.
		alarma1.flagAlarmaReconocida = 0;
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



//Si timer0 acaba más tarde que timer1, estando encendidos los dos, los cambio de orden.
//Si timer0 no está activo y timer1 sí, los cambio de orden.
void ordenarTimers() {
	if (alarma0.timerIsOn & alarma1.timerIsOn) //Nótese que no es un AND normal, tienen que dar los dos 1.
		if (alarma0.tiempoReal > alarma1.tiempoReal) {//Elegimos el que acaba antes.
			alarma0.IsSelected = 0;
			alarma1.IsSelected = 1;
		}
		else
		{
			alarma0.IsSelected = 1;
			alarma1.IsSelected = 0;
		}

	//Otros casos, hemos de seleccionar el que se ha quedado huérfano.
	if (alarma0.timerIsOn == 0 && alarma1.timerIsOn == 1) {
		alarma1.IsSelected = 1;
	}
	if (alarma0.timerIsOn == 1 && alarma1.timerIsOn == 0) {
		alarma0.IsSelected = 1;
	}
	if (alarma0.timerIsOn == 0 && alarma1.timerIsOn == 0)
		preparacionEmisionRF24(2);//Le pido al arduino que apague la alarma.
	else
		preparacionEmisionRF24(1);//Envío al arduino principal la hora.

	///PENDIENTE de revisión
	/*
	if (!alarma0.timerIsOn && !alarma1.timerIsOn)
		preparacionEmisionRF24(2);//Pido apagar el timer en el otro arduino.
		*/
}


