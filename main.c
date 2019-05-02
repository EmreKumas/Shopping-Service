#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

//////////////////////////////////////////////////////// - GLOBAL VARIABLES
//<editor-fold desc="GLOBAL VARIABLES">

//Customer struct
typedef struct{

    int operation_type;
    int product_type;
    int product_amount;

}customer_struct;

//Reserve struct
typedef struct reserve_struct_type{

    int customer_no;
    int product_type;
    int product_amount;
    struct reserve_struct_type *next;

}reserve_struct;

//Transaction struct
typedef struct transaction_struct_type{

    int customer_no;
    int operation_type;
    int product_amount;
    int simulation_day;
    bool is_successful;
    struct transaction_struct_type *next;

}transaction_struct;

int number_of_customers;
int number_of_sellers;
int number_of_simulation_days;
int number_of_products;

int current_simulation_day;

int *num_of_instances_of_product;
int **customer_information;
int *seller_to_customer;

bool *customer_status;

pthread_t *customer_ids;
pthread_t *seller_ids;

pthread_mutex_t *seller_mutex;
pthread_mutex_t transaction_mutex;
pthread_mutex_t reserve_mutex;
pthread_mutex_t *product_mutex;

customer_struct *customers;
reserve_struct *reserve = NULL;
transaction_struct *transaction = NULL;

//</editor-fold>
//////////////////////////////////////////////////////// - PROTOTYPES OF FUNCTIONS
//<editor-fold desc="PROTOTYPES OF FUNCTIONS">

void read_file();
void read_from_file(FILE *fp);
void create_necessary_variables();

void create_threads();
void manage_threads();
void join_threads();

void *customer_thread(void *argument);
void *seller_thread(void *argument);

transaction_struct *create_transaction(int customer_to_serve, int simulation_day, bool is_successful);
reserve_struct *create_reserve(int customer_to_serve);
void add_to_transaction_list(transaction_struct *newTransaction);
void add_to_reserve_list(reserve_struct *newReserve);
bool cancel_reservation(int customer_to_serve);
void delete_reservation(reserve_struct *to_delete, reserve_struct *previous);

void print_summary();
void clean_up();
void clean_linkedlists();

//</editor-fold>
//////////////////////////////////////////////////////// - MAIN METHOD

int main(){

    //Before beginning we need to make sure that the program create different random numbers on every sequence.
    srand((unsigned int) time(0));

    //Reading the input file.
    read_file();

    //Creating necessary variables.
    create_necessary_variables();

    //Creating the threads.
    create_threads();

    //Managing threads;
    manage_threads();

    //When the job is done, we need to join all threads.
    join_threads();

    //Printing the summary and cleaning up spaces allocated.
    print_summary();
    clean_up();

    return 0;
}

//////////////////////////////////////////////////////// - READING OPERATIONS
//<editor-fold desc="READING OPERATIONS">

void read_file(){

    FILE *fp;

    if(access("input.txt", F_OK) != -1){

        fp = fopen("input.txt", "r");
        read_from_file(fp);

    }else{

        printf("\"input.txt\" does not exist. Please make sure it exists before starting this program.\n");
        exit(1);
    }

    for(int i = 0; i < number_of_customers; i++) customer_status[i] = true;
}

void read_from_file(FILE *fp){

    int loop_count = 0, i;
    char input[64];
    char *token;

    while(!feof(fp)){ // NOLINT

        if(fgets(input, 63, fp) != NULL){

            // We know that the first four line will contain the total number of some information.
            if(loop_count < 4){

                token = strtok(input, " ");

                switch(loop_count){
                    case 0:
                        number_of_customers = (int) strtol(token, NULL, 10);
                        break;
                    case 1:
                        number_of_sellers = (int) strtol(token, NULL, 10);
                        break;
                    case 2:
                        number_of_simulation_days = (int) strtol(token, NULL, 10);
                        break;
                    case 3:
                        number_of_products = (int) strtol(token, NULL, 10);
                        break;
                }
            }else if(loop_count < 4 + number_of_products){

                //Now, we will loop by product count.
                //Also, just for the first time we need to allocate some space.
                if(loop_count == 4)
                    num_of_instances_of_product = malloc(number_of_products * sizeof(int));

                num_of_instances_of_product[loop_count - 4] = (int) strtol(input, NULL, 10);
            }else{

                //And lastly, we need to fill in customers information.
                //Firstly, lets allocate some space for it.
                if(loop_count == 4 + number_of_products){
                    customer_information = malloc(number_of_customers * sizeof(int *));

                    for(i = 0; i < number_of_customers; i++)
                        customer_information[i] = malloc(3 * sizeof(int));
                }

                token = strtok(input, " ");
                customer_information[loop_count - 4 - number_of_products][0] = (int) strtol(token, NULL, 10);

                token = strtok(NULL, " ");
                customer_information[loop_count - 4 - number_of_products][1] = (int) strtol(token, NULL, 10);

                token = strtok(NULL, "\n");
                customer_information[loop_count - 4 - number_of_products][2] = (int) strtol(token, NULL, 10);
            }

            loop_count++;
        }
    }
}

