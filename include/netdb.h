#ifndef NN_NETDB_H
#define NN_NETDB_H

#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hostent {
    char  *h_name;       
    char **h_aliases;   
    int    h_addrtype;  
    int    h_length;    
    char **h_addr_list; 
};
#define h_addr h_addr_list[0]

#define NETDB_INTERNAL  -1
#define NETDB_SUCCESS    0
#define HOST_NOT_FOUND   1
#define TRY_AGAIN        2
#define NO_RECOVERY      3
#define NO_DATA          4
#define NO_ADDRESS       NO_DATA

struct hostent *gethostbyname(const char *name);

extern int h_errno;

#ifdef __cplusplus
}
#endif

#endif /* NN_NETDB_H */
