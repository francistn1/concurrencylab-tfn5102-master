#include "channel.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
channel_t* channel_create(size_t size)
{   //Create channel object, buffer and initilaze mutex
    channel_t* channel = (channel_t*)malloc(sizeof(channel_t));
    channel->buffer = buffer_create(size);
    channel->size = size;
    channel->status = 0;
    channel->sem_select = NULL;
    //channel->chan_data = NULL;
    //list_node_t* node = channel->list->head->data;
    channel->list = list_create(); // may or may not need, currently causing problems with the valgrind tests
    sem_init(&channel->sem_send, 0, (unsigned int)size);
    sem_init(&channel->sem_receive, 0, 0);
    pthread_mutex_init(&(channel->mutex), NULL);
    //in case memory allocation fails
    if(channel == NULL)
    {
        return NULL;
    }
    return channel;
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{
    //sem_post(&channel->sem_select);
    if(channel == NULL)//channel shouldn't have nothing
    {
        return GEN_ERROR;
    }
    //Need mutex in case of multiple thread access
    pthread_mutex_lock(&channel->mutex); // need to lock then unlock later for single thread access
    
    if(channel->status == -2)// check if channel is closed
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    pthread_mutex_unlock(&channel->mutex);

    sem_wait(&channel->sem_send);// need to wait
    
    pthread_mutex_lock(&channel->mutex);// lock before adding to share memory
    if(channel->status == -2)// check if currenct channel is closed
    {
        //chain effect
        pthread_mutex_unlock(&channel->mutex);
        sem_post(&channel->sem_send);//need to wait up every other thread 
        //sem_post(&channel->sem_select);
        return CLOSED_ERROR;
    }
    buffer_add(channel->buffer, data);
    sem_post(&channel->sem_receive);//increment recieve to keep track of the number of messages
    //sem_post(&channel->sem_select);
    if(channel->sem_select != NULL)
    {
       list_node_t* node = channel->list->head;
       while(node) {
            sem_post(node->data);
            node = node->next;
            }
        
    }
    pthread_mutex_unlock(&channel->mutex);//unlock after
    return SUCCESS;

}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{
    //sem_post(&channel->sem_select);
    if(channel == NULL)//channel shouldn't have nothing
    {
        return GEN_ERROR;
    }
    //Need mutex in case of multiple thread access
    pthread_mutex_lock(&channel->mutex); // need to lock then unlock later for single thread access
    
    if(channel->status == -2)// check if channel is closed
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    pthread_mutex_unlock(&channel->mutex);

    sem_wait(&channel->sem_receive);// need to wait
    
    pthread_mutex_lock(&channel->mutex);// lock before adding to share memory
    if(channel->status == -2)// check if currenct channel is closed
    {
        //chain effect
        pthread_mutex_unlock(&channel->mutex);
        sem_post(&channel->sem_receive);
        //sem_post(&channel->sem_select);
        return CLOSED_ERROR;
    }
    buffer_remove(channel->buffer, data);
    sem_post(&channel->sem_send);// keep track of open slots
    //sem_post(*&channel->sem_select);
    if(channel->sem_select != NULL)
    {
       list_node_t* node = channel->list->head;
       while(node) {
            sem_post(node->data);
            node = node->next;
            }
        
    }
    pthread_mutex_unlock(&channel->mutex);//unlock after
    return SUCCESS;
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    //void* local_pointer = &data;
    //sem_post(&channel->sem_select);
    if(channel == NULL)//channel shouldn't have nothing
    {
        return GEN_ERROR;
    }
    //Basically same as send but without waiting 
    //Need mutex in case of multiple thread access
    pthread_mutex_lock(&channel->mutex); // need to lock then unlock later for single thread access
    
    if(channel->status == -2)// check if channel is closed
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    pthread_mutex_unlock(&channel->mutex);

    
    int trywait = sem_trywait(&channel->sem_send);// need to wait
    
    if(trywait == 0)
    {
    pthread_mutex_lock(&channel->mutex);// lock before adding to share memory
    //sem_post(channel->sem_select);
    buffer_add(channel->buffer, data);
    sem_post(&channel->sem_receive);//increment recieve to keep track of the number of messages
   if(channel->sem_select != NULL)
    {
       list_node_t* node = channel->list->head;
       while(node) {
            sem_post(node->data);
            node = node->next;
            }
        
    }
    //sem_post(channel->sem_select);

    pthread_mutex_unlock(&channel->mutex);//unlock after
    }
    else {
        return CHANNEL_FULL;
    }
    
    return SUCCESS;

}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    //data = &channel->chan_data;//Data is set equal to the respective channel in sem select
     //Need mutex in case of multiple thread access
    pthread_mutex_lock(&channel->mutex); // need to lock then unlock later for single thread access
    if(channel == NULL)//channel shouldn't have nothing
    {
        pthread_mutex_unlock(&channel->mutex);
        return GEN_ERROR;
    }

    if(channel->status == -2)// check if channel is closed
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    pthread_mutex_unlock(&channel->mutex);

    int trywait = sem_trywait(&channel->sem_receive);// need to wait

    if(trywait == 0)
    {
    pthread_mutex_lock(&channel->mutex);// lock before adding to share memory
    buffer_remove(channel->buffer, data);
    if(channel->sem_select != NULL)
    {
       list_node_t* node = channel->list->head;
       while(node) {
            sem_post(node->data);
            node = node->next;
            }
        
    }
    sem_post(&channel->sem_send);// keep track of open slots
    //sem_post(channel->sem_select);
    pthread_mutex_unlock(&channel->mutex);//unlock after
    }
    else {
        return CHANNEL_EMPTY;
    }
    return SUCCESS;

}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GEN_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    
    //channel->thread_num++; // inspired by barrier example, should count the amount of threads that call this function
    //int count = 0;
    if(channel == NULL)//channel shouldn't have nothing
    {
        return GEN_ERROR;
    }

    pthread_mutex_lock(&channel->mutex); // need to lock then unlock later for single thread access
    if(channel->status == -2)// check if channel is closed
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    pthread_mutex_unlock(&channel->mutex); 
    
    pthread_mutex_lock(&channel->mutex);
    channel->status = -2;
    //sem_post(*&channel->sem_select);
    sem_post(&channel->sem_send);
    sem_post(&channel->sem_receive);
    //sem_post(channel->sem_select);
    if(channel->sem_select != NULL)
    {
       list_node_t* node = channel->list->head;
       while(node) {
            sem_post(node->data);
            node = node->next;
            }
        
    }
    pthread_mutex_unlock(&channel->mutex);
    return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GEN_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{
    if(channel == NULL)//channel shouldn't have nothing
    {
        return GEN_ERROR;
    }

    pthread_mutex_lock(&channel->mutex); // need to lock then unlock later for single thread access
    if(channel->status != -2)// check if channel is not closed
    {
        pthread_mutex_unlock(&channel->mutex);
        return DESTROY_ERROR;
    }
    pthread_mutex_unlock(&channel->mutex);
    
    sem_destroy(&channel->sem_send);
    sem_destroy(&channel->sem_receive);
    //sem_destroy(channel->sem_select);
    pthread_mutex_destroy(&channel->mutex);
    list_destroy(channel->list);
    buffer_free(channel->buffer);
    free(channel);

    return SUCCESS;
}

// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    sem_t local_sem;
    //channel_status local_select 
    //channel_t local_channel;
    //Need to keep tract of channels in case of multiple select calls
    sem_init(&local_sem, 0, 0);
    for(size_t i = 0; i < channel_count; i++)
    {
    pthread_mutex_lock(&channel_list[i].channel->mutex);
    list_insert(channel_list[i].channel->list, &local_sem); // insert into a sem list
    pthread_mutex_unlock(&channel_list[i].channel->mutex);
    }
    while(true){  //Always needs to be true because when it's not it'll return
        for(size_t i = 0; i < channel_count; i++)
        {
            pthread_mutex_lock(&channel_list[i].channel->mutex);
            channel_list[i].channel->sem_select = &local_sem;
            pthread_mutex_unlock(&channel_list[i].channel->mutex);
            //channel_list[i].channel->chan_data = &channel_list[i].data;
            //list_insert(channel_list[i].channel->list, &local_sem);
            if(channel_list[i].dir == SEND)
            {
                enum channel_status send = channel_non_blocking_send(channel_list[i].channel, channel_list[i].data);
                
                if(send == SUCCESS)
                {
                    *selected_index = i;
                    for(size_t j = 0; j < channel_count; j++) {
                        pthread_mutex_lock(&channel_list[j].channel->mutex);
                        list_remove(channel_list[j].channel->list, list_find(channel_list[j].channel->list, &local_sem));
                        pthread_mutex_unlock(&channel_list[j].channel->mutex);
                    }
                    sem_destroy(&local_sem);
                    pthread_mutex_lock(&channel_list[i].channel->mutex);
                    channel_list[i].channel->sem_select = NULL;
                    pthread_mutex_unlock(&channel_list[i].channel->mutex);
                    return SUCCESS;
                }
                else if(send == CLOSED_ERROR){
                    *selected_index = i;
                    for(size_t j = 0; j < channel_count; j++) {
                        pthread_mutex_lock(&channel_list[j].channel->mutex);
                        list_remove(channel_list[j].channel->list, list_find(channel_list[j].channel->list, &local_sem));
                        pthread_mutex_unlock(&channel_list[j].channel->mutex);
                    }
                    sem_destroy(&local_sem);
                    pthread_mutex_lock(&channel_list[i].channel->mutex);
                    channel_list[i].channel->sem_select = NULL;
                    pthread_mutex_unlock(&channel_list[i].channel->mutex);
                    return CLOSED_ERROR;
                }
                
                else if(send == GEN_ERROR)
                {
                    *selected_index = i;
                    for(size_t j = 0; j < channel_count; j++) {
                        pthread_mutex_lock(&channel_list[j].channel->mutex);
                        list_remove(channel_list[j].channel->list, list_find(channel_list[j].channel->list, &local_sem));
                        pthread_mutex_unlock(&channel_list[j].channel->mutex);
                    }
                    sem_destroy(&local_sem);
                    pthread_mutex_lock(&channel_list[i].channel->mutex);
                    channel_list[i].channel->sem_select = NULL;
                    pthread_mutex_unlock(&channel_list[i].channel->mutex);
                    return GEN_ERROR;
                }
                
                
            }
            if(channel_list[i].dir == RECV)
            {
                enum channel_status receive = channel_non_blocking_receive(channel_list[i].channel, &channel_list[i].data); // Call recieve
                if(receive == SUCCESS)
                {
                *selected_index = i;
                for(size_t j = 0; j < channel_count; j++) {
                        pthread_mutex_lock(&channel_list[j].channel->mutex);
                        list_remove(channel_list[j].channel->list, list_find(channel_list[j].channel->list, &local_sem));
                        pthread_mutex_unlock(&channel_list[j].channel->mutex);
                    }
                sem_destroy(&local_sem);
                pthread_mutex_lock(&channel_list[i].channel->mutex);
                channel_list[i].channel->sem_select = NULL;
                pthread_mutex_unlock(&channel_list[i].channel->mutex);
                return SUCCESS;
                }
                else if(receive == CLOSED_ERROR){
                    *selected_index = i;
                   for(size_t j = 0; j < channel_count; j++) {
                        pthread_mutex_lock(&channel_list[j].channel->mutex);
                        list_remove(channel_list[j].channel->list, list_find(channel_list[j].channel->list, &local_sem));
                        pthread_mutex_unlock(&channel_list[j].channel->mutex);
                    }
                    sem_destroy(&local_sem);
                    pthread_mutex_lock(&channel_list[i].channel->mutex);
                    channel_list[i].channel->sem_select = NULL;
                    pthread_mutex_unlock(&channel_list[i].channel->mutex);
                    return CLOSED_ERROR;
                }
                
                else if(receive == GEN_ERROR)
                {
                    *selected_index = i;
                    for(size_t j = 0; j < channel_count; j++) {
                        pthread_mutex_lock(&channel_list[j].channel->mutex);
                        list_remove(channel_list[j].channel->list, list_find(channel_list[j].channel->list, &local_sem));
                        pthread_mutex_unlock(&channel_list[j].channel->mutex);
                    }
                    sem_destroy(&local_sem);
                    pthread_mutex_lock(&channel_list[i].channel->mutex);
                    channel_list[i].channel->sem_select = NULL;
                    pthread_mutex_unlock(&channel_list[i].channel->mutex);
                    return GEN_ERROR;
                }
            }
        }

    //After checking every channel, need to check if it was unsuccessful. If so we wait
    sem_wait(&local_sem); // when posted it starts all over again
    }

    //May need to implement goto to clean up
    
    //sem_post(&local_sem);
    //Use send post when channel is not full, modify functions
    
}

/*
(gdb) r test_channel_close_with_send 10 doesn't have to be 10
CRTL + z
thread apply all bt */