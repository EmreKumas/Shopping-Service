#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

//////////////////////////////////////////////////////// - PROTOTYPES OF FUNCTIONS
//<editor-fold desc="PROTOTYPES OF FUNCTIONS">

void read_file();
void read_from_file(FILE *fp);

void create_threads();
void join_threads();

void *customer_thread(void *argument);
void *seller_thread(void *argument);

void print_summary();
void clean_up();

//</editor-fold>
//////////////////////////////////////////////////////// - GLOBAL VARIABLES

//Customer struct
typedef struct{

    int operation_type;
    int product_type;
    int product_amount;

}customer_struct;

//Reserve struct
typedef struct{

    int customer_no;
    int product_type;
    int product_amount;

}reserve_struct;

int number_of_customers;
int number_of_sellers;
int number_of_simulation_days;
int number_of_products;

int *num_of_instances_of_product;
int **customer_information;

bool *customer_status;

pthread_t *customer_ids;
pthread_t *seller_ids;

pthread_mutex_t *seller_mutex;

customer_struct *customers;
reserve_struct *reserve = NULL;

//////////////////////////////////////////////////////// - MAIN METHOD

int main(){

    //Before beginning we need to make sure that the program create different random numbers on every sequence.
    srand((unsigned int) time(0));

    //Reading the input file.
    read_file();

    //Creating the threads.
    create_threads();

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

    customers = malloc(number_of_customers * sizeof(customer_struct));
    customer_status = malloc(number_of_customers * sizeof(bool));

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

        thread_control = pthread_create(&customer_ids[i], NULL, &customer_thread, (void *) i);

        if(thread_control){
            fprintf(stderr, "Error: return code from creating customer thread is %d\n", thread_control);
            exit(-1);
        }
    }

    //Creating seller threads.
    for(i = 0; i < number_of_sellers; i++){

        thread_control = pthread_create(&seller_ids[i], NULL, &seller_thread, NULL);

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
}

void *customer_thread(void *argument){

    int i, operation_type;
    int product_type, product_amount;
    int status = 0;
    int customer_no = (int) argument;

    //While the simulation days is not over...
    while(true){

        if(customer_status[customer_no] == false){

            //If this is the case, this customer cannot do anything else until the current day finishes. So we need to suspend it.
            ///********
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

        //We need to keep wait the customer until its job finishes.
        pthread_mutex_lock(&seller_mutex[i]);

        //Now, the job has finished so we need to release the mutex.
        pthread_mutex_unlock(&seller_mutex[i]);

        break;
    }

    pthread_exit(NULL);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void *seller_thread(void *argument){



    pthread_exit(NULL);
}
#pragma clang diagnostic pop

//</editor-fold>

//////////////////////////////////////////////////////// - OTHER FUNCTIONS
//<editor-fold desc="OTHER FUNCTIONS">

void cancel_reservation(int customer_no){


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
}
//</editor-fold>
