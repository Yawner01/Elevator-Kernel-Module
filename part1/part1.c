int main() {
    asm("mov $39, %rax\n\t" "syscall");
    asm("mov $39, %rax\n\t" "syscall");
    asm("mov $39, %rax\n\t" "syscall");
    asm("mov $39, %rax\n\t" "syscall");

    return 0;
}