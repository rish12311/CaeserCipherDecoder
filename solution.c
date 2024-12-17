#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

int max_string_size = 0;
int current_fact = 0;
int sum = 0;
pthread_mutex_t f_sum = PTHREAD_MUTEX_INITIALIZER;

struct thread_val {
    int id;          // For thread finish check
    char *word;      // Matrix element (word)
    char *file_cont; // Word file content
    int rfactor;     // Rotate factor
};

struct message {
    long mtype;
    int key;
};

struct caeser_thread_val {
    char *word;      // Word to rotate
    int rfactor;     // Rotate factor (Caesar cipher)
};

// Thread function to perform Caesar cipher rotation
void *caesar_cipher(void *values) {
    struct caeser_thread_val *val = (struct caeser_thread_val *)values;
    int fact = val->rfactor;

    // Rotate the characters of `val->word` using Caesar cipher with `fact`
    for (int i = 0; val->word[i] != '\0'; i++) {
        if (val->word[i] >= 'a' && val->word[i] <= 'z') {
            val->word[i] = ((val->word[i] - 'a' + fact + 26) % 26) + 'a';
        }
    }

    pthread_exit(NULL);
}

// Thread function to count occurrences of a rotated word
void *count_seq(void *values) {
    struct thread_val *val = (struct thread_val *)values;

    // Create a thread to handle Caesar cipher rotation
    pthread_t cipher_thread;
    struct caeser_thread_val cipher_values;
    cipher_values.word = val->word;
    cipher_values.rfactor = val->rfactor;

    pthread_create(&cipher_thread, NULL, caesar_cipher, &cipher_values);
    pthread_join(cipher_thread, NULL);

    int count = 0;

    // Search for occurrences of `val->word` in `val->file_cont`
    char *ptr = val->file_cont;
    while ((ptr = strstr(ptr, val->word)) != NULL) {
        // Ensure the match is a whole word and not a substring
        if ((ptr == val->file_cont || !isalpha(ptr[-1])) && !isalpha(ptr[strlen(val->word)])) {
            count++;
        }
        ptr += strlen(val->word); // Move the pointer to search for further occurrences
    }

    // Lock the mutex and update the global sum
    pthread_mutex_lock(&f_sum);
    sum += count;
    pthread_mutex_unlock(&f_sum);

    pthread_exit(NULL);
}

// Function to communicate with the message queue
int queue(int msgid, int a) {
    struct message msg;
    msg.mtype = 1;
    msg.key = a;

    if (msgsnd(msgid, &msg, sizeof(msg.mtype), 0) == -1) {
        perror("Error with message send");
        exit(-8);
    }

    if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(msg.mtype), 2, 0) == -1) {
        perror("Error with receiving message");
        exit(-9);
    }
    return msg.key % 26;
}

int main(int args, char *argv[]) {
    if (args != 2) {
        perror("Issue with number of expected command-line arguments");
        exit(-2);
    }
    int t = atoi(argv[1]);
    if (t >= 100) {
        perror("t is a 3 or higher digit number");
        exit(-3);
    }

    char ip_file_name[30];
    snprintf(ip_file_name, sizeof(ip_file_name), "input%d.txt", t);
    char word_filename[30];
    snprintf(word_filename, sizeof(word_filename), "words%d.txt", t);

    FILE *ip_file = fopen(ip_file_name, "r");
    if (!ip_file) {
        perror("Error opening input file");
        exit(-4);
    }

    int N, stringSize, shmkey, messkey;
    fscanf(ip_file, "%d %d %d %d", &N, &stringSize, &shmkey, &messkey);
    fclose(ip_file);

    max_string_size = stringSize;

    int mat_shm = shmget(shmkey, sizeof(char[N][N][stringSize]), IPC_CREAT | 0666);
    if (mat_shm < 0) {
        perror("Error in shmget");
        exit(-5);
    }

    char (*matrix)[N][stringSize] = shmat(mat_shm, NULL, 0);
    if (matrix == (void *)-1) {
        perror("Error in shared memory attaching in matrix");
        exit(-6);
    }

    FILE *file = fopen(word_filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(-10);
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    char *file_contents = (char *)malloc((file_size + 1) * sizeof(char));
    if (file_contents == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        exit(-11);
    }
    size_t read_size = fread(file_contents, sizeof(char), file_size, file);
    if (read_size != file_size) {
        perror("Error reading file");
        free(file_contents);
        fclose(file);
        exit(-12);
    }
    file_contents[file_size] = '\0';
    fclose(file);

    int msgid = msgget(messkey, 0666);
    if (msgid == -1) {
        perror("Error in msgget");
        exit(-13);
    }

    for (int k = 0; k <= (2 * N - 2); k++) {
        //printf("K= %d\n", k);

        if (k != 0) {
            current_fact = queue(msgid, sum);
        }
        //printf("sum = %d\n", sum);
        sum = 0;
        int thread_count = 0;
        pthread_t thread[N];
        struct thread_val *values[N];

        if (k <= N - 1) { // Accessing upper diagonals
            for (int i = 0; i <= k; i++) {
                int j = k - i;
                //printf("matrix[%d][%d]: %s\n", i, j, matrix[i][j]);
                
                // Allocate memory for each thread_val struct before using it
                values[thread_count] = (struct thread_val *)malloc(sizeof(struct thread_val));
                if (values[thread_count] == NULL) {
                    perror("Memory allocation failed");
                    exit(-14);
                }

                //printf("Threads count = %d\n", thread_count);
                values[thread_count]->id = thread_count;
                values[thread_count]->word = matrix[i][j];
                values[thread_count]->file_cont = file_contents;
                values[thread_count]->rfactor = current_fact;

                if (pthread_create(&thread[thread_count], NULL, count_seq, values[thread_count]) != 0) {
                    perror("Error in creating thread");
                   exit(-15);
                }
                thread_count++;
                //printf("matrix[%d][%d]: %s\n", i, j, matrix[i][j]);
            }
        } else { // Accessing lower diagonals
            for (int i = N - 1; i >= k - N + 1; i--) {
                int j = k - i;

                // Allocate memory for each thread_val struct before using it
                values[thread_count] = (struct thread_val *)malloc(sizeof(struct thread_val));
                if (values[thread_count] == NULL) {
                    perror("Memory allocation failed");
                    exit(-16);
                }
                //printf("Threads count lower = %d\n", thread_count);
                values[thread_count]->id = thread_count;
                values[thread_count]->word = matrix[i][j];
                values[thread_count]->file_cont = file_contents;
                values[thread_count]->rfactor = current_fact;

                if (pthread_create(&thread[thread_count], NULL, count_seq, values[thread_count]) != 0) {
                    perror("Error in creating thread");
                    exit(-17);
                }
                thread_count++;
            }
        }

        // Join threads and free the allocated memory
        for (int b = 0; b < thread_count; b++) {
            pthread_join(thread[b], NULL);
            free(values[b]); 
            //printf("Threads destroyed = %d\n", b); // Free the memory after the thread finishes
        }
    }

    int x = queue(msgid, sum);
    free(file_contents);
    return 0;
}
