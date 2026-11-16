#include <lpc21xx.h>

#define RS   (1 << 10)  // P0.10 -> Register Select
#define EN   (1 << 12)  // P0.12 -> Enable
#define D4   (1 << 16)  // P0.16 -> Data Bit 4
#define D5   (1 << 17)  // P0.17 -> Data Bit 5
#define D6   (1 << 18)  // P0.18 -> Data Bit 6
#define D7   (1 << 19)  // P0.19 -> Data Bit 7

#define mf   (1 << 8)   // P0.8  -> Door Open
#define mr   (1 << 4)   // P0.4  -> Door Close
#define SW   5          // P0.5  -> Push Button S1
#define LED1 (1 << 6)   // P0.6  -> LED Indicator
#define buzz (1 << 7)   // P0.7  -> Buzzer

void delay_ms(unsigned int count) 
{
    unsigned int i, j;
    for (i = 0; i < count; i++)
        for (j = 0; j < 6000; j++);
}

void LCD_Command(char cmd) 
{
    IO0CLR = RS; // RS = 0 for command
    
    IO0CLR = (D4 | D5 | D6 | D7);
    if (cmd & 0x10) IO0SET = D4;
    if (cmd & 0x20) IO0SET = D5;
    if (cmd & 0x40) IO0SET = D6;
    if (cmd & 0x80) IO0SET = D7;
    
    IO0SET = EN; delay_ms(1); IO0CLR = EN;

    IO0CLR = (D4 | D5 | D6 | D7);
    if (cmd & 0x01) IO0SET = D4;
    if (cmd & 0x02) IO0SET = D5;
    if (cmd & 0x04) IO0SET = D6;
    if (cmd & 0x08) IO0SET = D7;
    
    IO0SET = EN; delay_ms(1); IO0CLR = EN;
}

void LCD_Char(char data) 
{
    IO0SET = RS; // RS = 1 for data
    
    IO0CLR = (D4 | D5 | D6 | D7);
    if (data & 0x10) IO0SET = D4;
    if (data & 0x20) IO0SET = D5;
    if (data & 0x40) IO0SET = D6;
    if (data & 0x80) IO0SET = D7;
    
    IO0SET = EN; delay_ms(1); IO0CLR = EN;

    IO0CLR = (D4 | D5 | D6 | D7);
    if (data & 0x01) IO0SET = D4;
    if (data & 0x02) IO0SET = D5;
    if (data & 0x04) IO0SET = D6;
    if (data & 0x08) IO0SET = D7;
    
    IO0SET = EN; delay_ms(1); IO0CLR = EN;
}

void LCD_Init(void) 
{
    IO0DIR |= (RS | buzz | EN | D4 | D5 | D6 | D7 | mf | mr | LED1);
    IO0SET = buzz | LED1; // Keep buzzer and LED quiet/off at startup
    delay_ms(20);
    
    LCD_Command(0x02); // Initialize LCD in 4-bit mode
    LCD_Command(0x28); // 2 lines, 5x7 matrix
    LCD_Command(0x0C); // Display ON, Cursor OFF
    LCD_Command(0x06); // Auto increment cursor
    LCD_Command(0x01); // Clear display
    delay_ms(5);
}

void LCD_String(char *str) 
{
    while (*str) 
        LCD_Char(*str++);
 
}

void UART0_Init(void) 
{
    PINSEL0 |= 0x05;        // P0.0 = TXD0, P0.1 = RXD0
    U0LCR = 0x83;           // 8N1 frame, DLAB = 1
    U0DLL = 97;             // Baud rate 
    U0DLM = 0;
    U0LCR = 0x03;           // DLAB = 0
    U0FCR = 0x07;           // Enable and clear FIFO
}

char UART0_ReadChar(void) 
{
    while (!(U0LSR & 0x01)); 
    return U0RBR;
}

int main(void) 
{
    char rfid_settag[13] = "060067849F";
    char rfid_tag[13];
    int i, j;
    int flag;

    LCD_Init();
    UART0_Init();

    LCD_String("RFID Reader Ready");
    delay_ms(5000);
    LCD_Command(0x01); 

    while (1) 
	{
        for (i = 0; i < 12; i++) 
		{
            rfid_tag[i] = UART0_ReadChar();
        }
        rfid_tag[12] = '\0'; 

        LCD_Command(0x01);
        delay_ms(2);
        LCD_String("UID Detected:");
        LCD_Command(0xC0);         
        
        rfid_tag[10] = '\0'; 
        
        LCD_String(rfid_tag);
        delay_ms(5000);

        flag = 1;

        for (j = 0; j < 10; j++) 
		{
            if (rfid_settag[j] != rfid_tag[j]) 
			{
                flag = 0;
                break;
            }
        }

        if (flag == 1) 
		{
            LCD_Command(0x01);
            delay_ms(2);
            LCD_String("VALID USER:");
            LCD_Command(0xC0);
            LCD_String("DOOR OPEN");
            
            IO0SET = mf;
            IO0CLR = mr;
            delay_ms(5000);
            IO0CLR = mf | mr;
            
            LCD_Command(0x01);
            delay_ms(2);
            LCD_String("PRESS S1 TO SHUT");
            
            while(1) 
			{
                if(((IOPIN0 >> SW) & 1) == 0) 
				{
                    LCD_Command(0x01);
                    delay_ms(2);
                    LCD_String("DOOR CLOSING");

                    IO0SET = mr;
                    IO0CLR = mf;
                    delay_ms(5000);
                    IO0CLR = mr | mf;
                    break;
                }
            }

        } 
		else 
		{
            LCD_Command(0x01);
            delay_ms(2);
            LCD_String("INVALID USER:");
            LCD_Command(0xC0);          
            LCD_String("ACCESS DENIED");
            
            IO0CLR = buzz | LED1;      
            delay_ms(5000);
            IO0SET = buzz | LED1;      
        }

        delay_ms(1000); 
    }
}
