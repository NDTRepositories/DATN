#include "../Lib/datamanager.h"

__attribute__((unused)) static const char TAG[] = "Data_manager";
void encodeDataToHex(char *datauart,int id, float PH, float DO, float EC, float temperature, float latitude, float longitude)
{
    sprintf(datauart, "%08x%08x%08x%08x%08x%08x%08x", id, *(unsigned int *)&PH, *(unsigned int *)&DO, *(unsigned int *)&EC, *(unsigned int *)&temperature, *(unsigned int *)&latitude, *(unsigned int *)&longitude);
}

void decodeHexToData(const char *datauart, dataSensor_st *data)
{
    sscanf(datauart, "%08x%08x%08x%08x%08x%08x%08x", &data->id,(unsigned int *)&data->PH, (unsigned int *)&data->DO, (unsigned int *)&data->EC, (unsigned int *)&data->temperature, (unsigned int *)&data->latitude, (unsigned int *)&data->longitude);
}
float truncateDecimal(float num) 
{
    return trunc(num * 100) / 100;
}
