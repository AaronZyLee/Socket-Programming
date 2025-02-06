# Socket-Programming-Project

### What I have done  
	I have completed all parts of requirement.     
	1. Phase 1  
	- All four server programs boot up, listening at specific port, showing boot up message.  
	- Run client program, sending function (write or compute) and input parameters to the AWS server over TCP.  
	2. Phase 2A
	- AWS server send write function and parameters to the server A over UDP.
	- Server A write the new link info to the database and sends back the new link_id to AWS over UDP, displaying the results.
	- The <write> function is implemented in serverA.c 
	3. Phase 2B  
	 - AWS server send compute function and parameters to the server A over UDP.
	 - The <search> function is implemented in serverA.c
	 - Server A sends the searching result back to AWS over UDP. If the link is found, it communicates with Server B. Otherwise, it displays that link_id not found.
	 - Server B receives the parameters and compute the transmission delay, propogation delay and end-to-end-delay.
	 - The <computeTt>,<computeTp> functions are implemented in serverB.c
	 - Server B sends results back to AWS.
	4. Phase 3
	- AWS sends the results back to client and monitor over TCP and displays the operation status. 
	
### Code files  
	***aws.c***  
	Receive function and parameters from client over TCP, send the function and parameters to database server and computation server separately over UDP. Send result to monitor and client over TCP. After booting, the aws server can only be shut down by crtl+c command.  
	***client.c***  
	Send function and parameters to aws over TCP. Receive feedback from aws and show on screen. The client will terminate itself after receiving feedback from aws.  
	***monitor.c***  
	Booting before running client, show both input and result sent by aws server.  
	***serverA.c***  
	Receive function and parameters from aws over UDP, do the function <write> or <search> in database, send back result to aws.  
	***serverB.c***  
	Receive parameters from aws over UDP, do the computation of end-to-end-delay based on the input, send back result to aws.   

### Messages exchange format example  
	1. Execute <write> function  
	- aws console  
	The AWS is up and running.  
	The AWS received operation <write> and from the client using TCP over port <24776>
	The AWS sent operation <write> and arguments to monitor using TCP over port <25776>
	The AWS sent operation <write> to Backend-Server A using UDP over port <23776>
	The AWS received response from Backend-Server A for writing using UDP over port <23776>
	The AWS sent write response to the monitor using TCP over port <25776>
	The AWS sent result to client for operation <write> using TCP over port <24776>
	- client console  
	The client is up and running.
	The client sent <write> operation to AWS.
	The write operation has been completed successfully 
	- serverA console  
	The Server A received input for writing
	The Server A wrote link <4> to database 
	- monitor console
	The monitor is up and running.
	The monitor received BW = <2>, L = <3>, V = <4> and P = <5> from the AWS
	The write operation has been completed successfully

	2. Execute <compute> function 
	- aws console  
	The AWS is up and running.
	The AWS received operation <compute> and from the client using TCP over port <24776>
	The AWS sent operation <compute> and arguments to monitor using TCP over port <25776>
	The AWS sent operation <compute> to Backend-Server A using UDP over port <23776>
	Link ID not found
	The AWS sent compute results to the monitor using TCP over port <25776>”
	The AWS sent result to client for operation <compute> using TCP over port <24776>
	The AWS received operation <compute> and from the client using TCP over port <24776>
	The AWS sent operation <compute> and arguments to monitor using TCP over port <25776>
	The AWS sent operation <compute> to Backend-Server A using UDP over port <23776>
	The AWS received link information from Backend-Server A using UDP over port <23776>
	The AWS sent link ID=<1>, size=<10000>, power=<10>, and link information to Backend-Server B using UDP over port <23776>
	The AWS received outputs from Backend-Server B using UDP over port <23776>
	The AWS sent compute results to the monitor using TCP over port <25776>”
	The AWS sent result to client for operation <compute> using TCP over port <24776> 
	- client console  
	The client is up and running.
	The client sent ID=<5>, size=<10000>, and power=<10> to AWS.
	Link ID not found 
	The client is up and running.
	The client sent ID=<1>, size=<10000>, and power=<10> to AWS.
	The delay for link <1> is <0.42>ms
	- monitor console  
	The monitor is up and running.
	The monitor received link ID=<5>, size=<10000>, and power=<10> from the AWS
	Link ID not found
	The monitor received link ID=<1>, size=<10000>, and power=<10> from the AWS
	The result for link <1>:
	Tt=<0.40>ms,
	Tp=<0.02>ms,
	Delay=<0.42>ms
	- serverA console  
	The Server A is up and running using UDP on port <21776>.
	The Server A received input <5> for computing
	Link ID not found
	The Server A received input <1> for computing
	The Server A finished sending the search result to AWS
	- serverB console  
	The Server B is up and running using UDP on port <22776>.
	The Server B received link information: link <1>, file size <10000>, and signal power <10>
	The Server B finished the calculation for link <1>
	The Server B finished sending the output to AWS 

### Idiosyncrasy of my project.  
	I set maximum size of the TCP and UTP buffer as 4000, if the searching result is very long, the memory will crash.  

### Reused Code  
	My TCP and UDP setting up code is based on examples in Beej's book.  
