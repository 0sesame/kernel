void IRQ_handle_keyboard(int, int, void *);
void IRQ_handle_timeout(int interrupt_number, int error, void *args);
void IRQ_handle_div0(int interrupt_number, int error, void *args);
void IRQ_page_fault(int interrupt_number, int error, void *args);
void IRQ_yield(int interrupt_number, int error, void *args);
void IRQ_exit(int interrupt_number, int error, void *args);