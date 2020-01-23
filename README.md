# Shopping-Service

This program simulates a shopping service by synchronizing the product sales, reservations and cancellations. There exists different ***customers*** and ***sellers*** who can make following operations:

- Buy Product
- Reserve Product
- Cancel Reservation

## Implementation Details

We used ***PThread*** library, ***mutexes*** and ***semaphores*** to provide synchronization. Each customer and seller is a different **thread**, and they all work asynchronously. While a seller can serve more than one customer, a customer can only shop from one seller.
The main thread is responsible for finishing the current day and cancelling the reservations that were supposed to finish but couldn't.

Each seller's and customer's actions are determined randomly to provide variation.

### Input

You give an input to the program containing these informations:

- Number of Customers
- Number of Sellers
- Number of Simulation Days
- Number of Products
- Number of Instances for each product
- Detailed Customer Informations
- Limits per Day

### Output

The program outputs these:
- Each transaction containing the informations of the customer_id, seller_id, simulation_day, operation and is_successful.
- The number of transaction that each customer did. Also, same for the sellers. 
- The amounts of bought, reserved and cancelled information for each product.
