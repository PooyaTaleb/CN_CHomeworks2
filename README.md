# CN_CHomeworks2
second project for network course at Tehran university

first we set our paramaters for our simulation in the main function. our parameters our error rate and bit rate and the number of receiver nodes and sender nodes which should be the same. then we create our reciever and sender nodes and our load balancer node. we implement the load balancer node in the LB class.
the sender nodes send their packets via UDP protocol to the load balancer, and load balancer sends randomly sends each packet to a receiver node via TCP protocol.

we calculate throuput and average end to end delay parameters in the topology described above using two functions ThroughputMonitor and AverageDelayMonitor.

our ThroughputMonitor is as follows:
![image](https://user-images.githubusercontent.com/121708191/216147022-47841602-18f0-43de-9c57-1d27b8ba2024.png)

which yields the following result:
![image](https://user-images.githubusercontent.com/121708191/216147258-9df98ae1-525a-4833-87f7-c6d9707025a7.png)

the first 3 flows are from load balancer to 3 receiver nodes and the next 3 flows are from sender nodes to load balancer. each flow calculates a throughput which varies from link to link.

our AverageDelayMonitor is as follows:
![image](https://user-images.githubusercontent.com/121708191/216149057-287d533f-6acc-4d49-b3bd-b2405a8d08bf.png)

which yeilds the following result:
![image](https://user-images.githubusercontent.com/121708191/216149650-8da88795-aaca-46a8-b1c1-7b87bc278bd8.png)

