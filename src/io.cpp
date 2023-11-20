#include "Arduino.h"

int file_size(char* path) {
    FILE* file = NULL;
    file = fopen(path, "r");
    if (file == NULL)
    {
        printf("\nFailed to open file: %s\n", path);
        return 0;
    }

    // Determine the size of the file
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Close the file
    fclose(file);
    return size;
}

void read_file_data(char* path, int size, uint8_t* data) {
    FILE* file = NULL;
    file = fopen(path, "r");
    if (file == NULL)
    {
        printf("\nFailed to open file: %s\n", path);
        return;
    }

    if (fread(data, 1, size, file) != size)
    {
        printf("\nFailed to read file data\n");
        fclose(file);
        free(data);
        data = NULL;
        return;
    }
    
    fclose(file);
}