#include "comandos.h"

#define REG_SIZE 10

char cad[200] = {'\0'};
int cont = 0;
float meanTemp = 0, temp = 0, reg[REG_SIZE]={0};
uint8_t regCont = 0;

void showTemperature()
{
    char tempCad[30]={0};
     
    sprintf(tempCad,"Temperatura actual: %.2f",temp);
    strcpy(cad,tempCad);
}

void showRegisters()
{
    char regCad[200]={0}, auxCad[25]={0}, br[]="<br>";

    sprintf(regCad,"Menos reciente <br>");
    for (int i = 0; i < regCont; i++)
    {
        sprintf(auxCad,"%d)%.2f%s",i+1,reg[i],br);
        strcat(regCad, auxCad);
    }
    strcat(regCad,"Mas reciente");

    strcpy(cad,regCad);
}

void sortReg()
{
    for (int i = 0; i < REG_SIZE - 1; i++)
    {
        reg[i] = reg[i+1];
    }
}