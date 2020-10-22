#include <stdio.h>
#include<sys/mman.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define A 248
#define B 0x936F88A5
#define C mmap
#define D 131
#define E 170
#define F block
#define G 112
#define H random
#define I 29
#define J sum
#define K cv

typedef struct{
    unsigned char * start;
    size_t count;
    FILE * urandom;
} wrn_args;

typedef struct {
    unsigned int file_number;
    pthread_mutex_t * mutex;
    pthread_cond_t * cv;
} ras_args;

void* write_random_numbers(void * wrn_data){
    wrn_args *data = (wrn_args*) wrn_data;
    for (size_t i = 0; i < data->count; ) {
        i += fread(data->start + i, 1, data->count - i, data->urandom);
    }
    return NULL;
}

void generate_and_write(unsigned char *src,
                        unsigned int file_number,
                        pthread_mutex_t * mutex,
                        pthread_cond_t * cv){

    FILE * urandom = fopen("/dev/urandom", "rb");
    size_t wrn_part =  A * 1024 * 1024 / D;
    unsigned char * start = src;
    pthread_t wrn_threads[D];
    for (int i = 0; i<D-1; i++){
        wrn_args wrn_data = {start, wrn_part, urandom};
        pthread_create(&(wrn_threads[i]), NULL, write_random_numbers, &wrn_data);
        start+=wrn_part;
    }

    wrn_args wrn_data = {start, wrn_part + A * 1024 * 1024 % D, urandom};
    pthread_create(&(wrn_threads[D-1]), NULL, write_random_numbers, &wrn_data);
    for(int i = 0; i < D; i++)
        pthread_join(wrn_threads[i], NULL);

    fclose(urandom);
    pthread_mutex_lock(mutex);
    for (int i = 0; i < file_number; i++){
        char result_name[13];
        snprintf(result_name,13, "lab_os_%d.bin", i);
        FILE * file = fopen(result_name, "wb");
        size_t file_size = E*1024*1024;
        for (int j = 0; j < file_size; ){
            unsigned int block = rand() % (A * 1024 * 1024 / G);
            unsigned int block_size = file_size - j < G ? file_size - j < G : G;
            j += fwrite( src + block*G, 1, block_size, file);
        }
        fclose(file);
        printf("%d data_generated\n", i);
    }
    pthread_cond_broadcast(cv);
    pthread_mutex_unlock(mutex);
}

int convert_char_buf_2_int(unsigned char buf[]){
    int sum = 0;
    for (int i = 0; i < G / sizeof (int); i+=sizeof (int)){
        int num = 0;
        for (int j = 0; j < sizeof (int); j++){
            num = (num<<8) + buf[i+j];
        }
        sum += num;
    }
    return sum;
}

_Noreturn void* read_and_sum(void * ras_data){
    ras_args * args = (ras_args*) ras_data;
    char filename[13];
    snprintf(filename,  13, "lab_os_%d.bin", args->file_number);
    while(1){
        long long sum = 0;
        pthread_mutex_lock(args->mutex);
        printf("%ld wait on cond\n", pthread_self());
        pthread_cond_wait(args->cv, args->mutex);
        printf("%ld waited\n", pthread_self());
        FILE *file = fopen(filename, "rb");
        unsigned char buf[G];
        for (unsigned int i = 0; i < 2*E*1024*1024/G; i++){
            unsigned int block = rand() % (2*E*1024*1024/G);
            fseek(file, block*G, 0);
            if (fread(&buf, 1, G, file) != G) continue;
            else sum+=convert_char_buf_2_int(buf);
        }
        printf("%lld\n", sum);
        fclose(file);
        pthread_mutex_unlock(args->mutex);
    }
}

unsigned char* allocate_memory(){
    return C((void *)B, A*1024*1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

int main() {
    unsigned char *src;
    printf("Before allocation");
    getchar();
    src = allocate_memory();
    if ( src == MAP_FAILED ){
        printf("Error: can't allocate to the address");
        return 1;
    }
    printf("After allocation");
    getchar();

    unsigned int file_number = A / E + (A % E > 0 ? 1 : 0);

    pthread_mutex_t mutex;
    pthread_cond_t cv;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cv, NULL);

    generate_and_write(src, file_number, &mutex, &cv);
    printf("After filling");
    getchar();
    munmap(src, A*1024*1024);
    printf("After deallocation");
    getchar();

    src = allocate_memory();
    if ( src == MAP_FAILED ){
        printf("Error: can't allocate to the address");
        return 1;
    }

    pthread_t ras_threads[I];

    for (int i = 0; i < I; i++){
        int file_count = i / (I / file_number);
        if (file_count == file_number) file_count--;
        ras_args ras_data = {file_count, &mutex, &cv};
        pthread_create(&ras_threads[i], NULL, read_and_sum, &ras_data);
    }

    while(1)
        generate_and_write(src, file_number, &mutex, &cv);

    for (int i = 0; i < I; i++)
        pthread_join( ras_threads[0], NULL);

    return 0;
}
