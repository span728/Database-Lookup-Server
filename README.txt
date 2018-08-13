Stephanie Pan 
sp3507 
Lab6 
Due 4/7/18 11:59pm 

 
This part of the lab asked for a server that would connect the client to
mdb-lookup, from lab4.  I mainly put together code from tcp-recver.c and the
lab4 solution code of mdb-lookup.c  The main things I changed were changing
stdin and stdout to the socket.  This code works as expected and there are
no valgrind errors when you control-c after all connections have been
closed.  


