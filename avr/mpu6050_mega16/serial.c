#include "serial.h"


void shift(uint8_t k)
{
    if(k >= _buffer_length)
    {
        _buffer_length = 0;
        return;
    }


    uint8_t i,j;
    for(i = k, j = 0; i < _buffer_length; i++,j++)
    {
        _buffer[j] = _buffer[i];
    }
    _buffer_length -= k;

}

void push(byte b)
{
    if(_buffer_length < BUFFER_LEN)
    {
        _buffer[_buffer_length] = b;
        _buffer_length++;
    }
    else
    {
        shift(1);
        _buffer[_buffer_length] = b;
        _buffer_length++;
    }
}


#if defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__)
// прерывание по приёму байта
ISR(USART_RXC_vect)
{
    byte c = UDR;
    lastChar = c;
    push(c);
}
#endif

#if defined(__AVR_ATmega88P__)
// прерывание по приёму байта
ISR(USART_RX_vect)
{
    byte c = UDR0;
    lastChar = c;
    push(c);
}
#endif

uint8_t serialAvailable()
{
    return _buffer_length;
}

void serialInit(uint8_t SPEED)
{
    _buffer_length = 0;

#if defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__)
    UBRRH = (unsigned char)SPEED << 8;
    UBRRL = (unsigned char)SPEED;
    UCSRA = (0 << U2X);
	UCSRB = (0 << UDRIE) | (0 << TXCIE) | (1 << RXCIE) | (1 << RXEN) | (1 << TXEN);
    UCSRC = (1 << URSEL) | (0 << UMSEL) | (0 << USBS) | (3 << UCSZ0); //8n1
#endif

#if defined(__AVR_ATmega88P__)
    UBRR0H = (unsigned char)SPEED << 8;
    UBRR0L = (unsigned char)SPEED;
    UCSR0A = (0 << U2X0);
	UCSR0B = (0 << UDRIE0) | (0 << TXCIE0) | (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (3 << UCSZ00); //8n1
#endif

    _serial_initialized = TRUE;
}


void serialEnd()
{
#if defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__)
    UCSRB = (0 << UDRIE) | (0 << TXCIE) | (0 << RXCIE) | (0 << RXEN) | (0 << TXEN);
#endif

#if defined(__AVR_ATmega88P__)
    UCSR0B = (0 << UDRIE0) | (0 << TXCIE0) | (0 << RXCIE0) | (0 << RXEN0) | (0 << TXEN0);
#endif

    _serial_initialized = FALSE;
}


void serialWriteChar(byte ch)
{
    if(_serial_initialized)
    {
        WAIT_FOR_WRITE;
        #if defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__)
        UDR = ch;
        #endif

        #if defined(__AVR_ATmega88P__)
        UDR0 = ch;
        #endif
    }
}

void serialWrite(byte* buf)
{
    if(_serial_initialized)
    {
        while(*buf)
        {
            WAIT_FOR_WRITE;
            #if defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__)
            UDR = *buf;
            #endif
            #if defined(__AVR_ATmega88P__)
            UDR0 = *buf;
            #endif
            buf++;
        }
    }
}

void serialWriteF(const unsigned char* buf)
{
    byte c;
    if(_serial_initialized)
    {
        for(;(c = pgm_read_byte(buf)) != 0;buf++)
        {
            WAIT_FOR_WRITE;
            #if defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__)
            UDR = c;
            #endif
            #if defined(__AVR_ATmega88P__)
            UDR0 = c;
            #endif
            buf++;
        }
    }
}


byte serialLast()
{
    if( ! _buffer_length) return -1;
    byte c = lastChar;
    return c;
}


byte serialReadChar()
{
    if(! _buffer_length) return -1;

    byte c = _buffer[0];
    shift(1);
    return c;
}

byte serialRead(byte* buf, uint8_t n)
{
    while(1)
    {
        if(serialAvailable() >= n)
        {
            break;
        }
    }

    memcpy(buf,_buffer,n);
    shift(n);
    return n;
}


uint8_t serialReadUntil(byte* buf, uint8_t maxlen, byte terminator)
{
    uint8_t i;
    for(i=0;i<_buffer_length;i++)
    {
        if((_buffer[i] == terminator) || i == (maxlen-1))
        {
            memcpy(buf,_buffer,i+1);
            shift(i+1);
            return i+1;
        }
    }

    while(serialLast() != terminator)
        ;

    uint8_t length = serialAvailable();
    if(length > maxlen)
        length = maxlen;
    memcpy(buf,_buffer,length);
    shift(length);
    return length;
}


void serialClearBuffer()
{
    _buffer_length = 0;
}

uint8_t serialWaitUntil(byte terminator)
{
    while(1)
    {
        if(serialLast() == terminator)
            return serialAvailable();
    }
    return 0;
}

long serialParseInt()
{
    uint8_t found = 0;
    byte* bufptr = _buffer;
    byte* endptr = 0;
    while(bufptr - _buffer < _buffer_length)
    {
        if( (*bufptr == '-') || (isdigit(*bufptr)))
           {
               found = 1;
               break;
           }
        bufptr++;
    }
    if (!found) return 0; //not found
    long num;
    num = strtol(bufptr,&endptr,10);

    if(endptr == bufptr) return 0; //not found

    if(endptr == 0)
        shift(_buffer_length);
    else
        shift(endptr - _buffer);

    return num;
}

 float serialParseFloat()
{
    uint8_t found = 0;
    byte* bufptr = _buffer;
    byte* endptr = 0;
    while(bufptr - _buffer < _buffer_length)
    {
        if( (*bufptr == '-') || (isdigit(*bufptr)))
           {
               found = 1;
               break;
           }
        bufptr++;
    }
    if (!found) return 0; //not found
    float num;
    num = (float)strtod(bufptr,&endptr);

    if(endptr == bufptr) return 0; //not found

    if(endptr == 0)
        shift(_buffer_length);
    else
        shift(endptr - _buffer);

    return num;
}

void serialPrintInt(uint16_t value)
{
    char buf[20];
    sprintf(buf,"%i",value);
    serialWrite(buf);
}

void serialPrintIntLn(uint16_t value)
{
    char buf[20];
    sprintf(buf,"%i\n",value);
    serialWrite(buf);
}

void serialPrintDouble(double value)
{
    char buf[20];
    dtostrf(value,2,2,buf);
    serialWrite(buf);
}

void serialPrintDoubleLn(double value)
{
    serialPrintFloat(value);
    serialWrite("\n");
}

