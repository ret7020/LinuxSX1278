#ifndef GPIO_H
#define GPIO_H

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int exportPin(uint8_t pin)
{
    FILE *exportFile = fopen("/sys/class/gpio/export", "w");
    if (exportFile == NULL)
        return -1;
    fprintf(exportFile, "%d", pin);
    fclose(exportFile);

    return 0;
}

int setDirPin(uint8_t pin, const char *dir)
{
    char directionPath[50];
    snprintf(directionPath, sizeof(directionPath), "/sys/class/gpio/gpio%d/direction", pin);
    FILE *directionFile = fopen(directionPath, "w");
    if (directionFile == NULL)
        return -1;
    fprintf(directionFile, dir);
    fclose(directionFile);

    return 0;
}

int writePin(uint8_t pin, uint8_t value)
{
    char valuePath[50];
    snprintf(valuePath, sizeof(valuePath), "/sys/class/gpio/gpio%d/value", pin);
    FILE *valueFile = fopen(valuePath, "w");
    if (valueFile == NULL)
        return -1;

    // if 
    // printf("Setting pin: %d = %d\n", pin, value);
    if (value) fprintf(valueFile, "1");
    else fprintf(valueFile, "0");
    fflush(valueFile);
    fclose(valueFile);

    return 0;
}

int releasePin(uint8_t pin)
{
    FILE *unexportFile = fopen("/sys/class/gpio/unexport", "w");
    if (unexportFile == NULL)
        return -1;
    fprintf(unexportFile, "%d", pin);
    fclose(unexportFile);

    return 0;
}

int setPin(uint8_t pin, uint8_t value)
{
    return exportPin(pin) + setDirPin(pin, "out") + writePin(pin, value) + releasePin(pin);
}

#endif