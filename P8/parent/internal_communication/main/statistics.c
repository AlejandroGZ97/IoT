#include "statistics.h"

float mean(){
    float mean=0;

    for (i = 0; i < SIZE; i++)
        mean += tempValues[i];
    
    return mean/SIZE;
}

float median(){
    uint8_t greater,smaller;

    for ( i = 0; i < SIZE; i++)
    {
        greater = 0;
        smaller = 0;
        for ( j = 0; j < SIZE; j++)
        {
            if (i==j)
                continue;
            
            if (tempValues[i]>tempValues[j])
                greater++;
            else if (tempValues[i]<tempValues[j])
                smaller++;
        }
        if (smaller < 3 && greater < 3)
        {
            return tempValues[i];
        }
    }
    return tempValues[i];
}

float variance(float mean){
    float sum = 0;
    for (i = 0; i < SIZE; i++)
    {
        sum += pow((tempValues[i]-mean),2);
    }
    return sum / (SIZE-1);
}