neuralnetwork-with-pso
======================

neural network \w particle swarm optimizer

-----------------------note taking ----------------------------
I want to the get the content of the first node (node with the CommFlag="0")

so I did it as following;

xmlNodePtr pnode;
pnode=pxmlDoc->children->children;

but it does not work. If I change it to

pnode=pxmlDoc->children->children->next;

then it works. Can someone explain it to me.

In XML all characters in the content of the document are significant including blanks and formatting line breaks.
