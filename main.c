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
    int seller_no;
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
int **product_sales;

bool *customer_status;
bool *current_day_initialized;

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

transaction_struct *create_transaction(int customer_to_serve, int simulation_day, bool is_successful, int seller_no);
reserve_struct *create_reserve(int customer_to_serve);

void add_to_transaction_list(transaction_struct *newTransaction);
void add_to_reserve_list(reserve_struct *newReserve);
int cancel_reservation(int customer_to_serve);
void delete_reservation(reserve_struct *to_delete, reserve_struct *previous);

void print_summary();
void clean_up();
void clean_reserve_list();
void clean_transaction_list();

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

    //Also we need to reset the values of customer_status and seller_to_customer;
    if(customer_status != NULL && seller_to_customer != NULL){
        for(int i = 0; i < number_of_customers; i++) customer_status[i] = true;
        for(int i = 0; i < number_of_sellers; i++) seller_to_customer[i] = -1;
    }

    //We will create space for current_day_initialized if it is not already allocated.
    if(current_day_initialized == NULL){
        current_day_initialized = malloc(number_of_simulation_days * sizeof(bool));

        //Also, we need to set all values of it to zero except the first day since we already initialized it.
        current_day_initialized[0] = true;
        for(int i = 1; i < number_of_simulation_days; i++) current_day_initialized[i] = false;
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
                    default:break;
                }
            }else if(loop_count < 4 + number_of_products){

                //Now, we will loop by product count.
                //Also, just for the first time we need to allocate some space if it is not already allocated.
                if(loop_count == 4 && num_of_instances_of_product == NULL)
                    num_of_instances_of_product = malloc(number_of_products * sizeof(int));

                num_of_instances_of_product[loop_count - 4] = (int) strtol(input, NULL, 10);
            }else{

                //And lastly, we need to fill in customers information.
                //Firstly, lets allocate some space for it if it is not already allocated.
                if(loop_count == 4 + number_of_products && customer_information == NULL){
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

    //Creating mutexes.
    seller_mutex = malloc(number_of_sellers * sizeof(pthread_mutex_t));

    for(i = 0; i < number_of_sellers; i++){

        pthread_mutex_init(&seller_mutex[i], NULL);
    }

    pthread_mutex_init(&transaction_mutex, NULL);
    pthread_mutex_init(&reserve_mutex, NULL);

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
        sleep(1);
        current_simulation_day++;
        printf("Day %d ended.\n",current_simulation_day);

        //After the current day finishes, we need to reset all information.
        //To be able to do that, we need to wait for all customers and sellers to finish their current job.
        for(int i = 0; i < number_of_sellers; i++){
            while(seller_to_customer[i] != -1);
        }

        //Now, all jobs has finished, we can reset all values back to original.
        read_file();

        //We need to clear the reserve list, as well.
        clean_reserve_list();

        //Also, we will set the current days flag to true if the simulation has not ended.
        if(current_simulation_day != number_of_simulation_days) current_day_initialized[current_simulation_day] = true;
    }
}

void *customer_thread(void *argument){

    int i = 0;
    int operation_type, product_type, product_amount;
    int status = -1;
    int customer_no = (int)(intptr_t) argument;

    //While the simulation days is not over...
    while(current_simulation_day != number_of_simulation_days){

        //Before doing anything, we should check if the current day has been initialized.
        while(current_day_initialized[current_simulation_day] == false);

        int current_day = current_simulation_day;

        if(customer_status[customer_no] == false){

            //If this is the case, this customer cannot do anything else until the current day finishes. So we need to suspend it.
            while(current_day == current_simulation_day);

            goto end_of_customer;
        }

        //For each operation, we need another random number for the type of the operation. We have 3 different operations.
        operation_type = (int) random() % 3;

        if(operation_type == 0){ // BUY PRODUCT

            product_type = (int) random() % number_of_products;
            product_amount = (int) (random() % 5) + 1;

            customers[customer_no].operation_type = 0;
            customers[customer_no].product_type = product_type;
            customers[customer_no].product_amount = product_amount;

        }else if(operation_type == 1){ // RESERVE PRODUCT

            product_type = (int) random() % number_of_products;
            product_amount = (int) (random() % 5) + 1;

            customers[customer_no].operation_type = 1;
            customers[customer_no].product_type = product_type;
            customers[customer_no].product_amount = product_amount;

        }else{ // CANCEL RESERVATION

            customers[customer_no].operation_type = 2;
        }

        //Customer needs to find an empty seller as long as we're on the same day.
        while(current_day == current_simulation_day){

            for(i = 0; i < number_of_sellers; i++){

                status = pthread_mutex_trylock(&seller_mutex[i]);

                //If we able to lock anything which means we found an empty seller, we will exit the loop.
                if(status != EBUSY)
                    break;
            }

            if(status != EBUSY)
                break;
        }

        //We need to reset the loop if we're not on the same day.
        if(current_day != current_simulation_day){
            if(status != EBUSY && status != -1) pthread_mutex_unlock(&seller_mutex[i]);
            continue;
        }

        //Now, we need a way to communicate with the seller. So we set the arrays value.
        seller_to_customer[i] = customer_no;

        //We need to keep the customer waiting until its job finishes.
        while(seller_to_customer[i] == customer_no);

        end_of_customer:;
    }

    pthread_exit(NULL);
}

