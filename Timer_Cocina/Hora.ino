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

//La acción que selecciona qué datos enviar al arduino nodriza.
void preparacionEmisionRF24(byte opcion) {
	bool enviamos = 0;
	datos_RF informacionSale;
	switch (opcion)
	{
	case 1://Vamos a enviar el timer actual activo.
		informacionSale.tipoDato = 1;
		if (alarma0.IsSelected) {
			informacionSale.dato = alarma0.tiempoReal;
			enviamos = 1;
		}
		else if (alarma1.IsSelected) {
			informacionSale.dato = alarma1.tiempoReal;
			enviamos = 1;
		}
		break;
	case 2://Vamos a anular el timer.
		informacionSale.tipoDato = 1;
		informacionSale.dato = 0;//Envío un cero, que parará el timer en el arduino nodriza.
		enviamos = 1;
		break;
	case 7://Vamos a decir que hemos quitado aquí la sonería.
		informacionSale.tipoDato = 7;
		break;
	case 8://Vamos a preguntar la hora!!
		informacionSale.tipoDato = 8;//8 en el otro arduino significa pedir la hora.
		enviamos = 1;
		break;
	}
	
	if (enviamos)
		emisionRF24(informacionSale);
}
//La acción que emite al arduino nodriza.
void emisionRF24(datos_RF informacionSale) {
	boolean ok = 0;
	byte i = 0;

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

//Actualizamos en el sistema la hora que introduzcamos en segundos.
void ajustarHora(long datos) {
	//Primero convierto el tipo de dato:
	Hora.segundo = datos % 60;//Segundos.
	Hora.hora = datos / 3600;//Horas.
	datos = datos / 60;
	Hora.minuto = datos % 60;//Minutos.
	setTime(Hora.hora, Hora.minuto, Hora.segundo, 1, 1, 2000);
}