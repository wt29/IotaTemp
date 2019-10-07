#ifndef samplePower_h
#define samplePower_h

void    samplePower(int temp, int humidity);
int     sampleCycle(IotaInputChannel* tchannel );
// int     sampleCycle(IotaInputChannel* tchannel, IotaInputChannel* Ichannel, int cycles, int overSamples);
// float   getAref(int channel);
// int     readADC(uint8_t channel);
// float   sampleTemperature(uint8_t Vchan, float Vcal);
// String  samplePhase(uint8_t Vchan, uint8_t Ichan, uint16_t Ishift);
void    printSamples();

#endif