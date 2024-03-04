#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logs.h"
#include "controllers.h"

/*Function to read from file and populate controllers array*/
int readControllersFromFile(struct Controller *controllers, const char *filename) {
    int numControllers = 0;
    char line[50]; 
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }
    while (fgets(line, sizeof(line), file) != NULL && numControllers < 2) {
        
        char id[1];
        char key[1];
        if (sscanf(line, "CTRL-%9[^,],%12s", id, key) == 2) {
            
            strncpy(controllers[numControllers].name, id, 3);
            strncpy(controllers[numControllers].mac, key, 5);
            numControllers++;
        }
    }

    fclose(file);
    return numControllers;
}