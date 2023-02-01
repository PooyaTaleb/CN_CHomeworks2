# CN_CHomeworks2
second project for network course at Tehran university

first we set our paramaters for our simulation in the main function. our parameters our error rate and bit rate and the number of receiver nodes and sender nodes which should be the same. then we create our reciever and sender nodes and our load balancer node. we implement the load balancer node in the LB class.
the sender nodes send their packets via UDP protocol to the load balancer, and load balancer sends randomly sends each packet to a receiver node via TCP protocol.
