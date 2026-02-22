# Project Title: Packet Queue Simulator





##### Introduction



This project is a simulation of how a network router manages packets when its buffer has limited space(capacity it can hold packets). The program compares FIFO queue and priority queue.



The purpose of the project is to observe how packet loss changes depending on how packets are scheduled and transmitted.



The simulation is written in C++ and runs in console .

IDE : visual studio code





##### What the Program Does



The program simulates a router over a number of time steps.



At each time step:



The user enters how many packets arrive.



For each packet, the user specifies a priority level (1 to 4).



The router attempts to store packets in its buffer.



If the buffer is full, packets may be dropped.



One packet is transmitted per time step.



After the simulation finishes, the program compares:



FIFO (First-In First-Out)and Priority queue.





The results are shown as percentage loss per priority level.



How the Simulation Works

Packet



Each packet only contains a priority value:



Priority 1: Highest



Priority 4:Lowest



1 has the highest priority.



##### Buffer



The buffer is implemented using a vector.

Its size is limited by the capacity entered by the user.



If the buffer becomes full:



In FIFO mode: new packets are dropped.



In Priority mode: the router checks if the arriving packet has higher priority than the weakest packet in the buffer.



If yes: it replaces the weakest packet.



If no:  it is dropped.



##### Transmission



At each time step:



Exactly one packet is sent.



In FIFO mode: the first packet is sent.



In Priority mode: the packet with highest priority is picked first.



##### Statistics Recorded



The program tracks:



Number of packets arrived per priority



Number of packets dropped per priority



Total packets successfully sent



it calculates the percentage loss for each priority under both FIFO and Priority queue at the end.



##### User Input



1\.When the program runs, the user is asked to enter:



2\.Buffer capacity



3\.Simulation duration (number of time steps)



4\.For each time step:



5\.Number of arriving packets



6\.Priority for each packet



This allows clear comparison between both strategies.



##### What I Learned

##### 

From this project, I learned:



1\.How a router buffer works



2\.How packet dropping happens when capacity is exceeded



3\.The difference between FIFO and priority queue



4\.How to collect and compute performance statistics



5\.How simulation can be used to study how system behaves.







This project demonstrates how scheduling strategy affects packet loss in a limited-capacity router buffer.



**FIFO treats all packets equally, while the priority method protects high-priority traffic. The comparison clearly shows how priority scheduling can reduce loss for important packets.**



The simulation provides a simple but practical demonstration of data structures applied to a networking problem.

##                            **END**     

###### 

###### 

###### 

###### **student details:**

**Name: FANUEL NGALA**

**Registration Number: FEE3/2879/2025**

**Unit: Computer science**

 

