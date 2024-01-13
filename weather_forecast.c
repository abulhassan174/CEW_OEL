#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cJSON.h>
#include <time.h>
#include <math.h>
#include "email_sender.h"

#define API_KEY "b94486085331eff2b6fc6316d8951cf2"
#define CITY_NAME "karachi"
#define API_URL "http://api.openweathermap.org/data/2.5/weather?q="
#define HISTORY_FILE "temperature_history.txt"
#define REPORT_FILE "environmental_report.txt"

struct MemoryStruct
{
    char *memory;
    size_t size;
};

struct WeatherDetails
{
    char description[50];
    double temperature;
    int humidity;
    time_t timestamp;
};

void appendTemperatureToHistory(const char *filename, double temperature)
{
    FILE *file = fopen(filename, "a");
    if (file)
    {
        fprintf(file, "%.2f\n", temperature);
        fclose(file);
    }
    else
    {
        fprintf(stderr, "Failed to open file for writing\n");
    }
}

void analyzeTemperatureHistory(const char *filename, double *average)
{
    FILE *file = fopen(filename, "r");
    if (file)
    {
        double temperature;
        double sum = 0.0;
        int count = 0;

        while (fscanf(file, "%lf", &temperature) == 1)
        {
            sum += temperature;
            count++;
        }

        if (count > 0)
        {
            *average = sum / count;
        }
        else
        {
            *average = 0.0;
        }

        fclose(file);
    }
    else
    {
        fprintf(stderr, "Failed to open file for reading\n");
    }
}
