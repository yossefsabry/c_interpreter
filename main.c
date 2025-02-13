#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>  // For open()
#include <unistd.h> // For close(), read(), write()
#include <stdio.h>  // For perror()
#include <string.h>

#define int long long // work with 64bit target

int token;            // current token
char *src, *old_src;  // pointer to source code string;
int poolsize;         // default size of text/data/stack
int line;             // line number
int *text,            // text segment (store code instruction)
    *old_text,        // for dump text segment
    *stack;           // stack
char *data;           // data segment
// registers using to store running state values for computer or VM

/* 
    - PC: program counter, it stores an memory address in which stores the 
        next instruction to be run.
    - SP: stack pointer, which always points to the top of the stack.
        Notice the stack grows from high address to low address so that 
        when we push a new element to the stack, SP decreases.
    - BP: base pointer, points to some elements on the stack.
        It is used in function calls.
    - AX: a general register that we used to store the result of an instruction.
for more information check this Url
https://www.tutorialspoint.com/assembly_programming/assembly_registers.htm
*/
int *pc, *bp, *sp, ax, cycle; // virtual machine registers

// instructions
enum { LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  
        ,PUSH, OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD 
        ,SUB ,MUL ,DIV ,MOD , OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };

// setup func
void next();
void expression(int level);
void program();
int eval();

#undef int // Mac/clang needs this

int main(int argc, char **argv){
    #define int long long // work with 64bit target

    // fd => file pointer, i => number of bytes for reading from file
    int i, fd;

    // for ignore the name of program and get the first passing param
    argc--;
    argv++;

    poolsize = 256 * 1024; // arbitrary size (size reading from file)
    line = 1;

    /*
     open(*argv, 0) open the first arg in execution and O_RDONLY => 0 
     mean that the file is read only (RD ONLY)
     if this return something lesser than 0 this mean erorr happend
     */
    if ((fd = open(*argv, O_RDONLY)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    /*
     check if the malloc work correctly  this mean if malloc work and 
     gerernate array and genrate a address for first element
     then the code wan't run inside the if statement else gone run code inside
     if statement 
     and if malloc work store the address in src and old_src
     malloc in heap
    */
    if (!(src = old_src = malloc(poolsize))) {
        printf("could not malloc(%lld) for source area\n", poolsize);
        return -1;
    }

    /*
     read the source file:
     read(file pointer, place for save bytes from file, number of bytes)
     if read don't work correct then return -1 so handle this with this 
     if condiition
    */
    if ((i = read(fd, src, poolsize-1)) <= 0) {
        printf("read() returned %lld\n", i);
        return -1;
    }

    src[i] = 0; // add EOF character for end of file
    close(fd); // closing a file

    /*
     allocate memory for virtual machine and saving the new address in 
     text adn old text if error happend return NULL then handle with this
     if condition
     malloc in heap
    */
    if (!(text = old_text = malloc(poolsize))) {
        printf("could not malloc(%lld) for text area\n", poolsize);
        return -1;
    }

    // malloc size in heap for data (code statment) and handle if error happend
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%lld) for data area\n", poolsize);
        return -1;
    }

    // malloc size for stack and handle if error happend
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%lld) for stack area\n", poolsize);
        return -1;
    }

    // starting with setup the memory with zeros value
    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);

    bp = sp = (int *)((int)stack + poolsize); // setup the stack
    ax = 0; // for saving temp data


    i = 0;
    text[i++] = IMM;
    text[i++] = 10;
    text[i++] = PUSH;
    text[i++] = IMM;
    text[i++] = 20;
    text[i++] = ADD;
    text[i++] = PUSH;
    text[i++] = EXIT;

    pc = text;

    program();

    return eval();
}

void next() {
    token = *src++;
    return;
}

void expression(int level) {
    // do nothing
}

void program() {
    next(); // get next token
    while (token > 0) {
        printf("token is: %lld\n", token);
        next();
    }
}


int eval() {
    int op, *tmp;
    while (1) {
        op = *pc++; // get next operation code

        // load immediate value to ax
        if (op == IMM)       {ax = *pc++;} 
        // load character to ax, address in ax
        else if (op == LC)   {ax = *(char *)ax;} 
        // load integer to ax, address in ax
        else if (op == LI)   {ax = *(int *)ax;}
        // save character to address, value in ax, address on stack
        else if (op == SC)   {ax = *(char *)*sp++ = ax;}
        // save integer to address, value in ax, address on stack
        else if (op == SI)   {*(int *)*sp++ = ax;}
        // push the value of ax onto the stack
        else if (op == PUSH) {*--sp = ax;}
        // jump to the address
        else if (op == JMP)  {pc = (int *)*pc;} 
        // jump if ax is zero
        else if (op == JZ)   {pc = ax ? pc + 1 : (int *)*pc;} 
        // jump if ax is zero
        else if (op == JNZ)  {pc = ax ? (int *)*pc : pc + 1;} 
        // call subroutine
        else if (op == CALL) {*--sp = (int)(pc+1); pc = (int *)*pc;} 
        //else if (op == RET)  {pc = (int *)*sp++;} // return from subroutine;
        // make new stack frame
        else if (op == ENT)  {*--sp = (int)bp; bp = sp; sp = sp - *pc++;} 
        // add esp, <size>
        else if (op == ADJ)  {sp = sp + *pc++;} 
        // restore call frame and PC
        else if (op == LEV)  {sp = bp; bp = (int *)*sp++; pc = (int *)*sp++;} 
        // make new stack frame
        else if (op == ENT)  {*--sp = (int)bp; bp = sp; sp = sp - *pc++;} 
        // add esp, <size>
        else if (op == ADJ)  {sp = sp + *pc++;} 
        // restore call frame and PC
        else if (op == LEV)  {sp = bp; bp = (int *)*sp++; pc = (int *)*sp++;} 
        // load address for arguments.
        else if (op == LEA)  {ax = (int)(bp + *pc++);} 

        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;


        else if (op == EXIT) { printf("exit(%lld)", *sp); return *sp;}
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
        else if (op == PRTF) { 
            tmp = sp + pc[1]; 
            ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); 
        }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
        else {
            printf("unknown instruction:%lld\n", op);
            return -1;
        }
    }
    return 0;
}

