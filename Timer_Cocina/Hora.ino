//Mostramos la hora adquirida del otro arduino.
void mostrarHora() {
	int muestra;
	//Primero genero el número.
	muestra = hour() * 100 + minute();
	//Luego muestro la hora, parpadeando.
	if (millis() % 2000 < 1000)
		display.showNumberDec(muestra, true, 4, 0);
	else
		display.showNumberDecEx(muestra, B01000000, true, 4, 0);
}


void emisionFR24(byte opcion) {
	boolean ok = 0;
	byte i = 0;
	datos_RF informacionSale;
	switch (opcion)
	{
	case 1://Vamos a enviar el timer actual activo.
		informacionSale.tipoDato = 1;
		informacionSale.dato = alarma0.tiempoReal;
		break;
	case 2://Vamos a anular el timer.
		informacionSale.tipoDato = 2;
		break;
	case 7://Vamos a decir que hemos quitado aquí la sonería.
		informacionSale.tipoDato = 7;
		break;
	case 8://Vamos a preguntar la hora!!
		informacionSale.tipoDato = 8;//8 en el otro arduino significa pedir la hora.
		break;
	}
	 //Todos los mensajes van a ir dirigidos al arduino nodriza (por ahora).
	RF24NetworkHeader header2(nodo_reloj);     // (Address where the data is going)
	do {
		ok = network.write(header2, &informacionSale, sizeof(informacionSale)); // Send the data
		delay(20);
		i++;
		if (ok)
			break;
	} while (i > 5);
}

void recepcionDatos() {
	long incomingData;
	while (network.available()) { // While there is data ready
		RF24NetworkHeader header;
		network.read(header, &incomingData, sizeof(incomingData));
	}
	tenemosHora = 1;
	ajustarHora(incomingData);
}

void ajustarHora(long datos) {
	//Primero convierto el tipo de dato:
	Hora.segundo = datos % 60;//Segundos.
	Hora.hora = datos / 3600;//Horas.
	datos = datos / 60;
	Hora.minuto = datos % 60;//Minutos.
	setTime(Hora.hora, Hora.minuto, Hora.segundo, 1, 1, 2000);
}