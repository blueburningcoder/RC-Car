/*
 * RC-Car.c
 *
 * Created: 24.10.2017 22:06:32
 * Author : Victor
 */ 
//Interner Takt 3.3V
#define F_CPU 8000000UL

#include <avr/io.h>//for Pin IO
#include <avr/interrupt.h>//for Interrupts
#include <avr/sleep.h>//for IDLE mode
#include "pinout_car.h"
#include "sx1278_defs.h"

#define schedule_inc(x) (x=(x+1)&(schedule_max-1))
#define schedule_max (1<<4)
#define default_schedule sleep

#define HEADERSIZE 0

typedef void (*task)(void);
char cData;
unsigned char schedule_first=0;
unsigned char schedule_last=0;
task scheduler[schedule_max];
task ADC_task;//reserved
unsigned char spi_buffer[2][4];
unsigned char spi_buffer_index;

//Car
void exec_command(unsigned char command){
	switch(command){
		case 0:
			break;
	}
}

void SPI_MasterInit(void)
{
	/* Set MOSI and SCK output, all others input */
	DDRB = (1<<DDB3)|(1<<DDB5);
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);

}

// utterly pointless separate 'functions'
#define SPI_write(c)    spi(c)
#define SPI_read(c)     spi(0)

uint8_t spi(uint8_t c)
{
	SPDR = c;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}

//tasks
void sleep(){
	set_sleep_mode(SLEEP_MODE_IDLE);//detup idle no wakeup time only 1/4 powerconsumption
	sleep_mode();//goto sleep
}

void add_task(task schedule) {
	scheduler[schedule_inc(schedule_last)]=schedule;
}

void get_lora_package(){
	unsigned char pack_size=SPI_read();
	for(unsigned char i=0;i<HEADERSIZE;i++) spi_buffer[!spi_buffer_index][i]=SPI_read();
	spi_buffer_index=!spi_buffer_index;
	for(unsigned char i=0;i<pack_size;i++) exec_command(SPI_read());
}

void init(){//first task to execute
	for(unsigned char i=0;i<schedule_max;i++) scheduler[i]=default_schedule;
	add_task(SPI_MasterInit);
}

//end tasks
ISR(ADC_vect)//ADC isr reserved
{
	/*for using the ISR
	ADMUX = (1<<REFS0);               // Set Reference to AVCC and input to ADC0
	ADCSRA = (1<<ADEN)|(1<<ADPS2)     // Enable ADC, set prescaler to 16
	|(1<<ADIE);               // Fadc=Fcpu/prescaler=1000000/16=62.5kHz
	*/
	add_task(ADC_task);
}

ISR(INT0_vect)//lora pack reciving
{
	/* interrupt code here */
	add_task(get_lora_package);
}


int main(void)
{
	add_task(init);//run init with scheduler
	main_loop://While(1){//Contexswitch means stack mark and goto not
		scheduler[schedule_first]();
		scheduler[schedule_first]=default_schedule;
		(schedule_first==schedule_last)?schedule_first=schedule_inc(schedule_last):schedule_inc(schedule_first);
	goto main_loop;//}
}