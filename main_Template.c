#include <p24fxxxx.h>
#include <stdio.h>
#include <string.h>

// Configuration Bits
#ifdef __PIC24FJ64GA004__ //Defined by MPLAB when using 24FJ64GA004 device
_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF & COE_OFF & FWDTEN_OFF & ICS_PGx1 & IOL1WAY_ON) 
_CONFIG2( FCKSM_CSDCMD & OSCIOFNC_OFF & POSCMOD_HS & FNOSC_PRI & I2C1SEL_SEC)
#else
_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF & COE_OFF & FWDTEN_OFF & ICS_PGx2) 
_CONFIG2( FCKSM_CSDCMD & OSCIOFNC_OFF & POSCMOD_HS & FNOSC_PRI)
#endif

char rx [50];
int irx = 0;
int received_message = 0;


int timer = 0;

void __attribute__((__interrupt__, auto_psv)) _U2RXInterrupt(void){ 
	rx[irx] = U2RXREG;
	IFS1bits.U2RXIF = 0;
 
	//send_string("b");
	if(rx[irx]== '\n'){
			
		rx[irx]= '\0';
		irx = 0;
		//send_string(rx);
		received_message = 1;
		return;
	}	
	irx++;
	return;
}

void __attribute__((__interrupt__, auto_psv)) _T1Interrupt(void)
{
	timer++;
	/* Interrupt Service Routine code goes here */
	IFS0bits.T1IF = 0; //Reset Timer1 interrupt flag and Return from ISR
}

void I2C2_setup(){
	// timing and baud rate calculations
	I2C2BRG = 39;
	I2C2CON = 0x8000;
}

void I2C_read(int adress, int mode, int light, int temp){
	int adress_shift;
	adress_shift = adress << 1; // write uses last bit as 0
	
	I2C2CONbits.SEN = 1; //Start message
	while(I2C2CONbits.SEN){} // wait for SEN to go to 0
	I2C2TRN = adress_shift;
	while(I2C2STATbits.TBF){} // wait for ACK from slave
	while(I2C2STATbits.TRSTAT){}
	if(I2C2STATbits.ACKSTAT){
		// SEND MESSAGE SAYING ARDUINO NOT RESPONDING
		return;
	}
	I2C2TRN = mode; // send data
	while(I2C2STATbits.TBF){}
	while(I2C2STATbits.TRSTAT){}
	if (light != -1){
		I2C2TRN = light >> 8;
		while(I2C2STATbits.TBF){}
		while(I2C2STATbits.TRSTAT){}
		I2C2TRN = light;
		while(I2C2STATbits.TBF){}
		while(I2C2STATbits.TRSTAT){}
	}
	if (temp != -1){
		I2C2TRN = temp >> 8;
		while(I2C2STATbits.TBF){}
		while(I2C2STATbits.TRSTAT){}
		I2C2TRN = temp;
		while(I2C2STATbits.TBF){}
		while(I2C2STATbits.TRSTAT){}
	}
	I2C2CONbits.PEN = 1;
	while(I2C2CONbits.PEN){}
}

int I2C_write(int adress){
	adress = adress << 1;

	adress += 1;

	I2C2CONbits.SEN = 1; //Start message
	while(I2C2CONbits.SEN == 1){} // wait for SEN to go to 0
	I2C2TRN = adress;
	while(I2C2STATbits.TBF == 1){} // wait for ACK from slave
	while(I2C2STATbits.TRSTAT == 1){}
	//if(I2C2STATbits.ACKSTAT == 1){
		// SEND MESSAGE SAYING ARDUINO NOT RESPONDING
	//}

	I2C2CONbits.RCEN = 1;
	while(I2C2STATbits.RBF == 0){}
	int data = I2C2RCV;
	
//	while(I2C2CONbits.RCEN == 1){}
//	I2C2CONbits.ACKDT = 0;
	I2C2CONbits.PEN = 1;
	while(I2C2CONbits.PEN == 1){}
	return data;
}

void send_char(char c){
	//while(U2STAbits.UTXBF){}
	if(U2STAbits.UTXBF == 0){ //1 = Transmit buffer is full / 0 = Transmit buffer is not full, at least one more data word can be written
		U2TXREG = c;
	}
}

void send_string(char * string){
	int i=0;
	int j=0;
	PORTAbits.RA5 = 1;
	while(1){
		send_char(string[i]);
		i++;
		if(string[i]=='\0'){
			send_char('\n');
			break;
		}
		for( j = 0 ; j < 2000 ; j++){};//delay to avoid filling buffer
	}
	PORTAbits.RA5 = 0;
}


int converter(int port){
	int i = 0;
	AD1CHS = port; 			// Connect AN2 as S/H+ input
	AD1CON1bits.SAMP = 1; 		// start sampling...
	for(i = 0 ; i < 2 ; i++){};
								// before starting conversion.
	AD1CON1bits.SAMP = 0; 		// start converting
	while (!AD1CON1bits.DONE){}; // conversion done?
	return ADC1BUF0; // yes then get ADC value
}

