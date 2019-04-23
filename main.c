#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

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

int number_of_customers;
int number_of_sellers;
int number_of_simulation_days;
int number_of_products;

int *num_of_instances_of_product;
int **customer_information;

pthread_t *customer_ids;
pthread_t *seller_ids;

//////////////////////////////////////////////////////// - MAIN METHOD

int main(){

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

    //Creating seller threads.
    for(i = 0; i < number_of_sellers; i++){

        thread_control = pthread_create(&seller_ids[i], NULL, &seller_thread, NULL);

        if(thread_control){
            fprintf(stderr, "Error: return code from creating seller thread is %d\n", thread_control);
            exit(-1);
        }
    }

    //Creating customer threads.
    for(i = 0; i < number_of_customers; i++){

        thread_control = pthread_create(&customer_ids[i], NULL, &customer_thread, NULL);

        if(thread_control){
            fprintf(stderr, "Error: return code from creating customer thread is %d\n", thread_control);
            exit(-1);
        }
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void *customer_thread(void *argument){


    pthread_exit(NULL);
}


void *seller_thread(void *argument){


    pthread_exit(NULL);
}
#pragma clang diagnostic pop

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
}
//</editor-fold>
