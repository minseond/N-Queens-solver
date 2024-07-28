#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "stack.h"

#ifndef BOARD_SIZE
#define BOARD_SIZE 15
#endif

typedef struct {
    pthread_cond_t queue_cv;
    pthread_cond_t dequeue_cv;
    pthread_mutex_t lock;
    char **elem;
    int capacity;
    int num;
    int front;
    int rear;
} bounded_buffer;

void bounded_buffer_init(bounded_buffer *buf, int capacity) {
    pthread_cond_init(&(buf->queue_cv), 0x0);
    pthread_cond_init(&(buf->dequeue_cv), 0x0);
    pthread_mutex_init(&(buf->lock), 0x0);
    buf->capacity = capacity;
    buf->elem = (char **)calloc(sizeof(char *), capacity);
    buf->num = 0;
    buf->front = 0;
    buf->rear = 0;
}

void bounded_buffer_queue(bounded_buffer *buf, char *msg) {
    pthread_mutex_lock(&(buf->lock));
    while (!(buf->num < buf->capacity)) {
        pthread_cond_wait(&(buf->queue_cv), &(buf->lock));
    }

    buf->elem[buf->rear] = msg;
    buf->rear = (buf->rear + 1) % buf->capacity;
    buf->num += 1;

    pthread_cond_signal(&(buf->dequeue_cv));
    pthread_mutex_unlock(&(buf->lock));
}

char *bounded_buffer_dequeue(bounded_buffer *buf) {
    char *r = 0x0;

    pthread_mutex_lock(&(buf->lock));
    while (!(0 < buf->num)) {
        pthread_cond_wait(&(buf->dequeue_cv), &(buf->lock));
    }

    r = buf->elem[buf->front];
    buf->elem[buf->front] = NULL; // 디큐 후 슬롯 비우기
    buf->front = (buf->front + 1) % buf->capacity;
    buf->num -= 1;

    pthread_cond_signal(&(buf->queue_cv));
    pthread_mutex_unlock(&(buf->lock));

    return r;
}

void bounded_buffer_destroy(bounded_buffer *buf) {
    free(buf->elem);
    pthread_mutex_destroy(&(buf->lock));
    pthread_cond_destroy(&(buf->queue_cv));
    pthread_cond_destroy(&(buf->dequeue_cv));
}

typedef struct {
    struct stack_t *stack;
    int n;
} thread_arg_t;

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_solutions = 0;
int num_threads = 1;
bounded_buffer buf;

void handle_sigint(int sig) {
    pthread_mutex_lock(&count_mutex);
    printf("\nTotal solutions found: %d\n", total_solutions);
    pthread_mutex_unlock(&count_mutex);
    bounded_buffer_destroy(&buf);
    exit(0);
}

int row(int cell) {
    return cell / BOARD_SIZE;
}

int col(int cell) {
    return cell % BOARD_SIZE;
}

int is_feasible(struct stack_t *queens) {
    int board[BOARD_SIZE][BOARD_SIZE];
    int c, r;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            board[r][c] = 0;
        }
    }

    for (int i = 0; i < get_size(queens); i++) {
        int cell;
        get_elem(queens, i, &cell);

        int r = row(cell);
        int c = col(cell);

        if (board[r][c] != 0) {
            return 0;
        }

        for (int y = 0; y < BOARD_SIZE; y++) {
            board[y][c] = 1;
        }
        for (int x = 0; x < BOARD_SIZE; x++) {
            board[r][x] = 1;
        }

        for (int y = r + 1, x = c + 1; y < BOARD_SIZE && x < BOARD_SIZE; y++, x++) {
            board[y][x] = 1;
        }
        for (int y = r + 1, x = c - 1; y < BOARD_SIZE && x >= 0; y++, x--) {
            board[y][x] = 1;
        }
        for (int y = r - 1, x = c + 1; y >= 0 && x < BOARD_SIZE; y--, x++) {
            board[y][x] = 1;
        }
        for (int y = r - 1, x = c - 1; y >= 0 && x >= 0; y--, x--) {
            board[y][x] = 1;
        }
    }

    return 1;
}

void print_placement(struct stack_t *queens) {
    char msg[256];
    int offset = 0;
    for (int i = 0; i < get_size(queens); i++) {
        int queen;
        get_elem(queens, i, &queen);
        offset += snprintf(msg + offset, sizeof(msg) - offset, "[%d,%d] ", row(queen), col(queen));
    }
    bounded_buffer_queue(&buf, strdup(msg));
}

void *producer(void *arg) {
    thread_arg_t *args = (thread_arg_t *)arg;
    struct stack_t *queens = create_stack(BOARD_SIZE);

    queens->capacity = args->stack->capacity;
    queens->size = args->stack->size;
    memcpy(queens->buffer, args->stack->buffer, args->stack->size * sizeof(int));

    while (args->stack->size <= queens->size) {
        int latest_queen;
        top(queens, &latest_queen);

        if (latest_queen == BOARD_SIZE * BOARD_SIZE) {
            pop(queens, &latest_queen);
            if (!is_empty(queens)) {
                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            } else {
                break;
            }
            continue;
        }

        if (is_feasible(queens)) {
            if (get_size(queens) == args->n) {
                print_placement(queens);
                pthread_mutex_lock(&count_mutex);
                total_solutions++;
                pthread_mutex_unlock(&count_mutex);

                int latest_queen;
                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            } else {
                int latest_queen;
                top(queens, &latest_queen);
                push(queens, latest_queen + 1);
            }
        } else {
            int latest_queen;
            pop(queens, &latest_queen);
            push(queens, latest_queen + 1);
        }
    }

    delete_stack(queens);
    return NULL;
}

void *consumer(void *arg) {
    while (1) {
        char *msg = bounded_buffer_dequeue(&buf);
        if (msg) {
            printf("%s\n", msg);
            free(msg);
        }
    }
    return NULL;
}

void find_n_queens(int N) {
    pthread_t prod_threads[num_threads];
    pthread_t cons_thread;
    thread_arg_t args[num_threads];
    struct stack_t *initial_stack = create_stack(BOARD_SIZE);
    push(initial_stack, 0);

    bounded_buffer_init(&buf, 10);

    for (int i = 0; i < num_threads; i++) {
        args[i].stack = initial_stack;
        args[i].n = N;
        pthread_create(&prod_threads[i], NULL, producer, &args[i]);
    }

    pthread_create(&cons_thread, NULL, consumer, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(prod_threads[i], NULL);
    }

    pthread_cancel(cons_thread);
    pthread_join(cons_thread, NULL);

    delete_stack(initial_stack);
    bounded_buffer_destroy(&buf);
}

void find_n_queens_with_prepositions(int N, struct stack_t *prep) {
    pthread_t prod_threads[num_threads];
    pthread_t cons_thread;
    thread_arg_t args[num_threads];

    bounded_buffer_init(&buf, 10);

    for (int i = 0; i < num_threads; i++) {
        args[i].stack = prep;
        args[i].n = N;
        pthread_create(&prod_threads[i], NULL, producer, &args[i]);
    }

    pthread_create(&cons_thread, NULL, consumer, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(prod_threads[i], NULL);
    }

    pthread_cancel(cons_thread);
    pthread_join(cons_thread, NULL);

    bounded_buffer_destroy(&buf);
}

int main(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-t num_threads]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, handle_sigint);

    find_n_queens(4);

    return EXIT_SUCCESS;
} 