//</editor-fold>
//////////////////////////////////////////////////////// - THREADS
//<editor-fold desc="THREADS">

void create_threads(){

    //First, we need to create the arrays which will hold ids.
    customer_ids = malloc(number_of_customers * sizeof(pthread_t));
    seller_ids = malloc(number_of_sellers * sizeof(pthread_t));

    int i, thread_control;

    //Creating customer threads.
    for(i = 0; i < number_of_customers; i++){

        thread_control = pthread_create(&customer_ids[i], NULL, &customer_thread, (void *)(intptr_t) i);

        if(thread_control){
            fprintf(stderr, "Error: return code from creating customer thread is %d\n", thread_control);
            exit(-1);
        }
    }

    //Creating seller threads.
    for(i = 0; i < number_of_sellers; i++){

        thread_control = pthread_create(&seller_ids[i], NULL, &seller_thread, (void *)(intptr_t) i);

        if(thread_control){
            fprintf(stderr, "Error: return code from creating seller thread is %d\n", thread_control);
            exit(-1);
        }
    }

    seller_mutex = malloc(number_of_sellers * sizeof(pthread_mutex_t));

    //Creating mutexes.
    for(i = 0; i < number_of_sellers; i++){

        pthread_mutex_init(&seller_mutex[i], NULL);
    }

    pthread_mutex_init(&transaction_mutex, NULL);
    pthread_mutex_init(&reserve_mutex, NULL);
}

void join_threads(){

    int i, thread_control;

    for(i = 0; i < number_of_customers; i++){

        thread_control = pthread_join(customer_ids[i], NULL);

        if(thread_control){
            fprintf(stderr, "Error: return code from joining the customer thread is %d\n", thread_control);
            exit(-1);
        }
    }

    for(i = 0; i < number_of_sellers; i++){

        thread_control = pthread_join(seller_ids[i], NULL);

        if(thread_control){
            fprintf(stderr, "Error: return code from joining the seller thread is %d\n", thread_control);
            exit(-1);
        }
    }

    //Destroying mutexes.
    for(i = 0; i < number_of_sellers; i++){

        pthread_mutex_destroy(&seller_mutex[i]);
    }

    pthread_mutex_destroy(&transaction_mutex);
    pthread_mutex_destroy(&reserve_mutex);
}

void manage_threads(){

    //We created all threads. Since 10 seconds correspond to one simulation day, we need to suspend the main thread for 10 seconds.
    while(current_simulation_day < number_of_simulation_days){
        sleep(10);
        current_simulation_day++;

        //After the current day finishes, we need to reset all information.
        free(num_of_instances_of_product);

        for(int i = 0; i < number_of_customers; i++)
            free(customer_information[i]);

        read_file();
    }
}

void *customer_thread(void *argument){

    int i, operation_type;
    int product_type, product_amount;
    int status = 0;
    int customer_no = (int)(intptr_t) argument;

    //While the simulation days is not over...
    while(current_simulation_day != number_of_simulation_days){

        if(customer_status[customer_no] == false){

            //If this is the case, this customer cannot do anything else until the current day finishes. So we need to suspend it.
            int current_day = current_simulation_day;

            while(current_day == current_simulation_day);
        }

        //For each operation, we need another random number for the type of the operation. We have 3 different operations.
        operation_type = (int) random() % 3;

        if(operation_type == 0){ // BUY PRODUCT

            product_type = (int) (random() % number_of_products) + 1;
            product_amount = (int) (random() % 5) + 1;

            customers[customer_no].operation_type = 0;
            customers[customer_no].product_type = product_type;
            customers[customer_no].product_amount = product_amount;

        }else if(operation_type == 1){ // RESERVE PRODUCT

            product_type = (int) (random() % number_of_products) + 1;
            product_amount = (int) (random() % 5) + 1;

            customers[customer_no].operation_type = 1;
            customers[customer_no].product_type = product_type;
            customers[customer_no].product_amount = product_amount;

        }else{ // CANCEL RESERVATION

            customers[customer_no].operation_type = 2;
        }

        //Customer needs to find an empty seller.
        while(true){

            for(i = 0; i < number_of_sellers; i++){

                status = pthread_mutex_trylock(&seller_mutex[i]);

                //If we able to lock anything which means we found an empty seller, we will exit the loop.
                if(status != EBUSY)
                    break;
            }

            if(status != EBUSY)
                break;
        }

        //Now, we need a way to communicate with the seller. So we set the arrays value.
        seller_to_customer[i] = customer_no;

        //We need to keep wait the customer until its job finishes.
        pthread_mutex_lock(&seller_mutex[i]);

        //Now, the job has finished so we need to release the mutex.
        pthread_mutex_unlock(&seller_mutex[i]);
    }

    pthread_exit(NULL);
}