void *seller_thread(void *argument){

    int customer_to_serve, operation_type;
    int seller_no = (int)(intptr_t) argument;

    //While the simulation days is not over...
    while(current_simulation_day != number_of_simulation_days){

        //The seller waits for a customer to lock its mutex until the end of its execution.
        while(seller_to_customer[seller_no] == -1)
            if(current_simulation_day == number_of_simulation_days) goto end_of_seller;

        //Now, the seller can do a job.
        customer_to_serve = seller_to_customer[seller_no];
        operation_type = customers[customer_to_serve].operation_type;

        //Firstly, we need to check if that customer has any operation right.
        if(customer_information[customer_to_serve][1] <= 0){

            //Means the customer has no right to do its job.
            customer_status[customer_to_serve] = false;

        }else{

            if(operation_type == 0){ // BUY PRODUCT

                //We need to check if that amount exists.
                if(num_of_instances_of_product[customers[customer_to_serve].product_type] < customers[customer_to_serve].product_amount){

                    //Means wanted amount do not exist. Unsuccessful transaction.
                    add_to_transaction_list(create_transaction(customer_to_serve, current_simulation_day, false, seller_no));

                }else{

                    //We can make this transaction successfully.
                    add_to_transaction_list(create_transaction(customer_to_serve, current_simulation_day, true, seller_no));

                    //Lastly, we need to decrease the amount from product amount.
                    pthread_mutex_lock(&product_mutex[customers[customer_to_serve].product_type]);
                    num_of_instances_of_product[customers[customer_to_serve].product_type] -= customers[customer_to_serve].product_amount;
                    product_sales[customers[customer_to_serve].product_type][0] += customers[customer_to_serve].product_amount;
                    pthread_mutex_unlock(&product_mutex[customers[customer_to_serve].product_type]);
                }

            }else if(operation_type == 1){ // RESERVE PRODUCT

                //We need to check if that amount exists.
                if(num_of_instances_of_product[customers[customer_to_serve].product_type] < customers[customer_to_serve].product_amount){

                    //Means wanted amount do not exist. Unsuccessful transaction.
                    add_to_transaction_list(create_transaction(customer_to_serve, current_simulation_day, false, seller_no));

                }else if(customer_information[customer_to_serve][2] < customers[customer_to_serve].product_amount){

                    //Secondly, we need to check if customers reserve amount is enough. If this is the case, unsuccessful transaction.
                    add_to_transaction_list(create_transaction(customer_to_serve, current_simulation_day, false, seller_no));

                }else{

                    //We can make this transaction successfully.
                    add_to_transaction_list(create_transaction(customer_to_serve, current_simulation_day, true, seller_no));

                    //We need to decrease the amount from product amount.
                    pthread_mutex_lock(&product_mutex[customers[customer_to_serve].product_type]);
                    num_of_instances_of_product[customers[customer_to_serve].product_type] -= customers[customer_to_serve].product_amount;
                    product_sales[customers[customer_to_serve].product_type][1] += customers[customer_to_serve].product_amount;
                    pthread_mutex_unlock(&product_mutex[customers[customer_to_serve].product_type]);

                    //Also, we need to add this to the reserve list.
                    add_to_reserve_list(create_reserve(customer_to_serve));

                    //We need to decrease from customers allowed reservation count.
                    customer_information[customer_to_serve][2] -= customers[customer_to_serve].product_amount;
                }

            }else{ // CANCEL RESERVATION

                int product_amount = cancel_reservation(customer_to_serve);
                bool is_successful;

                if(product_amount == -1) is_successful = false;
                else is_successful = true;

                //We need to make the transaction.
                add_to_transaction_list(create_transaction(customer_to_serve, current_simulation_day, is_successful, seller_no));

                pthread_mutex_lock(&product_mutex[customers[customer_to_serve].product_type]);
                if(is_successful == true){
                    product_sales[customers[customer_to_serve].product_type][2] += product_amount;
		    num_of_instances_of_product[customers[customer_to_serve].product_type] += product_amount;
		}
                pthread_mutex_unlock(&product_mutex[customers[customer_to_serve].product_type]);
            }

            //We need to decrease from customers allowed operation count.
            customer_information[customer_to_serve][1]--;
        }

        //Since seller finished its job, we need to reset its status.
        seller_to_customer[seller_no] = -1;

        //We will release the mutex.
        pthread_mutex_unlock(&seller_mutex[seller_no]);

        end_of_seller:;
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

    product_sales = malloc(number_of_products * sizeof(int *));
    for(int i = 0; i < number_of_products; i++) product_sales[i] = calloc(3, sizeof(int));

    //Also we need to reset the values of customer_status and seller_to_customer;
    for(int i = 0; i < number_of_customers; i++) customer_status[i] = true;
    for(int i = 0; i < number_of_sellers; i++) seller_to_customer[i] = -1;
}

transaction_struct *create_transaction(int customer_to_serve, int simulation_day, bool is_successful, int seller_no){

    transaction_struct *newTransaction = malloc(sizeof(transaction_struct));
    newTransaction->customer_no = customer_to_serve;
    newTransaction->seller_no = seller_no;
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

int cancel_reservation(int customer_to_serve){

    int returnValue = -1;

    //First we need to check if any reserve exists.
    if(reserve != NULL){

        //We need to traverse the list until we find a matching customer number.
        reserve_struct *current = reserve;
        reserve_struct *pre_current = NULL;

        while(current != NULL){

            if(current->customer_no == customer_to_serve){

                returnValue = current->product_amount;

                customers[customer_to_serve].product_type = current->product_type;

                //Delete the reservation.
                delete_reservation(current, pre_current);

                break;

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

    char operation_name[16];

    //We need to create a file.
    FILE *fp = fopen("Output.txt", "w");

    fprintf(fp, "%s\t%s\t%s\t%s\t%s\n", "Customer_ID", "Seller_ID", "Operation", "Simulation_Day", "Is Successful");
    fprintf(fp, "-------------------------------------------------------------------------\n");

    transaction_struct *current = transaction;

    while(current != NULL){

        if(current->operation_type == 0) strcpy(operation_name, "BUY");
        else if(current->operation_type == 1) strcpy(operation_name, "RESERVE");
        else strcpy(operation_name, "CANCEL");

        fprintf(fp, "%d\t\t%d\t\t%s\t\t\t%d\t", (current->customer_no + 1), (current->seller_no + 1), operation_name, (current->simulation_day + 1));
        fprintf(fp, "%s\n", current->is_successful ? "true" : "false");

        current = current->next;
    }

    ////////////////////////////////////////////////////////

    fprintf(fp, "\n\nNUMBER OF TRANSACTION\n\n");

    current = transaction;
    int transaction_of_customer[number_of_customers];
    int transaction_of_seller[number_of_sellers];

    for(int i = 0; i < number_of_customers; i++) transaction_of_customer[i] = 0;
    for(int i = 0; i < number_of_sellers; i++) transaction_of_seller[i] = 0;

    while(current != NULL){

        transaction_of_customer[current->customer_no]++;
        transaction_of_seller[current->seller_no]++;

        current = current->next;
    }

    for(int i = 0; i < number_of_customers; i++) fprintf(fp, "Customer #%d - %d\n", (i + 1), transaction_of_customer[i]);
    fprintf(fp, "\n");
    for(int i = 0; i < number_of_sellers; i++) fprintf(fp, "Seller #%d - %d\n", (i + 1), transaction_of_seller[i]);

    ////////////////////////////////////////////////////////

    fprintf(fp, "\n\nPRODUCT INFORMATION\n\n");

    for(int i = 0; i < number_of_products; i++){

        fprintf(fp, "Product #%d\t%d\t%d\t%d\n", (i + 1), product_sales[i][0], product_sales[i][1], product_sales[i][2]);
    }

    ////////////////////////////////////////////////////////

    fclose(fp);
}

void clean_up(){

    int i;

    free(num_of_instances_of_product);

    for(i = 0; i < number_of_customers; i++) free(customer_information[i]);
    free(customer_information);

    for(i = 0; i < number_of_products; i++) free(product_sales[i]);
    free(product_sales);

    free(customer_ids);
    free(seller_ids);
    free(seller_mutex);
    free(customers);
    free(customer_status);
    free(seller_to_customer);
    free(product_mutex);
    free(current_day_initialized);

    clean_reserve_list();
    clean_transaction_list();
}

void clean_reserve_list(){

    reserve_struct *reserve_current = reserve;
    reserve_struct *reserve_to_delete;

    while(reserve_current != NULL){
        reserve_to_delete = reserve_current;
        reserve_current = reserve_current->next;

        free(reserve_to_delete);
    }

    reserve = NULL;
}

void clean_transaction_list(){

    transaction_struct *transaction_current = transaction;
    transaction_struct *transaction_to_delete;

    while(transaction_current != NULL){
        transaction_to_delete = transaction_current;
        transaction_current = transaction_current->next;

        free(transaction_to_delete);
    }
}
//</editor-fold>
