#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define QSIZE 3

// Queue created as global array for easy of use and to control memory a bit more. 
// Could be linked list in struct.
uint32_t qArray[QSIZE];
uint32_t head = 0;
int32_t tail = -1;
uint32_t elementsInQueue = 0;

pthread_mutex_t lock;

bool GnAudioPut(uint32_t val)
{   
    // We assume that Put can be called from interrupt context
    // hence we will not block if mutex cannot be locked.
    if(pthread_mutex_trylock(&lock) != 0)
    {
        printf("trylock failed\n"); // Do not print from interrupt context ;)
        return false;
    }
    
    // Is queue full
    if (elementsInQueue == QSIZE)
    {
        pthread_mutex_unlock(&lock);
        return false;
    }
    
    tail = (tail + 1) % QSIZE;
    qArray[tail] = val;
    elementsInQueue++;
    
    pthread_mutex_unlock(&lock);
    
    return true;
}

bool GnAudioGet(uint32_t *val)
{
    // It is assumed that Get will not be called
    // from interrupt context, hence this is allowed to block.
    pthread_mutex_lock(&lock);
    
    // Is queue empty
    if (elementsInQueue == 0)
    {
        pthread_mutex_unlock(&lock);
        return false;
    }
    
    *val = qArray[head];
    head = (head + 1) % QSIZE;
    elementsInQueue--;
    
    pthread_mutex_unlock(&lock);
    
    return true;
}

// **************************
// Testing

pthread_t writerThread;
pthread_t readerThread;

void *writerTask(void* arg)
{
    uint32_t testValues[] = {1,2,3,4,5,6,7,8,9};
    int testValueIndex = 0;
    
    int writeTryCounter = 0;
    int failCount = 0;
    
    while (writeTryCounter < 10000)
    {
        if (GnAudioPut(testValues[testValueIndex]))
        {
            // printf("Put OK (fail count %d)\n", failCount);
            failCount = 0;
        }
        else
        {
            failCount++;
        }
        
        testValueIndex = (testValueIndex + 1) % 9;
        writeTryCounter++;
        
        for (int i = 0; i < 5000; i++)
            ;
    }
}

void *readerTask(void *arg)
{
    uint32_t value;
    
    int readTryCounter = 0;
    int failCount = 0;
    
    while (readTryCounter < 10000)
    {
        if(GnAudioGet(&value))
        {
            // printf("Value read %u (fail count %d)\n", value, failCount);
            failCount = 0;
        }
        else
        {
            failCount++;
        }
        
        readTryCounter++;
        
        for (int i = 0; i < 5000; i++)
            ;
    }
    
}

int main()
{
    if (pthread_mutex_init(&lock, NULL) != 0) 
    {
        printf("Mutex init failed\n");
        return 1;
    }
    
    if (pthread_create(&writerThread, NULL, &writerTask, NULL) != 0)
        printf("Writer thread cannot be created\n");
    
    if (pthread_create(&readerThread, NULL, &readerTask, NULL) != 0)
        printf("Reader thread cannot be created\n");
    
    pthread_join(writerThread, NULL);
    pthread_join(readerThread, NULL);
    pthread_mutex_destroy(&lock);
    
    return 0;
}