void system_state(int state){
	char mod[20];
	char msg[50];

	if (state == 1){ //blinds UP
		LATAbits.LATA0 = 1;
		LATAbits.LATA1 = 0;
		strcpy(mod, "subir");
	}
	else if (state == -1){ //blinds DOWN
		LATAbits.LATA0 = 0;
		LATAbits.LATA1 = 1;
		strcpy(mod, "descer");
	}
	else{ //blinds still
		LATAbits.LATA0 = 0;
		LATAbits.LATA1 = 0;
		strcpy(mod, "parado");
	}
	sprintf(msg, "<msg><m>6</m><e>%s</e></msg>", mod);
	send_string(msg);
}

void show_values(){
	char string_values[50];
	int Light_Level, Temperature_Level;

	Light_Level = converter(3);
	Temperature_Level = converter(2);
	sprintf(string_values, "<msg><m>7</m><l>%d</l><t>%d</t></msg>", Light_Level, Temperature_Level);
	send_string(string_values);
}

int get_next_state(int light_avg, int temp_avg){ //light is 80% and temp is 20% of the result
	int state = 0;
	int Light_Level, Temperature_Level, res_avg, res, res_final;

	Light_Level = converter(3);
	Temperature_Level = converter(2);
	
	res_avg = light_avg*0.8+temp_avg*0.2;
	res = Light_Level*0.8+Temperature_Level*0.2;

	res_final = res_avg - res;

	if (res_final < -30){
		state = -1;
		LATAbits.LATA0 = 0;
		LATAbits.LATA1 = 1;
	}
	else if (res_final > 30){
		state = 1;
		LATAbits.LATA0 = 1;
		LATAbits.LATA1 = 0;
	}
	else{
		state = 0;
		LATAbits.LATA0 = 0;
		LATAbits.LATA1 = 0;
	}
	
	return state;
}

int main(void){
		/* The following code will enable Timer1 interrupts, load the Timer1
	Period register and start Timer1.
	When a Timer1 period match interrupt occurs, the interrupt service
	routine must clear the Timer1 interrupt status flag in software.
	*/
	U2MODEbits.UARTEN = 1; //Enable UART
	// timing and baud rate calculations
	U2BRG = 25;        	// 9600 baud (BREGH=0)

	U2STA = 0;
	U2STAbits.ADDEN = 1; //Address detect enabled

	U2MODE = 0x8080; //Enable Uart for 8-bit data
	
	U2STAbits.UTXEN = 1; //Enable Transmit and Receive UART pins

	IFS1bits.U2RXIF = 0;
	IEC1bits.U2RXIE = 1; // enable uart receive interrupt

	T1CON = 0x00; //Stops the Timer1 and reset control reg.
	TMR1 = 0x00; //Clear contents of the timer register
	// FFFF cycles to FFFF is 1 cycles, and 4Mhz/ (2^16 = 65 536) = 61 vezes interrupt ativa da 1s
	PR1 = 0xFFFF; //Load the Period register with the value 0xFFFF
	IPC0bits.T1IP = 0x01; //Setup Timer1 interrupt for desired priority level
	// (This example assigns level 1 priority)
	IFS0bits.T1IF = 0; //Clear the Timer1 interrupt status flag
	IEC0bits.T1IE = 1; //Enable Timer1 interrupts
	T1CONbits.TON = 1; //Start Timer1 with prescaler settings at 1:1 and
	//clock source set to the internal instruction cycle

	TRISDbits.TRISD6 = 1;
	TRISDbits.TRISD7 = 1;
	TRISDbits.TRISD13 = 1;

	TRISAbits.TRISA0 = 0;
	TRISAbits.TRISA1 = 0;
	TRISAbits.TRISA5 = 0;

	I2C2_setup(); // setup I2C variables

	AD1PCFG = 0xFFF3; // AN2 and AN3 as analog, all other pins are digital
	AD1CON1 = 0x0000; // SAMP bit = 0 ends sampling and starts converting
	AD1CSSL = 0; // No inputs are scanned
	AD1CON3 = 0x0002; // Manual Sample, Tad = 3Tcy
	AD1CON2 = 0; // Configure A/D voltage reference
	AD1CON1bits.ADON = 1; // turn ADC ON
	
//	IPC7bits.U2RXIP2=0;
//	IPC7bits.U2RXIP1=0;
//	IPC7bits.U2RXIP0=0;

 	int mode = 0, light = 0, temp = 0;	
 	int adress = 0x0A;
	int data;
	int i;
	send_string("comeco");
	while ( 1 ){

		if(received_message){
			send_string(rx);
			received_message = 0;
		}
		if (timer == 305 /*61*5s*/){
			timer = 0;

			light = converter(3);
			temp = converter(2);
			
			data = I2C_write(adress);
			if(data !=0){
				I2C_read(adress, 3, light, temp);
			}
		}
	}
}
