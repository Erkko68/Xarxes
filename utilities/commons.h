/**
 * @file commons.h
 * @brief Common header file including essential system libraries and project-specific headers.
 * 
 * 
 * This header file serves as a central location to include commonly used libraries and
 * project-specific headers, reducing redundancy and ensuring consistency across multiple
 * source files within the project.
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

#ifndef COMMONS_H
#define COMMONS_H

/*System Standard Libraries*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include <time.h>

#include <pthread.h>

#include <arpa/inet.h>
#include <sys/select.h>

/*Own Libraries*/
#include "pdu/udp.h"
#include "pdu/tcp.h"
#include "server/controllers.h"
#include "server/conf.h"
#include "server/subs_process.h"
#include "server/commands.h"
#include "server/data.h"
#include "logs.h"


#endif /* COMMONS_H */