void *seller_thread(void *argument){

    int customer_to_serve, operation_type;
    int seller_no = (int)(intptr_t) argument;

    //While the simulation days is not over...
    while(current_simulation_day != number_of_simulation_days){

        //The seller waits for a customer to lock its mutex...
        while(pthread_mutex_trylock(&seller_mutex[seller_no]) != EBUSY){

            //Means mutex is not locked. We will release it.
            pthread_mutex_unlock(&seller_mutex[seller_no]);
        }

        //Now, the seller can do a job.
        customer_to_serve = seller_to_customer[seller_no];
        operation_type = customers[customer_to_serve].operation_type;

        //Firstly, we need to check if that customer has any operation right.
        if(customer_information[customer_to_serve][1] <= 0){

            //Means the customer has no right to do its job. After adding the transaction we need to terminate.
            add_to_transaction_list(create_transaction(customer_to_serve, 1, false));

            customer_status[customer_to_serve] = false;

        }else{

            if(operation_type == 0){ // BUY PRODUCT

                //We need to check if that amount exists.
                if(num_of_instances_of_product[customers[customer_to_serve].product_type] < customers[customer_to_serve].product_amount){

                    //Means wanted amount do not exist. Unsuccessful transaction.
                    add_to_transaction_list(create_transaction(customer_to_serve, 1, false));

                }else{

                    //We can make this transaction successfully.
                    add_to_transaction_list(create_transaction(customer_to_serve, 1, true));

                    //Lastly, we need to decrease the amount from product amount.
                    pthread_mutex_lock(&product_mutex[customers[customer_to_serve].product_type]);
                    num_of_instances_of_product[customers[customer_to_serve].product_type] -= customers[customer_to_serve].product_amount;
                    pthread_mutex_unlock(&product_mutex[customers[customer_to_serve].product_type]);
                }

            }else if(operation_type == 1){ // RESERVE PRODUCT

                //We need to check if that amount exists.
                if(num_of_instances_of_product[customers[customer_to_serve].product_type] < customers[customer_to_serve].product_amount){

                    //Means wanted amount do not exist. Unsuccessful transaction.
                    add_to_transaction_list(create_transaction(customer_to_serve, 1, false));

                }else if(customer_information[customer_to_serve][2] < customers[customer_to_serve].product_amount){

                    //Secondly, we need to check if customers reserve amount is enough. If this is the case, unsuccessful transaction.
                    add_to_transaction_list(create_transaction(customer_to_serve, 1, false));

                }else{

                    //We can make this transaction successfully.
                    add_to_transaction_list(create_transaction(customer_to_serve, 1, true));

                    //We need to decrease the amount from product amount.
                    pthread_mutex_lock(&product_mutex[customers[customer_to_serve].product_type]);
                    num_of_instances_of_product[customers[customer_to_serve].product_type] -= customers[customer_to_serve].product_amount;
                    pthread_mutex_unlock(&product_mutex[customers[customer_to_serve].product_type]);

                    //Also, we need to add this to the reserve list.
                    add_to_reserve_list(create_reserve(customer_to_serve));

                    //We need to decrease from customers allowed reservation count.
                    customer_information[customer_to_serve][2] -= customers[customer_to_serve].product_amount;
                }

            }else{ // CANCEL RESERVATION

                bool is_successful = cancel_reservation(customer_to_serve);

                //We need to make the transaction.
                add_to_transaction_list(create_transaction(customer_to_serve, 1, is_successful));
            }

            //We need to decrease from customers allowed operation count.
            customer_information[customer_to_serve][1]--;
        }

        //We will release the mutex.
        pthread_mutex_unlock(&seller_mutex[seller_no]);
    }

    pthread_exit(NULL);
}

