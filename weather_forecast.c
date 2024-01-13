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

// Function to generate a report based on analyzed data
void generateEnvironmentalReport(const struct WeatherDetails *weather, double averageTemperature)
{
    FILE *file = fopen(REPORT_FILE, "w");

    if (file)
    {
        fprintf(file, "Environmental Report\n");
        fprintf(file, "---------------------\n");
        fprintf(file, "Weather: %s\n", weather->description);
        fprintf(file, "Average Temperature (last 24 hours): %.2f Celsius\n", averageTemperature);
        fprintf(file, "Current Temperature: %.2f Celsius\n", weather->temperature);
        fprintf(file, "Humidity: %d%%\n", weather->humidity);

        struct tm *tm_info = localtime(&weather->timestamp);
        fprintf(file, "Time: %s\n", asctime(tm_info));

        fclose(file);
        printf("Environmental report generated and saved to '%s'\n", REPORT_FILE);
    }
    else
    {
        fprintf(stderr, "Failed to open file for writing\n");
    }
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL)
    {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void saveRawDataToFile(const char *filename, const char *data)
{
    FILE *file = fopen(filename, "w");
    if (file)
    {
        fprintf(file, "%s", data);
        fclose(file);
        printf("Raw data saved to '%s'\n", filename);
    }
    else
    {
        fprintf(stderr, "Failed to open file for writing\n");
    }
}

void displayWeatherDetails(const char *json, struct WeatherDetails *weather)
{
    cJSON *root = cJSON_Parse(json);

    if (root != NULL)
    {
        cJSON *main = cJSON_GetObjectItem(root, "main");
        cJSON *weatherJson = cJSON_GetArrayItem(cJSON_GetObjectItem(root, "weather"), 0);
        cJSON *description = cJSON_GetObjectItem(weatherJson, "description");
        cJSON *temperature = cJSON_GetObjectItem(main, "temp");
        cJSON *humidity = cJSON_GetObjectItem(main, "humidity");
        cJSON *timestamp = cJSON_GetObjectItem(root, "dt");

        if (description != NULL && temperature != NULL && humidity != NULL && timestamp != NULL)
        {
            snprintf(weather->description, sizeof(weather->description), "%s", description->valuestring);
            weather->temperature = temperature->valuedouble - 273.15;
            weather->humidity = humidity->valueint;
            weather->timestamp = timestamp->valueint;
        }
        else
        {
            printf("Error: Unable to retrieve weather details.\n");
        }

        cJSON_Delete(root);
    }
    else
    {
        printf("Error: Unable to parse JSON data.\n");
    }
}

void saveProcessedDataToFile(const char *filename, const struct WeatherDetails *weather)
{
    FILE *file = fopen(filename, "w");
    if (file)
    {
        fprintf(file, "Weather: %s\n", weather->description);
        fprintf(file, "Temperature: %.2f Celsius\n", weather->temperature);
        fprintf(file, "Humidity: %d%%\n", weather->humidity);

        struct tm *tm_info = localtime(&weather->timestamp);
        fprintf(file, "Time: %s", asctime(tm_info));

        fclose(file);
        printf("Processed data saved to '%s'\n", filename);
    }
    else
    {
        fprintf(stderr, "Failed to open file for writing\n");
    }
}

int detectAnomalies(double temperature)
{
    // Define anomaly thresholds
    const double highTemperatureThreshold = 40.0;
    const double lowTemperatureThreshold = -5.0;

    // Check for anomalies
    if (temperature > highTemperatureThreshold)
    {
        printf("Anomaly Detected: High Temperature (%.2f Celsius)\n", temperature);
        return 1; // Anomaly detected
    }
    else if (temperature < lowTemperatureThreshold)
    {
        printf("Anomaly Detected: Low Temperature (%.2f Celsius)\n", temperature);
        return 1; // Anomaly detected
    }

    return 0; // No anomaly detected
}

// Function to read data from the file for CURLOPT_READFUNCTION
size_t read_callback(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fread(ptr, size, nmemb, stream);
}

// Function to send email with attachment
int send_email_with_attachment(const char *to, const char *cc, const char *file_path)
{
    CURL *curl;
    CURLcode res = CURLE_OK;

    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        fprintf(stderr, "Error opening file: %s\n", file_path);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        struct curl_slist *recipients = NULL;

        // Specify email server details
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com");
        curl_easy_setopt(curl, CURLOPT_USERPWD, "badar4530946@cloud.neduet.edu.pk:Megahas123$");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

        // Add email recipients
        if (to)
            recipients = curl_slist_append(recipients, to);
        if (cc)
            recipients = curl_slist_append(recipients, cc);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // Set the read callback function to read the file content
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, file);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        // Perform the email sending
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        // Clean up
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }

    // Close the file
    fclose(file);

    curl_global_cleanup();
    return (int)res;
}

int main(void)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        struct MemoryStruct rawChunk;

        rawChunk.memory = malloc(1);
        rawChunk.size = 0;

        char request_url[256];
        snprintf(request_url, sizeof(request_url), "%s%s&appid=%s", API_URL, CITY_NAME, API_KEY);

        curl_easy_setopt(curl, CURLOPT_URL, request_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&rawChunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else
        {
            // Save raw data to file
            saveRawDataToFile("weather_raw_data.json", rawChunk.memory);

            struct WeatherDetails weather;
            displayWeatherDetails(rawChunk.memory, &weather);

            // Save temperature to history
            appendTemperatureToHistory(HISTORY_FILE, weather.temperature);

            // Print processed data to console
            printf("Processed data for %s:\n", CITY_NAME);
            printf("Weather: %s\n", weather.description);
            printf("Temperature: %.2f Celsius\n", weather.temperature);
            printf("Humidity: %d%%\n", weather.humidity);

            // Detect anomalies
            if (detectAnomalies(weather.temperature))
            {
                // Handle anomalies as needed (e.g., send alerts, log, etc.)
                printf("Anomalies detected. Handling as needed.\n");

                // Save processed data to file
                saveProcessedDataToFile("weather_processed_data.txt", &weather);

                // Analyze temperature history
                double average;
                analyzeTemperatureHistory(HISTORY_FILE, &average);

                // Generate environmental report
                generateEnvironmentalReport(&weather, average);

                // Example usage of send_email_with_attachment
                const char *recipient_email = "abulhassankk.174@gmail.com";
                const char *cc_email = "abulhassankk.174@gmail.com";
                const char *attachment_path = "environmental_report.txt";

                int result = send_email_with_attachment(recipient_email, cc_email, attachment_path);

                if (result == 0)
                {
                    printf("Email sent successfully!\n");
                }
                else
                {
                    printf("Failed to send email.\n");
                }
            }
            else
            {
                printf("No anomalies detected. Skipping email sending.\n");
            }

            struct tm *tm_info = localtime(&weather.timestamp);
            printf("Time: %s", asctime(tm_info));
        }

        // Free allocated memory
        free(rawChunk.memory);

        // Clean up
        curl_easy_cleanup(curl);
    }

    // Clean up global resources
    curl_global_cleanup();

    return 0;
}
