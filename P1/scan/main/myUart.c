#include "myUart.h"

void uartInit(uart_port_t uart_num, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop, uint8_t txPin, uint8_t rxPin)
{
    uart_config_t uart_config = {
        .baud_rate = (int) baudrate,
        .data_bits = (uart_word_length_t)(size-5),
        .parity    = (parity==eParityEven)?UART_PARITY_EVEN:(parity==eParityOdd)?UART_PARITY_ODD:UART_PARITY_DISABLE,
        .stop_bits = (stop==eStop1)?UART_STOP_BITS_1:UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_REF_TICK,
    };

    ESP_ERROR_CHECK(uart_driver_install(uart_num, READ_BUF_SIZE, READ_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, txPin, rxPin,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

}

void delayMs(uint16_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void uartClrScr(uart_port_t uart_num)
{
    // Uso "const" para sugerir que el contenido del arreglo lo coloque en Flash y no en RAM
    const char caClearScr[] = "\e[2J";
    uart_write_bytes(uart_num, caClearScr, sizeof(caClearScr));
}
void uartGoto11(uart_port_t uart_num)
{
    // Limpie un poco el arreglo de caracteres, los siguientes tres son equivalentes:
     // "\e[1;1H" == "\x1B[1;1H" == {27,'[','1',';','1','H'}
    const char caGoto11[] = "\e[1;1H";
    uart_write_bytes(uart_num, caGoto11, sizeof(caGoto11));
}

bool uartKbhit(uart_port_t uart_num)
{
    uint8_t length;
    uart_get_buffered_data_len(uart_num, (size_t*)&length);
    return (length > 0);
}

void uartPutchar(uart_port_t uart_num, char c)
{
    uart_write_bytes(uart_num, &c, sizeof(c));
}

char uartGetchar(uart_port_t uart_num)
{
    unsigned char c;
    // Wait for a received byte
    while(!uartKbhit(uart_num))
    {
        delayMs(10);
    }
    // read byte, no wait
    uart_read_bytes(uart_num,&c, sizeof(c), 0);

    return c;
}

void uartPuts(uart_port_t uart_num, char *str)
{
    while(*str!='\0')
    {   
        uartPutchar(uart_num,*str);
        (str++);
    }
}

void uartGets(uart_port_t uart_num, char *str)
{
    char c;
    int cont=0;

    while(1)
    {
        c=uartGetchar(uart_num);

        if(c==13)
        {
            str[cont]='\0';
            break;
        }
        else
        {
            if(c==8)
            {
                cont--;
                if(cont<0)
                {
                     cont=0;
                     uartPutchar(uart_num,32);
                }
                else
                {
                    uartPutchar(uart_num,8);
                    uartPutchar(uart_num,32);
                    uartPutchar(uart_num,8);
                    uartPutchar(uart_num,32);
                }
            }
            else
            {
                str[cont]=c;
                cont++;
            }
        }
        uartPutchar(uart_num,c);
    }
}


int contar(char *str)
{
    int i,cont=0;

    for(i=0;str[i]!='\0';i++)
    {
        cont++;
    }
    return cont;
}

uint16_t myAtoi(char *str)
{
    int x=0,i,pos=1,tam=0;
    
    tam=contar(str); 

    for(i=0;i<tam-1;i++)
    {
        pos=pos*10; 
    }

    for(i=0;i<tam;i++)
    {
        if(str[i]>='0' && str[i]<='9')
        {
            x=x+(str[i]-48)*pos; 
            pos=pos/10;
        }
        else
        {
            x=x/(pos*10);
            break;
        }
    }

    return x;
}

void myItoa(uint16_t number, char* str, uint8_t base)
{
    uint16_t cociente=0,residuo=0;
    int tam,i=0,j=0;
    char c,c2;

    cociente=number;

    while(cociente!=0)
    {
        residuo=cociente%base; 
        cociente=(cociente/base);
        if(residuo==15 || residuo==14 || residuo==13 || residuo==12 || residuo==11 || residuo==10)
            str[i]=residuo+55; 
        else
            str[i]=residuo+48; 
        i++;
    }
    str[i]='\0';

    tam=contar(str);

    for(i=0;i<tam-1;i++)
    {
        for(j=i+1;j<tam;j++)
        {
            c=str[i]; 
            c2=str[j];
            str[i]=c2;
            str[j]=c;
        }
    }
}

void uartGotoxy(uart_port_t uart_num, uint8_t x, uint8_t y)
{
    char setX[10] = {0} ,setY[10] = {0};

    myItoa(x,setX,10);
    myItoa(y,setY,10);

    uartPuts(uart_num,"\e[");
    uartPuts(uart_num,setX);
    uartPuts(uart_num,";");
    uartPuts(uart_num,setY);
    uartPuts(uart_num,"H");
}