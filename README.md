Not an actual readme, just required for the assignment.

1. Describe how your exchange works.
The exchange starts by setting up signal handlers for SIGUSR1 and SIGCHLD for inter-process communication, initialising products from the products file and setting up traders. To set up the traders, FIFOs are created for both directions of communication with every trader. The parent process is then forked once for every trader, and using execl each child process runs one of the traders. The exchange then opens each pair of FIFOs in either read or write mode. When all the setup is complete, the market is opened.

The exchange then pauses until a signal is received. The signal handlers queue the signals, storing their signal type and trader. Upon receiving SIGUSR1, the exchange will read the FIFO corresponding to the sending trader. It will decode the instruction and respond accordingly. New orders are placed into a linked list sorted in descending order by price. When all traders have sent SIGCHLD and disconnected, the exchange closes and frees/unlinks all resources.


2. Describe your design decisions for the trader and how it's fault-tolerant.
Storing orders in a linked list simplifies inserting, cancelling, updating and sorting orders. Storing the state of the exchange with a exchange_data_t struct makes important objects such as orders, traders and products easy to access, manipulate and pass around using a pointer. Using a queue to store signal data ensures messages are not missed and are processed in order of arrival, avoiding race conditions. Dynamically allocating memory for the order list, product array and trader array avoids overallocation and is securely implemented.

Other fault tolerance measures taken include:
	Checking instructions are valid (positive quanties/prices, real products, real orders)
	Checking messages are of the correct format
	Ensuring order and trader files exist
	Ensuring disconnected traders will not send or receive messages or signals
	Initialising structures and values to zeroes whenever possible
	Using calloc to automatically initialise dynamics arrays to zeroes
		Ensuring all dynamically allocated memory and files are always freed, unlinked or closed
