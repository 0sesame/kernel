#include <stdbool.h>
#define BUFF_SIZE 128

struct UartState{
    char buff[BUFF_SIZE];
    char *consumer, *producer;
    char *end_of_buff;
    bool busy;
};
int SER_write(char *byte, int len);
void SER_init(void);