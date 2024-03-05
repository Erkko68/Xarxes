/**
 * @file Controllers.h
 * @brief Functions for saving, loading, and managing clients for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

/*Define struct for controller info*/
struct ControllerInfo{
    unsigned char status;
    char situation[13];
    char elements[10][8];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
};

/*Define struct to load authorized clients*/
struct Controller{
    char name[9];
    char mac[13];
    struct ControllerInfo data;
};

/**
 * @brief Reads controller data from a file and dynamically allocates memory for each controller struct.
 *
 * This function reads controller data from the specified file 'filename' and dynamically allocates memory 
 * for each controller struct found in the file. It parses each line of the file, extracting the name and 
 * MAC address of each controller. Memory is allocated for each controller struct using realloc as they 
 * are encountered in the file. The total number of controllers read is returned.
 * 
 * @param controllers Pointer to a pointer to the Controller struct array where the controller data will be stored.
 * @param filename The name of the file to read controller data from.
 * @return Returns the total number of controllers read from the file on success.
 * 
 * @throw Error if no controller has been read, when memory allocation fails, or file descriptor fails.
 */
int initialiseControllers(struct Controller **controllers, const char *filename);