#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *hello_world(void *args)
{
    int a = *static_cast<int *>(args);
    printf("Hello from thread %d\n", a);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    const size_t count = strtol(argv[1], NULL, 10);
    for (size_t i = 0; i < count; i++)
    {
        pthread_t thread;
        if (pthread_create(&thread, NULL, hello_world, &i))
        {
            printf("Error create thread\n");
        }

        if (pthread_join(thread, NULL))
        {
            printf("Error joining thread\n");
        }
    }
}