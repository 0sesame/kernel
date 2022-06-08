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
void initialize_uart_state(struct UartState *state);
char consume_byte(struct UartState *uart_state);
int producer_add_byte_to_buff(struct UartState *uart_state, char byte);
bool uart_buffer_empty(struct UartState *uart_state);