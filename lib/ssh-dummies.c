__attribute__((weak)) void _start() {
    asm("mov $60, %rax");  // Системный вызов exit
    asm("mov $0, %rdi");   // Передача аргумента 0 для exit
    asm("syscall");        // Вызов системного вызова
}

int __jos_errno_loc;