//</editor-fold>
//////////////////////////////////////////////////////// - OTHER FUNCTIONS
//<editor-fold desc="OTHER FUNCTIONS">

void create_necessary_variables(){

    customers = malloc(number_of_customers * sizeof(customer_struct));
    customer_status = malloc(number_of_customers * sizeof(bool));

    seller_to_customer = malloc(number_of_sellers * sizeof(int));

    product_mutex = malloc(number_of_products * sizeof(pthread_mutex_t));
}

transaction_struct *create_transaction(int customer_to_serve, int simulation_day, bool is_successful){

    transaction_struct *newTransaction = malloc(sizeof(transaction_struct));
    newTransaction->customer_no = customer_to_serve;
    newTransaction->operation_type = customers[customer_to_serve].operation_type;
    newTransaction->product_amount = customers[customer_to_serve].product_amount;
    newTransaction->simulation_day = simulation_day;
    newTransaction->is_successful = is_successful;
    newTransaction->next = NULL;

    return newTransaction;
}

reserve_struct *create_reserve(int customer_to_serve){

    reserve_struct *newReserve = malloc(sizeof(reserve_struct));
    newReserve->customer_no = customer_to_serve;
    newReserve->product_type = customers[customer_to_serve].product_type;
    newReserve->product_amount = customers[customer_to_serve].product_amount;
    newReserve->next = NULL;

    return newReserve;
}

void add_to_transaction_list(transaction_struct *newTransaction){

    //We need to lock the mutex to provide synchronization.
    pthread_mutex_lock(&transaction_mutex);

    //First we need to check if any transaction exists.
    if(transaction == NULL){
        transaction = newTransaction;
    }else{

        //We need to find the last element in the linked list.
        transaction_struct *current = transaction;
        while(current->next != NULL)
            current = current->next;

        current->next = newTransaction;
    }

    pthread_mutex_unlock(&transaction_mutex);
}

void add_to_reserve_list(reserve_struct *newReserve){

    //We need to lock the mutex to provide synchronization.
    pthread_mutex_lock(&reserve_mutex);

    //First we need to check if any reserve exists.
    if(reserve == NULL){
        reserve = newReserve;
    }else{

        //We need to find the last element in the linked list.
        reserve_struct *current = reserve;
        while(current->next != NULL)
            current = current->next;

        current->next = newReserve;
    }

    pthread_mutex_unlock(&reserve_mutex);
}

bool cancel_reservation(int customer_to_serve){

    bool returnValue = false;

    //First we need to check if any reserve exists.
    if(reserve != NULL){

        //We need to traverse the list until we find a matching customer number.
        reserve_struct *current = reserve;
        reserve_struct *pre_current = NULL;

        while(current->next != NULL){

            if(current->customer_no == customer_to_serve){

                //Delete the reservation.
                delete_reservation(current, pre_current);

                returnValue = true;
            }else{
                pre_current = current;
                current = current->next;
            }
        }
    }

    return returnValue;
}

void delete_reservation(reserve_struct *to_delete, reserve_struct *previous){

    //We need to lock the mutex to provide synchronization.
    pthread_mutex_lock(&reserve_mutex);

    if(reserve == to_delete){

        //Means this is the first entry.
        reserve = to_delete->next;

    }else
        previous->next = to_delete->next;

    free(to_delete);

    pthread_mutex_unlock(&reserve_mutex);
}

//</editor-fold>
//////////////////////////////////////////////////////// - PRINTING AND CLEANING-UP
//<editor-fold desc="PRINTING AND CLEANING-UP">

void print_summary(){



}

void clean_up(){

    int i;

    free(num_of_instances_of_product);

    for(i = 0; i < number_of_customers; i++)
        free(customer_information[i]);

    free(customer_information);
    free(customer_ids);
    free(seller_ids);
    free(seller_mutex);
    free(customers);
    free(customer_status);
    free(seller_to_customer);
    free(product_mutex);

    clean_linkedlists();
}

void clean_linkedlists(){

    reserve_struct *reserve_current = reserve;
    reserve_struct *reserve_to_delete;

    transaction_struct *transaction_current = transaction;
    transaction_struct *transaction_to_delete;

    while(reserve_current != NULL){
        reserve_to_delete = reserve_current;
        reserve_current = reserve_current->next;

        free(reserve_to_delete);
    }

    while(transaction_current != NULL){
        transaction_to_delete = transaction_current;
        transaction_current = transaction_current->next;

        free(transaction_to_delete);
    }
}

//</editor-fold>
